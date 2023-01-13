// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

use std::sync::{Arc, Mutex};
use std::time::Duration;

use gpiosim_sys::{Pull, Sim, Value as SimValue};
use libgpiod::{
    chip::Chip,
    line::{self, Bias, Direction, Drive, Edge, EventClock, Offset, SettingVal, Value},
    request, Result,
};

pub(crate) struct TestConfig {
    sim: Arc<Mutex<Sim>>,
    chip: Option<Chip>,
    request: Option<request::Request>,
    rconfig: request::Config,
    lconfig: line::Config,
    lsettings: Option<line::Settings>,
}

impl TestConfig {
    pub(crate) fn new(ngpio: usize) -> Result<Self> {
        Ok(Self {
            sim: Arc::new(Mutex::new(Sim::new(Some(ngpio), None, true)?)),
            chip: None,
            request: None,
            rconfig: request::Config::new().unwrap(),
            lconfig: line::Config::new().unwrap(),
            lsettings: Some(line::Settings::new().unwrap()),
        })
    }

    pub(crate) fn set_pull(&self, offsets: &[Offset], pulls: &[Pull]) {
        for i in 0..pulls.len() {
            self.sim
                .lock()
                .unwrap()
                .set_pull(offsets[i], pulls[i])
                .unwrap();
        }
    }

    pub(crate) fn rconfig_set_consumer(&mut self, consumer: &str) {
        self.rconfig.set_consumer(consumer).unwrap();
    }

    pub(crate) fn lconfig_val(&mut self, dir: Option<Direction>, val: Option<Value>) {
        let mut settings = Vec::new();

        if let Some(dir) = dir {
            settings.push(SettingVal::Direction(dir));
        }

        if let Some(val) = val {
            settings.push(SettingVal::OutputValue(val));
        }

        if !settings.is_empty() {
            self.lsettings().set_prop(&settings).unwrap();
        }
    }

    pub(crate) fn lconfig_bias(&mut self, dir: Direction, bias: Option<Bias>) {
        let settings = vec![SettingVal::Direction(dir), SettingVal::Bias(bias)];
        self.lsettings().set_prop(&settings).unwrap();
    }

    pub(crate) fn lconfig_clock(&mut self, clock: EventClock) {
        let settings = vec![SettingVal::EventClock(clock)];
        self.lsettings().set_prop(&settings).unwrap();
    }

    pub(crate) fn lconfig_debounce(&mut self, duration: Duration) {
        let settings = vec![
            SettingVal::Direction(Direction::Input),
            SettingVal::DebouncePeriod(duration),
        ];
        self.lsettings().set_prop(&settings).unwrap();
    }

    pub(crate) fn lconfig_drive(&mut self, dir: Direction, drive: Drive) {
        let settings = vec![SettingVal::Direction(dir), SettingVal::Drive(drive)];
        self.lsettings().set_prop(&settings).unwrap();
    }

    pub(crate) fn lconfig_edge(&mut self, dir: Option<Direction>, edge: Option<Edge>) {
        let mut settings = Vec::new();

        if let Some(dir) = dir {
            settings.push(SettingVal::Direction(dir));
        }

        settings.push(SettingVal::EdgeDetection(edge));
        self.lsettings().set_prop(&settings).unwrap();
    }

    pub(crate) fn lconfig_add_settings(&mut self, offsets: &[Offset]) {
        self.lconfig
            .add_line_settings(offsets, self.lsettings.take().unwrap())
            .unwrap();
    }

    pub(crate) fn request_lines(&mut self) -> Result<()> {
        let chip = Chip::open(&self.sim.lock().unwrap().dev_path())?;

        self.request = Some(chip.request_lines(Some(&self.rconfig), &self.lconfig)?);
        self.chip = Some(chip);

        Ok(())
    }

    pub(crate) fn sim(&self) -> Arc<Mutex<Sim>> {
        self.sim.clone()
    }

    pub(crate) fn sim_val(&self, offset: Offset) -> Result<SimValue> {
        self.sim.lock().unwrap().val(offset)
    }

    pub(crate) fn chip(&self) -> &Chip {
        self.chip.as_ref().unwrap()
    }

    pub(crate) fn lsettings(&mut self) -> &mut line::Settings {
        self.lsettings.as_mut().unwrap()
    }

    pub(crate) fn request(&mut self) -> &mut request::Request {
        self.request.as_mut().unwrap()
    }
}

impl Drop for TestConfig {
    fn drop(&mut self) {
        // Explicit freeing is important to make sure "request" get freed
        // before "sim" and "chip".
        self.request = None;
    }
}
