// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::ptr;

use super::{
    gpiod,
    request::{Event, Request},
    Error, OperationType, Result,
};

/// Line edge events
///
/// An iterator over the elements of type `Event`.

pub struct Events<'a> {
    buffer: &'a mut Buffer,
    read_index: usize,
    len: usize,
}

impl<'a> Events<'a> {
    pub fn new(buffer: &'a mut Buffer, len: usize) -> Self {
        Self {
            buffer,
            read_index: 0,
            len,
        }
    }

    /// Get the number of contained events in the snapshot, this doesn't change
    /// on reading events from the iterator.
    pub fn len(&self) -> usize {
        self.len
    }

    /// Check if buffer is empty.
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl<'a> Iterator for Events<'a> {
    type Item = Result<&'a Event>;

    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        if self.read_index + n >= self.len {
            return None;
        }

        self.read_index += n + 1;
        Some(self.buffer.event(self.read_index - 1))
    }

    fn next(&mut self) -> Option<Self::Item> {
        // clippy false-positive, fixed in next clippy release:
        // https://github.com/rust-lang/rust-clippy/issues/9820
        #[allow(clippy::iter_nth_zero)]
        self.nth(0)
    }
}

/// Line edge events buffer
#[derive(Debug, Eq, PartialEq)]
pub struct Buffer {
    pub(crate) buffer: *mut gpiod::gpiod_edge_event_buffer,
    events: Vec<*mut gpiod::gpiod_edge_event>,
}

// SAFETY: Buffer models an owned gpiod_edge_event_buffer. However, there may
// be events tied to it. Concurrent access from multiple threads to a buffer
// and its associated events is not allowed by the C lib.
// In Rust, those events will always be borrowed from a buffer instance. Thus,
// either Rust prevents the user to move the Buffer while there are still
// borrowed events, or we can safely send the the Buffer.
unsafe impl Send for Buffer {}

impl Buffer {
    /// Create a new edge event buffer.
    ///
    /// If capacity equals 0, it will be set to a default value of 64. If
    /// capacity is larger than 1024, it will be limited to 1024.
    pub fn new(capacity: usize) -> Result<Self> {
        // SAFETY: The `gpiod_edge_event_buffer` returned by libgpiod is guaranteed to live as long
        // as the `struct Buffer`.
        let buffer = unsafe { gpiod::gpiod_edge_event_buffer_new(capacity) };
        if buffer.is_null() {
            return Err(Error::OperationFailed(
                OperationType::EdgeEventBufferNew,
                errno::errno(),
            ));
        }

        // SAFETY: `gpiod_edge_event_buffer` is guaranteed to be valid here.
        let capacity = unsafe { gpiod::gpiod_edge_event_buffer_get_capacity(buffer) };

        Ok(Self {
            buffer,
            events: vec![ptr::null_mut(); capacity],
        })
    }

    /// Get the capacity of the event buffer.
    pub fn capacity(&self) -> usize {
        self.events.len()
    }

    /// Get edge events from a line request.
    ///
    /// This function will block if no event was queued for the line.
    pub fn read_edge_events(&mut self, request: &Request) -> Result<Events> {
        for i in 0..self.events.len() {
            self.events[i] = ptr::null_mut();
        }

        // SAFETY: `gpiod_line_request` is guaranteed to be valid here.
        let ret = unsafe {
            gpiod::gpiod_line_request_read_edge_events(
                request.request,
                self.buffer,
                self.events.len(),
            )
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::LineRequestReadEdgeEvent,
                errno::errno(),
            ))
        } else {
            let ret = ret as usize;

            if ret > self.events.len() {
                Err(Error::TooManyEvents(ret, self.events.len()))
            } else {
                Ok(Events::new(self, ret))
            }
        }
    }

    /// Read an event stored in the buffer.
    fn event<'a>(&mut self, index: usize) -> Result<&'a Event> {
        if self.events[index].is_null() {
            // SAFETY: The `gpiod_edge_event` returned by libgpiod is guaranteed to live as long
            // as the `struct Event`.
            let event = unsafe {
                gpiod::gpiod_edge_event_buffer_get_event(self.buffer, index.try_into().unwrap())
            };

            if event.is_null() {
                return Err(Error::OperationFailed(
                    OperationType::EdgeEventBufferGetEvent,
                    errno::errno(),
                ));
            }

            self.events[index] = event;
        }

        // SAFETY: Safe as the underlying events object won't get freed until the time the returned
        // reference is still used.
        Ok(unsafe {
            // This will not lead to `drop(event)`.
            (self.events.as_ptr().add(index) as *const Event)
                .as_ref()
                .unwrap()
        })
    }
}

impl Drop for Buffer {
    /// Free the edge event buffer and release all associated resources.
    fn drop(&mut self) {
        // SAFETY: `gpiod_edge_event_buffer` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_edge_event_buffer_free(self.buffer) };
    }
}
