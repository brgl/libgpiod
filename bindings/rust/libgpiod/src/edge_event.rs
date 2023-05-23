// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::time::Duration;

use super::{
    gpiod,
    line::{EdgeKind, Offset},
    Error, OperationType, Result,
};

/// Line edge events handling
///
/// An edge event object contains information about a single line edge event.
/// It contains the event type, timestamp and the offset of the line on which
/// the event occurred as well as two sequence numbers (global for all lines
/// in the associated request and local for this line only).
///
/// Edge events are stored into an edge-event buffer object to improve
/// performance and to limit the number of memory allocations when a large
/// number of events are being read.

#[derive(Debug, Eq, PartialEq)]
pub struct Event(*mut gpiod::gpiod_edge_event);

impl Event {
    pub fn event_clone(event: &Event) -> Result<Event> {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        let event = unsafe { gpiod::gpiod_edge_event_copy(event.0) };
        if event.is_null() {
            return Err(Error::OperationFailed(
                OperationType::EdgeEventCopy,
                errno::errno(),
            ));
        }

        Ok(Self(event))
    }

    /// Get the event type.
    pub fn event_type(&self) -> Result<EdgeKind> {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        EdgeKind::new(unsafe { gpiod::gpiod_edge_event_get_event_type(self.0) } as u32)
    }

    /// Get the timestamp of the event.
    pub fn timestamp(&self) -> Duration {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        Duration::from_nanos(unsafe { gpiod::gpiod_edge_event_get_timestamp_ns(self.0) })
    }

    /// Get the offset of the line on which the event was triggered.
    pub fn line_offset(&self) -> Offset {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_edge_event_get_line_offset(self.0) }
    }

    /// Get the global sequence number of the event.
    ///
    /// Returns sequence number of the event relative to all lines in the
    /// associated line request.
    pub fn global_seqno(&self) -> usize {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_edge_event_get_global_seqno(self.0)
                .try_into()
                .unwrap()
        }
    }

    /// Get the event sequence number specific to concerned line.
    ///
    /// Returns sequence number of the event relative to the line within the
    /// lifetime of the associated line request.
    pub fn line_seqno(&self) -> usize {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        unsafe {
            gpiod::gpiod_edge_event_get_line_seqno(self.0)
                .try_into()
                .unwrap()
        }
    }
}

impl Drop for Event {
    /// Free the edge event.
    fn drop(&mut self) {
        // SAFETY: `gpiod_edge_event` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_edge_event_free(self.0) };
    }
}
