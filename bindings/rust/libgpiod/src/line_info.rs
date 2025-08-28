// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::ops::Deref;
use std::str;
use std::time::Duration;
use std::{ffi::CStr, marker::PhantomData};

use super::{
    gpiod,
    line::{Bias, Direction, Drive, Edge, EventClock, Offset},
    Error, Result,
};

/// Line info reference
///
/// Exposes functions for retrieving kernel information about both requested and
/// free lines.  Line info object contains an immutable snapshot of a line's status.
///
/// The line info contains all the publicly available information about a
/// line, which does not include the line value.  The line must be requested
/// to access the line value.
///
/// [InfoRef] only abstracts a reference to a [gpiod::gpiod_line_info] instance whose lifetime is managed
/// by a different object instance. The owned counter-part of this type is [Info].
#[derive(Debug)]
#[repr(transparent)]
pub struct InfoRef {
    _info: gpiod::gpiod_line_info,
    // Avoid the automatic `Sync` implementation.
    //
    // The C lib does not allow parallel invocations of the API. We could model
    // that by restricting all wrapper functions to `&mut Info` - which would
    // ensure exclusive access. But that would make the API a bit weird...
    // So instead we just suppress the `Sync` implementation, which suppresses
    // the `Send` implementation on `&Info` - disallowing to send it to other
    // threads, making concurrent use impossible.
    _not_sync: PhantomData<*mut gpiod::gpiod_line_info>,
}

impl InfoRef {
    /// Converts a non-owning pointer to a wrapper reference of a specific
    /// lifetime
    ///
    /// No ownership will be assumed, the pointer must be free'd by the original
    /// owner.
    ///
    /// SAFETY: The pointer must point to an instance that is valid for the
    /// entire lifetime 'a. The instance must be owned by an object that is
    /// owned by the thread invoking this method. The owning object may not be
    /// moved to another thread for the entire lifetime 'a.
    pub(crate) unsafe fn from_raw<'a>(info: *mut gpiod::gpiod_line_info) -> &'a InfoRef {
        unsafe { &*(info as *mut _) }
    }

    fn as_raw_ptr(&self) -> *mut gpiod::gpiod_line_info {
        self as *const _ as *mut _
    }

    /// Clones the line info object.
    pub fn try_clone(&self) -> Result<Info> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        let copy = unsafe { gpiod::gpiod_line_info_copy(self.as_raw_ptr()) };
        if copy.is_null() {
            return Err(Error::OperationFailed(
                crate::OperationType::LineInfoCopy,
                errno::errno(),
            ));
        }

        // SAFETY: The copy succeeded, we are the owner and stop using the
        // pointer after this.
        Ok(unsafe { Info::from_raw(copy) })
    }

    /// Get the offset of the line within the GPIO chip.
    ///
    /// The offset uniquely identifies the line on the chip. The combination of the chip and offset
    /// uniquely identifies the line within the system.
    pub fn offset(&self) -> Offset {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_get_offset(self.as_raw_ptr()) }
    }

    /// Get GPIO line's name.
    pub fn name(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Info`.
        let name = unsafe { gpiod::gpiod_line_info_get_name(self.as_raw_ptr()) };
        if name.is_null() {
            return Err(Error::NullString("GPIO line's name"));
        }

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Returns True if the line is in use, false otherwise.
    ///
    /// The user space can't know exactly why a line is busy. It may have been
    /// requested by another process or hogged by the kernel. It only matters that
    /// the line is used and we can't request it.
    pub fn is_used(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_used(self.as_raw_ptr()) }
    }

    /// Get the GPIO line's consumer name.
    pub fn consumer(&self) -> Result<&str> {
        // SAFETY: The string returned by libgpiod is guaranteed to live as long
        // as the `struct Info`.
        let name = unsafe { gpiod::gpiod_line_info_get_consumer(self.as_raw_ptr()) };
        if name.is_null() {
            return Err(Error::NullString("GPIO line's consumer name"));
        }

        // SAFETY: The string is guaranteed to be valid here by the C API.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    /// Get the GPIO line's direction.
    pub fn direction(&self) -> Result<Direction> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Direction::new(unsafe { gpiod::gpiod_line_info_get_direction(self.as_raw_ptr()) })
    }

    /// Returns true if the line is "active-low", false otherwise.
    pub fn is_active_low(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_active_low(self.as_raw_ptr()) }
    }

    /// Get the GPIO line's bias setting.
    pub fn bias(&self) -> Result<Option<Bias>> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Bias::new(unsafe { gpiod::gpiod_line_info_get_bias(self.as_raw_ptr()) })
    }

    /// Get the GPIO line's drive setting.
    pub fn drive(&self) -> Result<Drive> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Drive::new(unsafe { gpiod::gpiod_line_info_get_drive(self.as_raw_ptr()) })
    }

    /// Get the current edge detection setting of the line.
    pub fn edge_detection(&self) -> Result<Option<Edge>> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Edge::new(unsafe { gpiod::gpiod_line_info_get_edge_detection(self.as_raw_ptr()) })
    }

    /// Get the current event clock setting used for edge event timestamps.
    pub fn event_clock(&self) -> Result<EventClock> {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        EventClock::new(unsafe { gpiod::gpiod_line_info_get_event_clock(self.as_raw_ptr()) })
    }

    /// Returns true if the line is debounced (either by hardware or by the
    /// kernel software debouncer), false otherwise.
    pub fn is_debounced(&self) -> bool {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_is_debounced(self.as_raw_ptr()) }
    }

    /// Get the debounce period of the line.
    pub fn debounce_period(&self) -> Duration {
        // c_ulong may be 32bit OR 64bit, clippy reports a false-positive here:
        // https://github.com/rust-lang/rust-clippy/issues/10555
        #[allow(clippy::unnecessary_cast)]
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        Duration::from_micros(unsafe {
            gpiod::gpiod_line_info_get_debounce_period_us(self.as_raw_ptr()) as u64
        })
    }
}

/// Line info
///
/// This is the owned counterpart to [InfoRef]. Due to a [Deref] implementation,
/// all functions of [InfoRef] can also be called on this type.
#[derive(Debug)]
pub struct Info {
    info: *mut gpiod::gpiod_line_info,
}

// SAFETY: Info models a owned instance whose ownership may be safely
// transferred to other threads.
unsafe impl Send for Info {}

impl Info {
    /// Converts a owned pointer into an owned instance
    ///
    /// Assumes sole ownership over a [gpiod::gpiod_line_info] instance.
    ///
    /// SAFETY: The pointer must point to an instance that is valid. After
    /// constructing an [Info] the pointer MUST NOT be used for any other
    /// purpose anymore. All interactions with the libgpiod API have to happen
    /// through this object.
    pub(crate) unsafe fn from_raw(info: *mut gpiod::gpiod_line_info) -> Info {
        Info { info }
    }
}

impl Deref for Info {
    type Target = InfoRef;

    fn deref(&self) -> &Self::Target {
        // SAFETY: The pointer is valid for the entire lifetime '0. Info is not
        // Sync. Therefore, no &Info may be held by a different thread. Hence,
        // the current thread owns it. Since we borrow with the lifetime of '0,
        // no move to a different thread can occur while a reference remains
        // being hold.
        unsafe { InfoRef::from_raw(self.info) }
    }
}

impl Drop for Info {
    fn drop(&mut self) {
        // SAFETY: `gpiod_line_info` is guaranteed to be valid here.
        unsafe { gpiod::gpiod_line_info_free(self.info) }
    }
}
