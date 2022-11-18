// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Rust wrappers for GPIOD APIs

//! libgpiod public API
//!
//! This is the complete documentation of the public Rust API made available to
//! users of libgpiod.
//!
//! The API is logically split into several parts such as: GPIO chip & line
//! operators, GPIO events handling etc.

use std::ffi::CStr;
use std::fs;
use std::os::raw::c_char;
use std::path::Path;
use std::time::Duration;
use std::{fmt, str};

use intmap::IntMap;
use thiserror::Error as ThisError;

use libgpiod_sys as gpiod;

/// Operation types, used with OperationFailed() Error.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum OperationType {
    ChipOpen,
    ChipWaitInfoEvent,
    ChipGetLine,
    ChipGetLineInfo,
    ChipGetLineOffsetFromName,
    ChipGetInfo,
    ChipReadInfoEvent,
    ChipRequestLines,
    ChipWatchLineInfo,
    EdgeEventBufferGetEvent,
    EdgeEventCopy,
    EdgeEventBufferNew,
    InfoEventGetLineInfo,
    LineConfigNew,
    LineConfigAddSettings,
    LineConfigGetOffsets,
    LineConfigGetSettings,
    LineRequestReconfigLines,
    LineRequestGetVal,
    LineRequestGetValSubset,
    LineRequestSetVal,
    LineRequestSetValSubset,
    LineRequestReadEdgeEvent,
    LineRequestWaitEdgeEvent,
    LineSettingsNew,
    LineSettingsCopy,
    LineSettingsGetOutVal,
    LineSettingsSetDirection,
    LineSettingsSetEdgeDetection,
    LineSettingsSetBias,
    LineSettingsSetDrive,
    LineSettingsSetActiveLow,
    LineSettingsSetDebouncePeriod,
    LineSettingsSetEventClock,
    LineSettingsSetOutputValue,
    RequestConfigNew,
    RequestConfigGetConsumer,
    SimBankGetVal,
    SimBankNew,
    SimBankSetLabel,
    SimBankSetNumLines,
    SimBankSetLineName,
    SimBankSetPull,
    SimBankHogLine,
    SimCtxNew,
    SimDevNew,
    SimDevEnable,
    SimDevDisable,
}

impl fmt::Display for OperationType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

/// Result of libgpiod operations.
pub type Result<T> = std::result::Result<T, Error>;

/// Error codes for libgpiod operations.
#[derive(Copy, Clone, Debug, Eq, PartialEq, ThisError)]
pub enum Error {
    #[error("Failed to get {0}")]
    NullString(&'static str),
    #[error("String not utf8: {0:?}")]
    StringNotUtf8(str::Utf8Error),
    #[error("Invalid String")]
    InvalidString,
    #[error("Invalid enum {0} value: {1}")]
    InvalidEnumValue(&'static str, u32),
    #[error("Operation {0} Failed: {1}")]
    OperationFailed(OperationType, errno::Errno),
    #[error("Invalid Arguments")]
    InvalidArguments,
    #[error("Event count more than buffer capacity: {0} > {1}")]
    TooManyEvents(usize, usize),
    #[error("Std Io Error")]
    IoError,
}

mod info_event;

/// GPIO chip related definitions.
pub mod chip;

mod edge_event;
mod event_buffer;
mod line_request;
mod request_config;

/// GPIO chip request related definitions.
pub mod request {
    pub use crate::edge_event::*;
    pub use crate::event_buffer::*;
    pub use crate::line_request::*;
    pub use crate::request_config::*;
}

mod line_config;
mod line_info;
mod line_settings;

/// GPIO chip line related definitions.
pub mod line {
    pub use crate::line_config::*;
    pub use crate::line_info::*;
    pub use crate::line_settings::*;

    use super::*;

    /// Value settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum Value {
        /// Active
        Active,
        /// Inactive
        InActive,
    }

    /// Maps offset to Value.
    pub type ValueMap = IntMap<Value>;

    impl Value {
        pub fn new(val: i32) -> Result<Self> {
            Ok(match val {
                0 => Value::InActive,
                1 => Value::Active,
                _ => return Err(Error::InvalidEnumValue("Value", val as u32)),
            })
        }

        pub(crate) fn value(&self) -> i32 {
            match self {
                Value::Active => 1,
                Value::InActive => 0,
            }
        }
    }

    /// Offset type.
    pub type Offset = u32;

    /// Direction settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum Direction {
        /// Request the line(s), but don't change direction.
        AsIs,
        /// Direction is input - for reading the value of an externally driven GPIO line.
        Input,
        /// Direction is output - for driving the GPIO line.
        Output,
    }

    impl Direction {
        pub(crate) fn new(dir: u32) -> Result<Self> {
            Ok(match dir {
                gpiod::GPIOD_LINE_DIRECTION_AS_IS => Direction::AsIs,
                gpiod::GPIOD_LINE_DIRECTION_INPUT => Direction::Input,
                gpiod::GPIOD_LINE_DIRECTION_OUTPUT => Direction::Output,
                _ => return Err(Error::InvalidEnumValue("Direction", dir)),
            })
        }

