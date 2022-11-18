// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::ffi::CStr;
use std::str;
use std::time::Duration;

use super::{
    gpiod,
    line::{Bias, Direction, Drive, Edge, EventClock, Offset},
    Error, Result,
};

/// Line info
///
/// Exposes functions for retrieving kernel information about both requested and
/// free lines.  Line info object contains an immutable snapshot of a line's status.
///
/// The line info contains all the publicly available information about a
/// line, which does not include the line value.  The line must be requested
/// to access the line value.

#[derive(Debug, Eq, PartialEq)]
pub struct Info {
    info: *mut gpiod::gpiod_line_info,
    contained: bool,
}

impl Info {
    fn new_internal(info: *mut gpiod::gpiod_line_info, contained: bool) -> Result<Self> {
        Ok(Self { info, contained })
    }

    /// Get a snapshot of information about the line.
    pub(crate) fn new(info: *mut gpiod::gpiod_line_info) -> Result<Self> {
        Info::new_internal(info, false)
    }

    /// Get a snapshot of information about the line and start watching it for changes.
    pub(crate) fn new_watch(info: *mut gpiod::gpiod_line_info) -> Result<Self> {
        Info::new_internal(info, false)
    }

    /// Get the Line info object associated with an event.
    pub(crate) fn new_from_event(info: *mut gpiod::gpiod_line_info) -> Result<Self> {
        Info::new_internal(info, true)
    }

    /// Get the offset of the line within the GPIO chip.
    ///
    /// The offset uniquely identifies the line on the chip. The combination of the chip and offset
    /// uniquely identifies the line within the system.

    pub fn offset(&self) -> Offset {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_get_offset(self.info) }
    }

    /// Get GPIO line's name.
    pub fn name(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Info`.
        let name = unsafe { gpiod::gpiod_line_info_get_name(self.info) };
        if name.is_null() {
            return Err(Error::NullString("GPIO line's name"));
        }

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Returns True if the line is in use, false otherwise.
    ///
    /// The user space can't know exactly why a line is busy. It may have been
    /// requested by another process or hogged by the kernel. It only matters that
    /// the line is used and we can't request it.
    pub fn is_used(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_used(self.info) }
    }

    /// Get the GPIO line's consumer name.
    pub fn consumer(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Info`.
        let name = unsafe { gpiod::gpiod_line_info_get_consumer(self.info) };
        if name.is_null() {
            return Err(Error::NullString("GPIO line's consumer name"));
        }

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get the GPIO line's direction.
    pub fn direction(&self) -> Result<Direction> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Direction::new(unsafe { gpiod::gpiod_line_info_get_direction(self.info) } as u32)
    }

    /// Returns true if the line is "active-low", false otherwise.
    pub fn is_active_low(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_active_low(self.info) }
    }

    /// Get the GPIO line's bias setting.
    pub fn bias(&self) -> Result<Option<Bias>> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Bias::new(unsafe { gpiod::gpiod_line_info_get_bias(self.info) } as u32)
    }

    /// Get the GPIO line's drive setting.
    pub fn drive(&self) -> Result<Drive> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Drive::new(unsafe { gpiod::gpiod_line_info_get_drive(self.info) } as u32)
    }

    /// Get the current edge detection setting of the line.
    pub fn edge_detection(&self) -> Result<Option<Edge>> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Edge::new(unsafe { gpiod::gpiod_line_info_get_edge_detection(self.info) } as u32)
    }

    /// Get the current event clock setting used for edge event timestamps.
    pub fn event_clock(&self) -> Result<EventClock> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        EventClock::new(unsafe { gpiod::gpiod_line_info_get_event_clock(self.info) } as u32)
    }

    /// Returns true if the line is debounced (either by hardware or by the
    /// kernel software debouncer), false otherwise.
    pub fn is_debounced(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_debounced(self.info) }
    }

    /// Get the debounce period of the line.
    pub fn debounce_period(&self) -> Duration {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Duration::from_micros(unsafe {
            gpiod::gpiod_line_info_get_debounce_period_us(self.info) as u64
        })
    }
}

impl Drop for Info {
    fn drop(&mut self) {
        // We must not free the Line info object created from `struct chip::Event` by calling
        // libgpiod API.
        if !self.contained {
            // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
            unsafe { gpiod::gpiod_line_info_free(self.info) }
        }
    }
}
