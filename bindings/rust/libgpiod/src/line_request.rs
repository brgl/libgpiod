// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

#[cfg(feature = "v2_1")]
use std::ffi::CStr;
use std::os::unix::prelude::AsRawFd;
use std::time::Duration;

use super::{
    gpiod,
    line::{self, Offset, Value, ValueMap},
    request, Error, OperationType, Result,
};

/// Line request operations
///
/// Allows interaction with a set of requested lines.
#[derive(Debug, Eq, PartialEq)]
pub struct Request {
    pub(crate) request: *mut gpiod::gpiod_line_request,
}

// SAFETY: Request models a wrapper around an owned gpiod_line_request and may
// be safely sent to other threads.
unsafe impl Send for Request {}

impl Request {
    /// Request a set of lines for exclusive usage.
    ///
    /// SAFETY: The pointer must point to an instance that is valid. After
    /// constructing a [Request] the pointer MUST NOT be used for any other
    /// purpose anymore. All interactions with the libgpiod API have to happen
    /// through this object.
    pub(crate) unsafe fn new(request: *mut gpiod::gpiod_line_request) -> Result<Self> {
        Ok(Self { request })
    }

    /// Get the name of the chip this request was made on.
    #[cfg(feature = "v2_1")]
    pub fn chip_name(&self) -> Result<&str> {
        // SAFETY: The `gpiod_line_request` is guaranteed to be live as long
        // as `&self`
        let name = unsafe { gpiod::gpiod_line_request_get_chip_name(self.request) };

        // SAFETY: The string is guaranteed to be valid, non-null and immutable
        // by the C API for the lifetime of the `gpiod_line_request`.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get the number of lines in the request.
    pub fn num_lines(&self) -> usize {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_request_get_num_requested_lines(self.request) }
    }

    /// Get the offsets of lines in the request.
    pub fn offsets(&self) -> Vec<Offset> {
        let mut offsets = vec![0; self.num_lines()];

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let num_offsets = unsafe {
            gpiod::gpiod_line_request_get_requested_offsets(
                self.request,
                offsets.as_mut_ptr(),
                self.num_lines(),
            )
        };
        offsets.shrink_to(num_offsets);
        offsets
    }

    /// Get the value (0 or 1) of a single line associated with the request.
    pub fn value(&self, offset: Offset) -> Result<Value> {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let value = unsafe { gpiod::gpiod_line_request_get_value(self.request, offset) };

        if value != 0 && value != 1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestGetVal,
                errno::errno(),
            ))
        } else {
            Value::new(value)
        }
    }

    /// Get values of a subset of lines associated with the request.
    pub fn values_subset(&self, offsets: &[Offset]) -> Result<ValueMap> {
        let mut values = vec![0; offsets.len()];

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_request_get_values_subset(
                self.request,
                offsets.len(),
                offsets.as_ptr(),
                values.as_mut_ptr(),
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestGetValSubset,
                errno::errno(),
            ))
        } else {
            let mut map = ValueMap::new();

            for (i, val) in values.iter().enumerate() {
                map.insert(offsets[i].into(), Value::new(*val)?);
            }

            Ok(map)
        }
    }

    /// Get values of all lines associated with the request.
    pub fn values(&self) -> Result<ValueMap> {
        self.values_subset(&self.offsets())
    }

    /// Set the value of a single line associated with the request.
    pub fn set_value(&mut self, offset: Offset, value: Value) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_request_set_value(self.request, offset, value.value()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestSetVal,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Set values of a subset of lines associated with the request.
    pub fn set_values_subset(&mut self, map: ValueMap) -> Result<&mut Self> {
        let mut offsets = Vec::new();
        let mut values = Vec::new();

        for (offset, value) in map {
            offsets.push(offset as u32);
            values.push(value.value());
        }

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_request_set_values_subset(
                self.request,
                offsets.len(),
                offsets.as_ptr(),
                values.as_ptr(),
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestSetValSubset,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Set values of all lines associated with the request.
    pub fn set_values(&mut self, values: &[Value]) -> Result<&mut Self> {
        if values.len() != self.num_lines() {
            return Err(Error::InvalidArguments);
        }

        let mut new_values = Vec::new();
        for value in values {
            new_values.push(value.value());
        }

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_request_set_values(self.request, new_values.as_ptr()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestSetVal,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Update the configuration of lines associated with the line request.
    pub fn reconfigure_lines(&mut self, lconfig: &line::Config) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_request_reconfigure_lines(self.request, lconfig.config) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestReconfigLines,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Wait for edge events on any of the lines associated with the request.
    pub fn wait_edge_events(&self, timeout: Option<Duration>) -> Result<bool> {
        let timeout = match timeout {
            Some(x) => x.as_nanos() as i64,
            // Block indefinitely
            None => -1,
        };

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret = unsafe { gpiod::gpiod_line_request_wait_edge_events(self.request, timeout) };

        match ret {
            -1 => Err(Error::OperationFailed(
                OperationType::LineRequestWaitEdgeEvent,
                errno::errno(),
            )),
            0 => Ok(false),
            _ => Ok(true),
        }
    }

    /// Get a number of edge events from a line request.
    ///
    /// This function will block if no event was queued for the line.
    pub fn read_edge_events<'a>(
        &self,
        buffer: &'a mut request::Buffer,
    ) -> Result<request::Events<'a>> {
        buffer.read_edge_events(self)
    }
}

impl AsRawFd for Request {
    /// Get the file descriptor associated with the line request.
    fn as_raw_fd(&self) -> i32 {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_request_get_fd(self.request) }
    }
}

impl Drop for Request {
    /// Release the requested lines and free all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_request_release(self.request) }
    }
}
