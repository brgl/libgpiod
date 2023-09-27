// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

pub mod info {
    /// GPIO chip info event related definitions.
    pub use crate::info_event::*;
}

use std::cmp::Ordering;
use std::ffi::{CStr, CString};
use std::os::{raw::c_char, unix::prelude::AsRawFd};
use std::path::Path;
use std::ptr;
use std::str;
use std::sync::Arc;
use std::time::Duration;

use super::{
    gpiod,
    line::{self, Offset},
    request, Error, OperationType, Result,
};

#[derive(Debug, Eq, PartialEq)]
struct Internal {
    chip: *mut gpiod::gpiod_chip,
}

impl Internal {
    /// Find a chip by path.
    fn open<P: AsRef<Path>>(path: &P) -> Result<Self> {
        // Null-terminate the string
        let path = path.as_ref().to_string_lossy() + "\0";

        // SAFETY: The `gpiod_chip` returned by libgpiod is guaranteed to live as long
        // as the `struct Internal`.
        let chip = unsafe { gpiod::gpiod_chip_open(path.as_ptr() as *const c_char) };
        if chip.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipOpen,
                errno::errno(),
            ));
        }

        Ok(Self { chip })
    }
}

impl Drop for Internal {
    /// Close the chip and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_chip_close(self.chip) }
    }
}

/// GPIO chip
///
/// A GPIO chip object is associated with an open file descriptor to the GPIO
/// character device. It exposes basic information about the chip and allows
/// callers to retrieve information about each line, watch lines for state
/// changes and make line requests.
#[derive(Debug, Eq, PartialEq)]
pub struct Chip {
    ichip: Arc<Internal>,
}

// SAFETY: Safe as `Internal` won't be freed until the `Chip` is dropped.
unsafe impl Send for Chip {}

impl Chip {
    /// Find a chip by path.
    pub fn open<P: AsRef<Path>>(path: &P) -> Result<Self> {
        let ichip = Arc::new(Internal::open(path)?);

        Ok(Self { ichip })
    }

    /// Get the chip name as represented in the kernel.
    pub fn info(&self) -> Result<Info> {
        Info::new(self)
    }

    /// Get the path used to find the chip.
    pub fn path(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Chip`.
        let path = unsafe { gpiod::gpiod_chip_get_path(self.ichip.chip) };

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(path) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get a snapshot of information about the line.
    pub fn line_info(&self, offset: Offset) -> Result<line::Info> {
        // SAFETY: The `gpiod_line_info` returned by libgpiod is guaranteed to live as long
        // as the `struct Info`.
        let info = unsafe { gpiod::gpiod_chip_get_line_info(self.ichip.chip, offset) };

        if info.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipGetLineInfo,
                errno::errno(),
            ));
        }

        line::Info::new(info)
    }

    /// Get the current snapshot of information about the line at given offset and start watching
    /// it for future changes.
    pub fn watch_line_info(&self, offset: Offset) -> Result<line::Info> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        let info = unsafe { gpiod::gpiod_chip_watch_line_info(self.ichip.chip, offset) };

        if info.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipWatchLineInfo,
                errno::errno(),
            ));
        }

        line::Info::new_watch(info)
    }

    /// Stop watching a line
    pub fn unwatch(&self, offset: Offset) {
        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_chip_unwatch_line_info(self.ichip.chip, offset);
        }
    }

    /// Wait for line status events on any of the watched lines on the chip.
    pub fn wait_info_event(&self, timeout: Option<Duration>) -> Result<bool> {
        let timeout = match timeout {
            Some(x) => x.as_nanos() as i64,
            // Block indefinitely
            None => -1,
        };

        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        let ret = unsafe { gpiod::gpiod_chip_wait_info_event(self.ichip.chip, timeout) };

        match ret {
            -1 => Err(Error::OperationFailed(
                OperationType::ChipWaitInfoEvent,
                errno::errno(),
            )),
            0 => Ok(false),
            _ => Ok(true),
        }
    }

    /// Read a single line status change event from the chip. If no events are
    /// pending, this function will block.
    pub fn read_info_event(&self) -> Result<info::Event> {
        // SAFETY: The `gpiod_info_event` returned by libgpiod is guaranteed to live as long
        // as the `struct Event`.
        let event = unsafe { gpiod::gpiod_chip_read_info_event(self.ichip.chip) };
        if event.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipReadInfoEvent,
                errno::errno(),
            ));
        }

        Ok(info::Event::new(event))
    }

    /// Map a GPIO line's name to its offset within the chip.
    pub fn line_offset_from_name(&self, name: &str) -> Result<Offset> {
        let name = CString::new(name).map_err(|_| Error::InvalidString)?;

        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_chip_get_line_offset_from_name(
                self.ichip.chip,
                name.as_ptr() as *const c_char,
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::ChipGetLineOffsetFromName,
                errno::errno(),
            ))
        } else {
            Ok(ret as u32)
        }
    }

    /// Request a set of lines for exclusive usage.
    pub fn request_lines(
        &self,
        rconfig: Option<&request::Config>,
        lconfig: &line::Config,
    ) -> Result<request::Request> {
        let req_cfg = match rconfig {
            Some(cfg) => cfg.config,
            _ => ptr::null(),
        } as *mut gpiod::gpiod_request_config;

        // SAFETY: The `gpiod_line_request` returned by libgpiod is guaranteed to live as long
        // as the `struct Request`.
        let request =
            unsafe { gpiod::gpiod_chip_request_lines(self.ichip.chip, req_cfg, lconfig.config) };

        if request.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipRequestLines,
                errno::errno(),
            ));
        }

        request::Request::new(request)
    }
}

impl AsRawFd for Chip {
    /// Get the file descriptor associated with the chip.
    ///
    /// The returned file descriptor must not be closed by the caller, else other methods for the
    /// `struct Chip` may fail.
    fn as_raw_fd(&self) -> i32 {
        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_chip_get_fd(self.ichip.chip) }
    }
}

/// GPIO chip Information
#[derive(Debug, Eq)]
pub struct Info {
    info: *mut gpiod::gpiod_chip_info,
}

impl Info {
    /// Find a GPIO chip by path.
    fn new(chip: &Chip) -> Result<Self> {
        // SAFETY: `chip.ichip.chip` is guaranteed to be valid here.
        let info = unsafe { gpiod::gpiod_chip_get_info(chip.ichip.chip) };
        if info.is_null() {
            return Err(Error::OperationFailed(
                OperationType::ChipGetInfo,
                errno::errno(),
            ));
        }

        Ok(Self { info })
    }

    /// Get the GPIO chip name as represented in the kernel.
    pub fn name(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Chip`.
        let name = unsafe { gpiod::gpiod_chip_info_get_name(self.info) };

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get the GPIO chip label as represented in the kernel.
    pub fn label(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Chip`.
        let label = unsafe { gpiod::gpiod_chip_info_get_label(self.info) };

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(label) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get the number of GPIO lines exposed by the chip.
    pub fn num_lines(&self) -> usize {
        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_chip_info_get_num_lines(self.info) }
    }
}

impl PartialEq for Info {
    fn eq(&self, other: &Self) -> bool {
        self.name().unwrap().eq(other.name().unwrap())
    }
}

impl PartialOrd for Info {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        let name = match self.name() {
            Ok(name) => name,
            _ => return None,
        };

        let other_name = match other.name() {
            Ok(name) => name,
            _ => return None,
        };

        name.partial_cmp(other_name)
    }
}

impl Drop for Info {
    /// Close the GPIO chip info and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_chip` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_chip_info_free(self.info) }
    }
}
