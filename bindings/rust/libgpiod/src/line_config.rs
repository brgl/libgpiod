// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use super::{
    Error, OperationType, Result, gpiod,
    line::{Offset, Settings, SettingsMap, Value},
};

/// Line configuration objects.
///
/// The line-config object contains the configuration for lines that can be
/// used in two cases:
///  - when making a line request
///  - when reconfiguring a set of already requested lines.
///
/// A new line-config object is empty. Using it in a request will lead to an
/// error. In order for a line-config to become useful, it needs to be assigned
/// at least one offset-to-settings mapping by calling
/// ::gpiod_line_config_add_line_settings.
///
/// When calling ::gpiod_chip_request_lines, the library will request all
/// offsets that were assigned settings in the order that they were assigned.

#[derive(Debug, Eq, PartialEq)]
pub struct Config {
    pub(crate) config: *mut gpiod::gpiod_line_config,
}

// SAFETY: Config models a wrapper around an owned gpiod_line_config and may be
// safely sent to other threads.
unsafe impl Send for Config {}

impl Config {
    /// Create a new line config object.
    pub fn new() -> Result<Self> {
        // SAFETY: The `gpiod_line_config` returned by libgpiod is guaranteed to live as long
        // as the `struct Config`.
        let config = unsafe { gpiod::gpiod_line_config_new() };

        if config.is_null() {
            return Err(Error::OperationFailed(
                OperationType::LineConfigNew,
                errno::errno(),
            ));
        }

        Ok(Self { config })
    }

    /// Resets the entire configuration stored in the object. This is useful if
    /// the user wants to reuse the object without reallocating it.
    pub fn reset(&mut self) {
        // SAFETY: `gpiod_line_config` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_config_reset(self.config) }
    }

    /// Add line settings for a set of offsets.
    pub fn add_line_settings(
        &mut self,
        offsets: &[Offset],
        settings: Settings,
    ) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_config` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_config_add_line_settings(
                self.config,
                offsets.as_ptr(),
                offsets.len(),
                settings.settings,
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineConfigAddSettings,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Set output values for a number of lines.
    pub fn set_output_values(&mut self, values: &[Value]) -> Result<&mut Self> {
        let mut mapped_values = Vec::new();
        for value in values {
            mapped_values.push(value.value());
        }

        let ret = unsafe {
            gpiod::gpiod_line_config_set_output_values(
                self.config,
                mapped_values.as_ptr(),
                values.len(),
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineConfigSetOutputValues,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get a mapping of offsets to line settings stored by this object.
    pub fn line_settings(&self) -> Result<SettingsMap> {
        let mut map = SettingsMap::new();
        // SAFETY: gpiod_line_config is guaranteed to be valid here
        let num_lines = unsafe { gpiod::gpiod_line_config_get_num_configured_offsets(self.config) };
        let mut offsets = vec![0; num_lines];

        // SAFETY: gpiod_line_config is guaranteed to be valid here.
        let num_stored = unsafe {
            gpiod::gpiod_line_config_get_configured_offsets(
                self.config,
                offsets.as_mut_ptr(),
                num_lines,
            )
        };

        for offset in &offsets[0..num_stored] {
            // SAFETY: `gpiod_line_config` is guaranteed to be valid here.
            let settings =
                unsafe { gpiod::gpiod_line_config_get_line_settings(self.config, *offset) };
            if settings.is_null() {
                return Err(Error::OperationFailed(
                    OperationType::LineConfigGetSettings,
                    errno::errno(),
                ));
            }

            // SAFETY: The above `gpiod_line_config_get_line_settings` call
            // returns a copy of the line_settings. We thus have sole ownership.
            // We no longer use the pointer for any other purpose.
            let settings = unsafe { Settings::from_raw(settings) };

            map.insert(*offset as Offset, settings);
        }

        Ok(map)
    }
}

impl Drop for Config {
    /// Free the line config object and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_line_config` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_config_free(self.config) }
    }
}
