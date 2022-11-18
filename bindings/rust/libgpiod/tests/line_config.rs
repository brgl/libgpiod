// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod line_config {
    use libgpiod::line::{
        self, Bias, Direction, Drive, Edge, EventClock, SettingKind, SettingVal, Value,
    };

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
        let lconfig = line::Config::new().unwrap();
        lconfig.add_line_settings(&[0, 1, 2], lsettings1).unwrap();
        lconfig.add_line_settings(&[4, 5], lsettings2).unwrap();

        // Retrieve settings
        let lsettings = lconfig.line_settings(1).unwrap();
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

        let lsettings = lconfig.line_settings(5).unwrap();
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
    fn offsets() {
        let mut lsettings1 = line::Settings::new().unwrap();
        lsettings1.set_direction(Direction::Input).unwrap();

        let mut lsettings2 = line::Settings::new().unwrap();
        lsettings2.set_event_clock(EventClock::Realtime).unwrap();

        // Add settings for multiple lines
        let lconfig = line::Config::new().unwrap();
        lconfig.add_line_settings(&[0, 1, 2], lsettings1).unwrap();
        lconfig.add_line_settings(&[4, 5], lsettings2).unwrap();

        // Verify offsets
        let offsets = lconfig.offsets().unwrap();
        assert_eq!(offsets, [0, 1, 2, 4, 5]);
    }
}
