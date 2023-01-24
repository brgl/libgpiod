// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::os::{raw::c_ulong, unix::prelude::AsRawFd};
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

impl Request {
    /// Request a set of lines for exclusive usage.
    pub(crate) fn new(request: *mut gpiod::gpiod_line_request) -> Result<Self> {
        Ok(Self { request })
    }

    /// Get the number of lines in the request.
    pub fn num_lines(&self) -> usize {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_request_get_num_requested_lines(self.request) as usize }
    }

    /// Get the offsets of lines in the request.
    pub fn offsets(&self) -> Vec<Offset> {
        let mut offsets = vec![0; self.num_lines() as usize];

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let num_offsets = unsafe {
            gpiod::gpiod_line_request_get_requested_offsets(
                self.request,
                offsets.as_mut_ptr(),
                self.num_lines() as u64,
            )
        };
        offsets.shrink_to(num_offsets as usize);
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
                offsets.len() as c_ulong,
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
    pub fn set_value(&self, offset: Offset, value: Value) -> Result<()> {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_request_set_value(self.request, offset, value.value()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestSetVal,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    /// Set values of a subset of lines associated with the request.
    pub fn set_values_subset(&self, map: ValueMap) -> Result<()> {
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
                offsets.len() as c_ulong,
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
            Ok(())
        }
    }

    /// Set values of all lines associated with the request.
    pub fn set_values(&self, values: &[Value]) -> Result<()> {
        if values.len() != self.num_lines() as usize {
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
            Ok(())
        }
    }

    /// Update the configuration of lines associated with the line request.
    pub fn reconfigure_lines(&self, lconfig: &line::Config) -> Result<()> {
        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_request_reconfigure_lines(self.request, lconfig.config) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestReconfigLines,
                errno::errno(),
            ))
        } else {
            Ok(())
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
        &'a self,
        buffer: &'a mut request::Buffer,
    ) -> Result<request::Events> {
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
