// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::time::Duration;

use super::{
    gpiod,
    line::{Bias, Direction, Drive, Edge, EventClock, SettingKind, SettingVal, Value},
    Error, OperationType, Result,
};

/// Line settings objects.
///
/// Line settings object contains a set of line properties that can be used
/// when requesting lines or reconfiguring an existing request.
///
/// Mutators in general can only fail if the new property value is invalid. The
/// return values can be safely ignored - the object remains valid even after
/// a mutator fails and simply uses the sane default appropriate for given
/// property.

#[derive(Debug, Eq, PartialEq)]
pub struct Settings {
    pub(crate) settings: *mut gpiod::gpiod_line_settings,
}

impl Settings {
    /// Create a new line settings object.
    pub fn new() -> Result<Self> {
        // SAFETY: The `gpiod_line_settings` returned by libgpiod is guaranteed to live as long
        // as the `struct Settings`.
        let settings = unsafe { gpiod::gpiod_line_settings_new() };

        if settings.is_null() {
            return Err(Error::OperationFailed(
                OperationType::LineSettingsNew,
                errno::errno(),
            ));
        }

        Ok(Self { settings })
    }

    pub fn new_with_settings(settings: *mut gpiod::gpiod_line_settings) -> Self {
        Self { settings }
    }

    /// Resets the line settings object to its default values.
    pub fn reset(&mut self) {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_settings_reset(self.settings) }
    }

    /// Makes copy of the settings object.
    pub fn settings_clone(&self) -> Result<Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let settings = unsafe { gpiod::gpiod_line_settings_copy(self.settings) };
        if settings.is_null() {
            return Err(Error::OperationFailed(
                OperationType::LineSettingsCopy,
                errno::errno(),
            ));
        }

        Ok(Self { settings })
    }

    /// Set line prop setting.
    pub fn set_prop(&mut self, props: &[SettingVal]) -> Result<&mut Self> {
        for property in props {
            match property {
                SettingVal::Direction(prop) => self.set_direction(*prop)?,
                SettingVal::EdgeDetection(prop) => self.set_edge_detection(*prop)?,
                SettingVal::Bias(prop) => self.set_bias(*prop)?,
                SettingVal::Drive(prop) => self.set_drive(*prop)?,
                SettingVal::ActiveLow(prop) => self.set_active_low(*prop),
                SettingVal::DebouncePeriod(prop) => self.set_debounce_period(*prop),
                SettingVal::EventClock(prop) => self.set_event_clock(*prop)?,
                SettingVal::OutputValue(prop) => self.set_output_value(*prop)?,
            };
        }

        Ok(self)
    }

    /// Get the line prop setting.
    pub fn prop(&self, property: SettingKind) -> Result<SettingVal> {
        Ok(match property {
            SettingKind::Direction => SettingVal::Direction(self.direction()?),
            SettingKind::EdgeDetection => SettingVal::EdgeDetection(self.edge_detection()?),
            SettingKind::Bias => SettingVal::Bias(self.bias()?),
            SettingKind::Drive => SettingVal::Drive(self.drive()?),
            SettingKind::ActiveLow => SettingVal::ActiveLow(self.active_low()),
            SettingKind::DebouncePeriod => SettingVal::DebouncePeriod(self.debounce_period()?),
            SettingKind::EventClock => SettingVal::EventClock(self.event_clock()?),
            SettingKind::OutputValue => SettingVal::OutputValue(self.output_value()?),
        })
    }

    /// Set the line direction.
    pub fn set_direction(&mut self, direction: Direction) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_settings_set_direction(self.settings, direction.gpiod_direction())
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetDirection,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the direction setting.
    pub fn direction(&self) -> Result<Direction> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        Direction::new(unsafe { gpiod::gpiod_line_settings_get_direction(self.settings) })
    }

    /// Set the edge event detection setting.
    pub fn set_edge_detection(&mut self, edge: Option<Edge>) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_settings_set_edge_detection(self.settings, Edge::gpiod_edge(edge))
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetEdgeDetection,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the edge event detection setting.
    pub fn edge_detection(&self) -> Result<Option<Edge>> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        Edge::new(unsafe { gpiod::gpiod_line_settings_get_edge_detection(self.settings) })
    }

    /// Set the bias setting.
    pub fn set_bias(&mut self, bias: Option<Bias>) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_settings_set_bias(self.settings, Bias::gpiod_bias(bias)) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetBias,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the bias setting.
    pub fn bias(&self) -> Result<Option<Bias>> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        Bias::new(unsafe { gpiod::gpiod_line_settings_get_bias(self.settings) })
    }

    /// Set the drive setting.
    pub fn set_drive(&mut self, drive: Drive) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_settings_set_drive(self.settings, drive.gpiod_drive()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetDrive,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the drive setting.
    pub fn drive(&self) -> Result<Drive> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        Drive::new(unsafe { gpiod::gpiod_line_settings_get_drive(self.settings) })
    }

    /// Set active-low setting.
    pub fn set_active_low(&mut self, active_low: bool) -> &mut Self {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_line_settings_set_active_low(self.settings, active_low);
        }
        self
    }

    /// Check the active-low setting.
    pub fn active_low(&self) -> bool {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_settings_get_active_low(self.settings) }
    }

    /// Set the debounce period setting.
    pub fn set_debounce_period(&mut self, period: Duration) -> &mut Self {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_line_settings_set_debounce_period_us(
                self.settings,
                period.as_micros().try_into().unwrap(),
            );
        }

        self
    }

    /// Get the debounce period.
    pub fn debounce_period(&self) -> Result<Duration> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        Ok(Duration::from_micros(unsafe {
            gpiod::gpiod_line_settings_get_debounce_period_us(self.settings) as u64
        }))
    }

    /// Set the event clock setting.
    pub fn set_event_clock(&mut self, clock: EventClock) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_settings_set_event_clock(self.settings, clock.gpiod_clock())
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetEventClock,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the event clock setting.
    pub fn event_clock(&self) -> Result<EventClock> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        EventClock::new(unsafe { gpiod::gpiod_line_settings_get_event_clock(self.settings) })
    }

    /// Set the output value setting.
    pub fn set_output_value(&mut self, value: Value) -> Result<&mut Self> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let ret =
            unsafe { gpiod::gpiod_line_settings_set_output_value(self.settings, value.value()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsSetOutputValue,
                errno::errno(),
            ))
        } else {
            Ok(self)
        }
    }

    /// Get the output value, 0 or 1.
    pub fn output_value(&self) -> Result<Value> {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        let value = unsafe { gpiod::gpiod_line_settings_get_output_value(self.settings) };

        if value != 0 && value != 1 {
            Err(Error::OperationFailed(
                OperationType::LineSettingsGetOutVal,
                errno::errno(),
            ))
        } else {
            Value::new(value)
        }
    }
}

impl Drop for Settings {
    /// Free the line settings object and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_line_settings` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_settings_free(self.settings) }
    }
}