        pub(crate) fn gpiod_direction(&self) -> u32 {
            match self {
                Direction::AsIs => gpiod::GPIOD_LINE_DIRECTION_AS_IS,
                Direction::Input => gpiod::GPIOD_LINE_DIRECTION_INPUT,
                Direction::Output => gpiod::GPIOD_LINE_DIRECTION_OUTPUT,
            }
        }
    }

    /// Internal bias settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum Bias {
        /// The internal bias is disabled.
        Disabled,
        /// The internal pull-up bias is enabled.
        PullUp,
        /// The internal pull-down bias is enabled.
        PullDown,
    }

    impl Bias {
        pub(crate) fn new(bias: u32) -> Result<Option<Self>> {
            Ok(match bias {
                gpiod::GPIOD_LINE_BIAS_UNKNOWN => None,
                gpiod::GPIOD_LINE_BIAS_AS_IS => None,
                gpiod::GPIOD_LINE_BIAS_DISABLED => Some(Bias::Disabled),
                gpiod::GPIOD_LINE_BIAS_PULL_UP => Some(Bias::PullUp),
                gpiod::GPIOD_LINE_BIAS_PULL_DOWN => Some(Bias::PullDown),
                _ => return Err(Error::InvalidEnumValue("Bias", bias)),
            })
        }

        pub(crate) fn gpiod_bias(bias: Option<Bias>) -> u32 {
            match bias {
                None => gpiod::GPIOD_LINE_BIAS_AS_IS,
                Some(bias) => match bias {
                    Bias::Disabled => gpiod::GPIOD_LINE_BIAS_DISABLED,
                    Bias::PullUp => gpiod::GPIOD_LINE_BIAS_PULL_UP,
                    Bias::PullDown => gpiod::GPIOD_LINE_BIAS_PULL_DOWN,
                },
            }
        }
    }

    /// Drive settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum Drive {
        /// Drive setting is push-pull.
        PushPull,
        /// Line output is open-drain.
        OpenDrain,
        /// Line output is open-source.
        OpenSource,
    }

    impl Drive {
        pub(crate) fn new(drive: u32) -> Result<Self> {
            Ok(match drive {
                gpiod::GPIOD_LINE_DRIVE_PUSH_PULL => Drive::PushPull,
                gpiod::GPIOD_LINE_DRIVE_OPEN_DRAIN => Drive::OpenDrain,
                gpiod::GPIOD_LINE_DRIVE_OPEN_SOURCE => Drive::OpenSource,
                _ => return Err(Error::InvalidEnumValue("Drive", drive)),
            })
        }

        pub(crate) fn gpiod_drive(&self) -> u32 {
            match self {
                Drive::PushPull => gpiod::GPIOD_LINE_DRIVE_PUSH_PULL,
                Drive::OpenDrain => gpiod::GPIOD_LINE_DRIVE_OPEN_DRAIN,
                Drive::OpenSource => gpiod::GPIOD_LINE_DRIVE_OPEN_SOURCE,
            }
        }
    }

    /// Edge detection settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum Edge {
        /// Line detects rising edge events.
        Rising,
        /// Line detects falling edge events.
        Falling,
        /// Line detects both rising and falling edge events.
        Both,
    }

    impl Edge {
        pub(crate) fn new(edge: u32) -> Result<Option<Self>> {
            Ok(match edge {
                gpiod::GPIOD_LINE_EDGE_NONE => None,
                gpiod::GPIOD_LINE_EDGE_RISING => Some(Edge::Rising),
                gpiod::GPIOD_LINE_EDGE_FALLING => Some(Edge::Falling),
                gpiod::GPIOD_LINE_EDGE_BOTH => Some(Edge::Both),
                _ => return Err(Error::InvalidEnumValue("Edge", edge)),
            })
        }

        pub(crate) fn gpiod_edge(edge: Option<Edge>) -> u32 {
            match edge {
                None => gpiod::GPIOD_LINE_EDGE_NONE,
                Some(edge) => match edge {
                    Edge::Rising => gpiod::GPIOD_LINE_EDGE_RISING,
                    Edge::Falling => gpiod::GPIOD_LINE_EDGE_FALLING,
                    Edge::Both => gpiod::GPIOD_LINE_EDGE_BOTH,
                },
            }
        }
    }

    /// Line setting kind.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum SettingKind {
        /// Line direction.
        Direction,
        /// Bias.
        Bias,
        /// Drive.
        Drive,
        /// Edge detection.
        EdgeDetection,
        /// Active-low setting.
        ActiveLow,
        /// Debounce period.
        DebouncePeriod,
        /// Event clock type.
        EventClock,
        /// Output value.
        OutputValue,
    }

    /// Line settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum SettingVal {
        /// Line direction.
        Direction(Direction),
        /// Bias.
        Bias(Option<Bias>),
        /// Drive.
        Drive(Drive),
        /// Edge detection.
        EdgeDetection(Option<Edge>),
        /// Active-low setting.
        ActiveLow(bool),
        /// Debounce period.
        DebouncePeriod(Duration),
        /// Event clock type.
        EventClock(EventClock),
        /// Output value.
        OutputValue(Value),
    }

    impl fmt::Display for SettingVal {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            write!(f, "{:?}", self)
        }
    }

    /// Event clock settings.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum EventClock {
        /// Line uses the monotonic clock for edge event timestamps.
        Monotonic,
        /// Line uses the realtime clock for edge event timestamps.
        Realtime,
        /// Line uses the hardware timestamp engine clock for edge event timestamps.
        HTE,
    }

    impl EventClock {
        pub(crate) fn new(clock: u32) -> Result<Self> {
            Ok(match clock {
                gpiod::GPIOD_LINE_EVENT_CLOCK_MONOTONIC => EventClock::Monotonic,
                gpiod::GPIOD_LINE_EVENT_CLOCK_REALTIME => EventClock::Realtime,
                gpiod::GPIOD_LINE_EVENT_CLOCK_HTE => EventClock::HTE,
                _ => return Err(Error::InvalidEnumValue("Eventclock", clock)),
            })
        }

        pub(crate) fn gpiod_clock(&self) -> u32 {
            match self {
                EventClock::Monotonic => gpiod::GPIOD_LINE_EVENT_CLOCK_MONOTONIC,
                EventClock::Realtime => gpiod::GPIOD_LINE_EVENT_CLOCK_REALTIME,
                EventClock::HTE => gpiod::GPIOD_LINE_EVENT_CLOCK_HTE,
            }
        }
    }

    /// Line status change event types.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum InfoChangeKind {
        /// Line has been requested.
        LineRequested,
        /// Previously requested line has been released.
        LineReleased,
        /// Line configuration has changed.
        LineConfigChanged,
    }

    impl InfoChangeKind {
        pub(crate) fn new(kind: u32) -> Result<Self> {
            Ok(match kind {
                gpiod::GPIOD_INFO_EVENT_LINE_REQUESTED => InfoChangeKind::LineRequested,
                gpiod::GPIOD_INFO_EVENT_LINE_RELEASED => InfoChangeKind::LineReleased,
                gpiod::GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED => InfoChangeKind::LineConfigChanged,
                _ => return Err(Error::InvalidEnumValue("InfoChangeKind", kind)),
            })
        }
    }

    /// Edge event types.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub enum EdgeKind {
        /// Rising edge event.
        Rising,
        /// Falling edge event.
        Falling,
    }

    impl EdgeKind {
        pub(crate) fn new(kind: u32) -> Result<Self> {
            Ok(match kind {
                gpiod::GPIOD_EDGE_EVENT_RISING_EDGE => EdgeKind::Rising,
                gpiod::GPIOD_EDGE_EVENT_FALLING_EDGE => EdgeKind::Falling,
                _ => return Err(Error::InvalidEnumValue("EdgeEvent", kind)),
            })
        }
    }
}

