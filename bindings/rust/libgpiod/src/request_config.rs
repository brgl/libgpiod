// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::str;

use super::{Error, OperationType, Result, gpiod};

/// Request configuration objects
///
/// Request config objects are used to pass a set of options to the kernel at
/// the time of the line request. The mutators don't return error values. If the
/// values are invalid, in general they are silently adjusted to acceptable
/// ranges.

#[derive(Debug, Eq, PartialEq)]
pub struct Config {
    pub(crate) config: *mut gpiod::gpiod_request_config,
}

// SAFETY: Config models a wrapper around an owned gpiod_request_config and may
// be safely sent to other threads.
unsafe impl Send for Config {}

impl Config {
    /// Create a new request config object.
    pub fn new() -> Result<Self> {
        // SAFETY: The `gpiod_request_config` returned by libgpiod is guaranteed to live as long
        // as the `struct Config`.
        let config = unsafe { gpiod::gpiod_request_config_new() };
        if config.is_null() {
            return Err(Error::OperationFailed(
                OperationType::RequestConfigNew,
                errno::errno(),
            ));
        }

        Ok(Self { config })
    }

    /// Set the consumer name for the request.
    ///
    /// If the consumer string is too long, it will be truncated to the max
    /// accepted length.
    pub fn set_consumer(&mut self, consumer: &str) -> Result<&mut Self> {
        let consumer = CString::new(consumer).map_err(|_| Error::InvalidString)?;

        // SAFETY: `gpiod_request_config` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_request_config_set_consumer(
                self.config,
                consumer.as_ptr() as *const c_char,
            )
        }

        Ok(self)
    }

    /// Get the consumer name configured in the request config.
    pub fn consumer(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Config`.
        let consumer = unsafe { gpiod::gpiod_request_config_get_consumer(self.config) };
        if consumer.is_null() {
            return Err(Error::OperationFailed(
                OperationType::RequestConfigGetConsumer,
                errno::errno(),
            ));
        }

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(consumer) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Set the size of the kernel event buffer for the request.
    pub fn set_event_buffer_size(&mut self, size: usize) -> &mut Self {
        // SAFETY: `gpiod_request_config` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_request_config_set_event_buffer_size(self.config, size) }

        self
    }

    /// Get the edge event buffer size setting for the request config.
    pub fn event_buffer_size(&self) -> usize {
        // SAFETY: `gpiod_request_config` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_request_config_get_event_buffer_size(self.config) }
    }
}

impl Drop for Config {
    /// Free the request config object and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_request_config` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_request_config_free(self.config) }
    }
}
