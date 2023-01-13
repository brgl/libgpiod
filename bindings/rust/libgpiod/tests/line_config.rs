// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod line_config {
    use libgpiod::chip::Chip;
    use libgpiod::line::{
        self, Bias, Direction, Drive, Edge, EventClock, SettingKind, SettingVal, Value,
    };
    use gpiosim_sys::Sim;

    #[test]
    fn settings() {
        let mut lsettings1 = line::Settings::new().unwrap();
        lsettings1
            .set_direction(Direction::Input)
            .unwrap()
            .set_edge_detection(Some(Edge::Both))
            .unwrap()
            .set_bias(Some(Bias::PullDown))
            .unwrap()
            .set_drive(Drive::PushPull)
            .unwrap();

        let mut lsettings2 = line::Settings::new().unwrap();
        lsettings2
            .set_direction(Direction::Output)
            .unwrap()
            .set_active_low(true)
            .set_event_clock(EventClock::Realtime)
            .unwrap()
            .set_output_value(Value::Active)
            .unwrap();

        // Add settings for multiple lines
        let mut lconfig = line::Config::new().unwrap();
        lconfig.add_line_settings(&[0, 1, 2], lsettings1).unwrap();
        lconfig.add_line_settings(&[4, 5], lsettings2).unwrap();

        let settings_map = lconfig.line_settings().unwrap();

        // Retrieve settings
        let lsettings = settings_map.get(1).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Direction).unwrap(),
            SettingVal::Direction(Direction::Input)
        );
        assert_eq!(
            lsettings.prop(SettingKind::EdgeDetection).unwrap(),
            SettingVal::EdgeDetection(Some(Edge::Both))
        );
        assert_eq!(
            lsettings.prop(SettingKind::Bias).unwrap(),
            SettingVal::Bias(Some(Bias::PullDown))
        );
        assert_eq!(
            lsettings.prop(SettingKind::Drive).unwrap(),
            SettingVal::Drive(Drive::PushPull)
        );

        let lsettings = settings_map.get(5).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Direction).unwrap(),
            SettingVal::Direction(Direction::Output)
        );
        assert_eq!(
            lsettings.prop(SettingKind::ActiveLow).unwrap(),
            SettingVal::ActiveLow(true)
        );
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::Realtime)
        );
        assert_eq!(
            lsettings.prop(SettingKind::OutputValue).unwrap(),
            SettingVal::OutputValue(Value::Active)
        );
    }

    #[test]
    fn set_global_output_values() {
        let sim = Sim::new(Some(4), None, true).unwrap();
        let mut settings = line::Settings::new().unwrap();
        settings.set_direction(Direction::Output).unwrap();

        let mut config = line::Config::new().unwrap();
        config
            .add_line_settings(&[0, 1, 2, 3], settings)
            .unwrap()
            .set_output_values(&[
                Value::Active,
                Value::InActive,
                Value::Active,
                Value::InActive
            ])
            .unwrap();

        let chip = Chip::open(&sim.dev_path()).unwrap();
        let _request = chip.request_lines(None, &config);

        assert_eq!(sim.val(0).unwrap(), gpiosim_sys::Value::Active);
        assert_eq!(sim.val(1).unwrap(), gpiosim_sys::Value::InActive);
        assert_eq!(sim.val(2).unwrap(), gpiosim_sys::Value::Active);
        assert_eq!(sim.val(3).unwrap(), gpiosim_sys::Value::InActive);
    }

    #[test]
    fn read_back_global_output_values() {
        let mut settings = line::Settings::new().unwrap();
        settings
            .set_direction(Direction::Output)
            .unwrap()
            .set_output_value(Value::Active)
            .unwrap();

        let mut config = line::Config::new().unwrap();
        config
            .add_line_settings(&[0, 1, 2, 3], settings)
            .unwrap()
            .set_output_values(&[
                Value::Active,
                Value::InActive,
                Value::Active,
                Value::InActive,
            ])
            .unwrap();

        assert_eq!(
            config
                .line_settings()
                .unwrap()
                .get(1)
                .unwrap()
                .output_value()
                .unwrap(),
            Value::InActive
        );
    }
}
