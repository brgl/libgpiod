// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::path::PathBuf;
use std::str;

use libgpiod::{Error, OperationType, Result, line::Offset};

use crate::*;

/// Sim Ctx
#[derive(Debug)]
struct SimCtx {
    ctx: *mut gpiosim_ctx,
}

// Safe as the pointer is guaranteed to be valid and the associated resource
// won't be freed until the object is dropped.
unsafe impl Send for SimCtx {}

impl SimCtx {
    fn new() -> Result<Self> {
        // SAFETY: `gpiosim_ctx` returned by gpiosim is guaranteed to live
        // as long as the `struct SimCtx`.
        let ctx = unsafe { gpiosim_ctx_new() };
        if ctx.is_null() {
            return Err(Error::OperationFailed(
                OperationType::SimCtxNew,
                errno::errno(),
            ));
        }

        Ok(Self { ctx })
    }
}

impl Drop for SimCtx {
    fn drop(&mut self) {
        // SAFETY: `gpiosim_ctx` is guaranteed to be valid here.
        unsafe { gpiosim_ctx_unref(self.ctx) }
    }
}

/// Sim Dev
#[derive(Debug)]
struct SimDev {
    dev: *mut gpiosim_dev,
}

// Safe as the pointer is guaranteed to be valid and the associated resource
// won't be freed until the object is dropped.
unsafe impl Send for SimDev {}

impl SimDev {
    fn new(ctx: &SimCtx) -> Result<Self> {
        // SAFETY: `gpiosim_dev` returned by gpiosim is guaranteed to live
        // as long as the `struct SimDev`.
        let dev = unsafe { gpiosim_dev_new(ctx.ctx) };
        if dev.is_null() {
            return Err(Error::OperationFailed(
                OperationType::SimDevNew,
                errno::errno(),
            ));
        }

        Ok(Self { dev })
    }

    fn enable(&self) -> Result<()> {
        // SAFETY: `gpiosim_dev` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_dev_enable(self.dev) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimDevEnable,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    fn disable(&self) -> Result<()> {
        // SAFETY: `gpiosim_dev` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_dev_disable(self.dev) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimDevDisable,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }
}

impl Drop for SimDev {
    fn drop(&mut self) {
        // SAFETY: `gpiosim_dev` is guaranteed to be valid here.
        unsafe { gpiosim_dev_unref(self.dev) }
    }
}

/// Sim Bank
#[derive(Debug)]
struct SimBank {
    bank: *mut gpiosim_bank,
}

// Safe as the pointer is guaranteed to be valid and the associated resource
// won't be freed until the object is dropped.
unsafe impl Send for SimBank {}

impl SimBank {
    fn new(dev: &SimDev) -> Result<Self> {
        // SAFETY: `gpiosim_bank` returned by gpiosim is guaranteed to live
        // as long as the `struct SimBank`.
        let bank = unsafe { gpiosim_bank_new(dev.dev) };
        if bank.is_null() {
            return Err(Error::OperationFailed(
                OperationType::SimBankNew,
                errno::errno(),
            ));
        }

        Ok(Self { bank })
    }

    fn chip_name(&self) -> Result<&str> {
        // SAFETY: The string returned by gpiosim is guaranteed to live as long
        // as the `struct SimBank`.
        let name = unsafe { gpiosim_bank_get_chip_name(self.bank) };

        // SAFETY: The string is guaranteed to be valid here.
        unsafe { CStr::from_ptr(name) }
            .to_str()
            .map_err(Error::StringNotUtf8)
    }

    fn dev_path(&self) -> Result<PathBuf> {
        // SAFETY: The string returned by gpiosim is guaranteed to live as long
        // as the `struct SimBank`.
        let path = unsafe { gpiosim_bank_get_dev_path(self.bank) };

        // SAFETY: The string is guaranteed to be valid here.
        let path = unsafe { CStr::from_ptr(path) }
            .to_str()
            .map_err(Error::StringNotUtf8)?;

        Ok(PathBuf::from(path))
    }

