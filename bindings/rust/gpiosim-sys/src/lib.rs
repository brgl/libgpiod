// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use libgpiod::{Error, Result};

#[allow(non_camel_case_types, non_upper_case_globals)]
#[cfg_attr(test, allow(deref_nullptr, non_snake_case))]
#[allow(dead_code)]
mod bindings_raw {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
use bindings_raw::*;

mod sim;
pub use sim::*;

/// Value settings.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum Value {
    /// Active
    Active,
    /// Inactive
    InActive,
}

impl Value {
    pub(crate) fn new(val: u32) -> Result<Self> {
        match val {
            GPIOSIM_VALUE_INACTIVE => Ok(Value::InActive),
            GPIOSIM_VALUE_ACTIVE => Ok(Value::Active),
            _ => Err(Error::InvalidEnumValue("Value", val as u32)),
        }
    }
}

/// Direction settings.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum Direction {
    /// Direction is input - for reading the value of an externally driven GPIO line.
    Input,
    /// Direction is output - for driving the GPIO line, value is high.
    OutputHigh,
    /// Direction is output - for driving the GPIO line, value is low.
    OutputLow,
}

impl Direction {
    fn val(self) -> i32 {
        (match self {
            Direction::Input => GPIOSIM_HOG_DIR_INPUT,
            Direction::OutputHigh => GPIOSIM_HOG_DIR_OUTPUT_HIGH,
            Direction::OutputLow => GPIOSIM_HOG_DIR_OUTPUT_LOW,
        }) as i32
    }
}

/// Internal pull settings.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum Pull {
    /// The internal pull-up is enabled.
    Up,
    /// The internal pull-down is enabled.
    Down,
}

impl Pull {
    fn val(self) -> i32 {
        (match self {
            Pull::Up => GPIOSIM_PULL_UP,
            Pull::Down => GPIOSIM_PULL_DOWN,
        }) as i32
    }
}
