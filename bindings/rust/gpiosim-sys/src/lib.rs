// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use libgpiod::{Error, OperationType, Result};

#[allow(non_camel_case_types, non_upper_case_globals)]
#[cfg_attr(test, allow(deref_nullptr, non_snake_case))]
#[allow(dead_code)]
mod bindings_raw {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
use bindings_raw::*;

mod sim;
pub use sim::*;

use crate::{
    gpiosim_direction_GPIOSIM_DIRECTION_INPUT as GPIOSIM_DIRECTION_INPUT,
    gpiosim_direction_GPIOSIM_DIRECTION_OUTPUT_HIGH as GPIOSIM_DIRECTION_OUTPUT_HIGH,
    gpiosim_direction_GPIOSIM_DIRECTION_OUTPUT_LOW as GPIOSIM_DIRECTION_OUTPUT_LOW,
    gpiosim_pull_GPIOSIM_PULL_DOWN as GPIOSIM_PULL_DOWN,
    gpiosim_pull_GPIOSIM_PULL_UP as GPIOSIM_PULL_UP,
    gpiosim_value_GPIOSIM_VALUE_ACTIVE as GPIOSIM_VALUE_ACTIVE,
    gpiosim_value_GPIOSIM_VALUE_ERROR as GPIOSIM_VALUE_ERROR,
    gpiosim_value_GPIOSIM_VALUE_INACTIVE as GPIOSIM_VALUE_INACTIVE,
};

/// Value settings.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum Value {
    /// Active
    Active,
    /// Inactive
    InActive,
}

impl Value {
    pub(crate) fn new(val: gpiosim_value) -> Result<Self> {
        Ok(match val {
            GPIOSIM_VALUE_INACTIVE => Value::InActive,
            GPIOSIM_VALUE_ACTIVE => Value::Active,
            GPIOSIM_VALUE_ERROR => {
                return Err(Error::OperationFailed(
                    OperationType::SimBankGetVal,
                    errno::errno(),
                ))
            }
            _ => return Err(Error::InvalidEnumValue("Value", val)),
        })
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
    fn val(self) -> gpiosim_direction {
        match self {
            Direction::Input => GPIOSIM_DIRECTION_INPUT,
            Direction::OutputHigh => GPIOSIM_DIRECTION_OUTPUT_HIGH,
            Direction::OutputLow => GPIOSIM_DIRECTION_OUTPUT_LOW,
        }
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
    fn val(self) -> gpiosim_pull {
        match self {
            Pull::Up => GPIOSIM_PULL_UP,
            Pull::Down => GPIOSIM_PULL_DOWN,
        }
    }
}
