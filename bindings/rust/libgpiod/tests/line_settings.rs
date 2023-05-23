// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod line_settings {
    use std::time::Duration;

    use libgpiod::line::{
        self, Bias, Direction, Drive, Edge, EventClock, SettingKind, SettingVal, Value,
    };

    #[test]
    fn direction() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Direction).unwrap(),
            SettingVal::Direction(Direction::AsIs)
        );

        lsettings.set_direction(Direction::Input).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Direction).unwrap(),
            SettingVal::Direction(Direction::Input)
        );

        lsettings.set_direction(Direction::Output).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Direction).unwrap(),
            SettingVal::Direction(Direction::Output)
        );
    }

    #[test]
    fn edge_detection() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EdgeDetection).unwrap(),
            SettingVal::EdgeDetection(None)
        );

        lsettings.set_edge_detection(Some(Edge::Both)).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EdgeDetection).unwrap(),
            SettingVal::EdgeDetection(Some(Edge::Both))
        );

        lsettings.set_edge_detection(Some(Edge::Rising)).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EdgeDetection).unwrap(),
            SettingVal::EdgeDetection(Some(Edge::Rising))
        );

        lsettings.set_edge_detection(Some(Edge::Falling)).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EdgeDetection).unwrap(),
            SettingVal::EdgeDetection(Some(Edge::Falling))
        );
    }

    #[test]
    fn bias() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Bias).unwrap(),
            SettingVal::Bias(None)
        );

        lsettings.set_bias(Some(Bias::PullDown)).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Bias).unwrap(),
            SettingVal::Bias(Some(Bias::PullDown))
        );

        lsettings.set_bias(Some(Bias::PullUp)).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Bias).unwrap(),
            SettingVal::Bias(Some(Bias::PullUp))
        );
    }

    #[test]
    fn drive() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Drive).unwrap(),
            SettingVal::Drive(Drive::PushPull)
        );

        lsettings.set_drive(Drive::PushPull).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Drive).unwrap(),
            SettingVal::Drive(Drive::PushPull)
        );

        lsettings.set_drive(Drive::OpenDrain).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Drive).unwrap(),
            SettingVal::Drive(Drive::OpenDrain)
        );

        lsettings.set_drive(Drive::OpenSource).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::Drive).unwrap(),
            SettingVal::Drive(Drive::OpenSource)
        );
    }

    #[test]
    fn active_low() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::ActiveLow).unwrap(),
            SettingVal::ActiveLow(false)
        );

        lsettings.set_active_low(true);
        assert_eq!(
            lsettings.prop(SettingKind::ActiveLow).unwrap(),
            SettingVal::ActiveLow(true)
        );

        lsettings.set_active_low(false);
        assert_eq!(
            lsettings.prop(SettingKind::ActiveLow).unwrap(),
            SettingVal::ActiveLow(false)
        );
    }

    #[test]
    fn debounce_period() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::DebouncePeriod).unwrap(),
            SettingVal::DebouncePeriod(Duration::from_millis(0))
        );

        lsettings.set_debounce_period(Duration::from_millis(5));
        assert_eq!(
            lsettings.prop(SettingKind::DebouncePeriod).unwrap(),
            SettingVal::DebouncePeriod(Duration::from_millis(5))
        );
    }

    #[test]
    fn event_clock() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::Monotonic)
        );

        lsettings.set_event_clock(EventClock::Realtime).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::Realtime)
        );

        lsettings.set_event_clock(EventClock::Monotonic).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::Monotonic)
        );
    }

    #[test]
    #[ignore]
    fn event_clock_hte() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::Monotonic)
        );

        lsettings.set_event_clock(EventClock::HTE).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::EventClock).unwrap(),
            SettingVal::EventClock(EventClock::HTE)
        );
    }

    #[test]
    fn output_value() {
        let mut lsettings = line::Settings::new().unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::OutputValue).unwrap(),
            SettingVal::OutputValue(Value::InActive)
        );

        lsettings.set_output_value(Value::Active).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::OutputValue).unwrap(),
            SettingVal::OutputValue(Value::Active)
        );

        lsettings.set_output_value(Value::InActive).unwrap();
        assert_eq!(
            lsettings.prop(SettingKind::OutputValue).unwrap(),
            SettingVal::OutputValue(Value::InActive)
        );
    }
}