    fn val(&self, offset: Offset) -> Result<Value> {
        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_bank_get_value(self.bank, offset) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankGetVal,
                errno::errno(),
            ))
        } else {
            Value::new(ret)
        }
    }

    fn set_label(&self, label: &str) -> Result<()> {
        let label = CString::new(label).map_err(|_| Error::InvalidString)?;

        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_bank_set_label(self.bank, label.as_ptr() as *const c_char) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankSetLabel,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    fn set_num_lines(&self, num: usize) -> Result<()> {
        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_bank_set_num_lines(self.bank, num) };
        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankSetNumLines,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    fn set_line_name(&self, offset: Offset, name: &str) -> Result<()> {
        let name = CString::new(name).map_err(|_| Error::InvalidString)?;

        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe {
            gpiosim_bank_set_line_name(self.bank, offset, name.as_ptr() as *const c_char)
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankSetLineName,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    fn set_pull(&self, offset: Offset, pull: Pull) -> Result<()> {
        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe { gpiosim_bank_set_pull(self.bank, offset, pull.val()) };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankSetPull,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }

    fn hog_line(&self, offset: Offset, name: &str, dir: Direction) -> Result<()> {
        let name = CString::new(name).map_err(|_| Error::InvalidString)?;

        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        let ret = unsafe {
            gpiosim_bank_hog_line(self.bank, offset, name.as_ptr() as *const c_char, dir.val())
        };

        if ret == -1 {
            Err(Error::OperationFailed(
                OperationType::SimBankHogLine,
                errno::errno(),
            ))
        } else {
            Ok(())
        }
    }
}

impl Drop for SimBank {
    fn drop(&mut self) {
        // SAFETY: `gpiosim_bank` is guaranteed to be valid here.
        unsafe { gpiosim_bank_unref(self.bank) }
    }
}

/// GPIO SIM
#[derive(Debug)]
pub struct Sim {
    _ctx: SimCtx,
    dev: SimDev,
    bank: SimBank,
}

impl Sim {
    pub fn new(ngpio: Option<usize>, label: Option<&str>, enable: bool) -> Result<Self> {
        let ctx = SimCtx::new()?;
        let dev = SimDev::new(&ctx)?;
        let bank = SimBank::new(&dev)?;

        if let Some(ngpio) = ngpio {
            bank.set_num_lines(ngpio)?;
        }

        if let Some(label) = label {
            bank.set_label(label)?;
        }

        if enable {
            dev.enable()?;
        }

        Ok(Self {
            _ctx: ctx,
            dev,
            bank,
        })
    }

    pub fn chip_name(&self) -> &str {
        self.bank.chip_name().unwrap()
    }

    pub fn dev_path(&self) -> PathBuf {
        self.bank.dev_path().unwrap()
    }

    pub fn val(&self, offset: Offset) -> Result<Value> {
        self.bank.val(offset)
    }

    pub fn set_label(&self, label: &str) -> Result<()> {
        self.bank.set_label(label)
    }

    pub fn set_num_lines(&self, num: usize) -> Result<()> {
        self.bank.set_num_lines(num)
    }

    pub fn set_line_name(&self, offset: Offset, name: &str) -> Result<()> {
        self.bank.set_line_name(offset, name)
    }

    pub fn set_pull(&self, offset: Offset, pull: Pull) -> Result<()> {
        self.bank.set_pull(offset, pull)
    }

    pub fn hog_line(&self, offset: Offset, name: &str, dir: Direction) -> Result<()> {
        self.bank.hog_line(offset, name, dir)
    }

    pub fn enable(&self) -> Result<()> {
        self.dev.enable()
    }

    pub fn disable(&self) -> Result<()> {
        self.dev.disable()
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        self.disable().unwrap()
    }
}