/// Various libgpiod-related functions.

/// Check if the file pointed to by path is a GPIO chip character device.
///
/// Returns true if the file exists and is a GPIO chip character device or a
/// symbolic link to it.
pub fn is_gpiochip_device<P: AsRef<Path>>(path: &P) -> bool {
    // Null-terminate the string
    let path = path.as_ref().to_string_lossy() + "\0";

    // SAFETY: libgpiod won't access the path reference once the call returns.
    unsafe { gpiod::gpiod_is_gpiochip_device(path.as_ptr() as *const c_char) }
}

/// GPIO devices.
///
/// Returns a vector of unique available GPIO Chips.
///
/// The chips are sorted in ascending order of the chip names.
pub fn gpiochip_devices<P: AsRef<Path>>(path: &P) -> Result<Vec<chip::Chip>> {
    let mut devices = Vec::new();

    for entry in fs::read_dir(path).map_err(|_| Error::IoError)?.flatten() {
        let path = entry.path();

        if is_gpiochip_device(&path) {
            let chip = chip::Chip::open(&path)?;
            let info = chip.info()?;

            devices.push((chip, info));
        }
    }

    devices.sort_by(|a, b| a.1.partial_cmp(&b.1).unwrap());
    devices.dedup_by(|a, b| a.1.eq(&b.1));

    Ok(devices.into_iter().map(|a| a.0).collect())
}

/// Get the API version of the libgpiod library as a human-readable string.
pub fn libgpiod_version() -> Result<&'static str> {
    // SAFETY: The string returned by libgpiod is guaranteed to live forever.
    let version = unsafe { gpiod::gpiod_version_string() };

    if version.is_null() {
        return Err(Error::NullString("GPIO library version"));
    }

    // SAFETY: The string is guaranteed to be valid here by the C API.
    unsafe { CStr::from_ptr(version) }
        .to_str()
        .map_err(Error::StringNotUtf8)
}

/// Get the API version of the libgpiod crate as a human-readable string.
pub fn crate_version() -> &'static str {
    env!("CARGO_PKG_VERSION")
}
