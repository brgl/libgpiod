// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::time::Duration;

use super::{
    gpiod,
    line::{self, InfoChangeKind},
    Error, OperationType, Result,
};

/// Line status watch events
///
/// Accessors for the info event objects allowing to monitor changes in GPIO
/// line state.
///
/// Callers can be notified about changes in line's state using the interfaces
/// exposed by GPIO chips. Each info event contains information about the event
/// itself (timestamp, type) as well as a snapshot of line's state in the form
/// of a line-info object.

#[derive(Debug, Eq, PartialEq)]
pub struct Event {
    pub(crate) event: *mut gpiod::gpiod_info_event,
}

// SAFETY: Event models a wrapper around an owned gpiod_info_event and may be
// safely sent to other threads.
unsafe impl Send for Event {}

impl Event {
    /// Get a single chip's line's status change event.
    pub(crate) fn new(event: *mut gpiod::gpiod_info_event) -> Self {
        Self { event }
    }

    /// Get the event type of the status change event.
    pub fn event_type(&self) -> Result<InfoChangeKind> {
        // SAFETY: `gpiod_info_event` is guaranteed to be valid here.
        InfoChangeKind::new(unsafe { gpiod::gpiod_info_event_get_event_type(self.event) })
    }

    /// Get the timestamp of the event, read from the monotonic clock.
    pub fn timestamp(&self) -> Duration {
        // SAFETY: `gpiod_info_event` is guaranteed to be valid here.
        Duration::from_nanos(unsafe { gpiod::gpiod_info_event_get_timestamp_ns(self.event) })
    }

    /// Get the line-info object associated with the event.
    pub fn line_info(&self) -> Result<line::Info> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        let info = unsafe { gpiod::gpiod_info_event_get_line_info(self.event) };

        if info.is_null() {
            return Err(Error::OperationFailed(
                OperationType::InfoEventGetLineInfo,
                errno::errno(),
            ));
        }

        line::Info::new_from_event(info)
    }
}

impl Drop for Event {
    /// Free the info event object and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_info_event` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_info_event_free(self.event) }
    }
}
