// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod line_request {
    use libc::{E2BIG, EINVAL};
    use std::time::Duration;

    use crate::common::*;
    use gpiosim_sys::{Pull, Value as SimValue};
    use libgpiod::{
        line::{
            self, Bias, Direction, Drive, Edge, EventClock, Offset, SettingVal, Value, ValueMap,
        },
        Error as ChipError, OperationType,
    };

    const NGPIO: usize = 8;

    mod invalid_arguments {
        use super::*;

        #[test]
        fn no_offsets() {
            let mut config = TestConfig::new(NGPIO).unwrap();

            assert_eq!(
                config.request_lines().unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipRequestLines, errno::Errno(EINVAL))
            );
        }

        #[test]
        fn out_of_bound_offsets() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[2, 0, 8, 4]);

            assert_eq!(
                config.request_lines().unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipRequestLines, errno::Errno(EINVAL))
            );
        }

        #[test]
        fn dir_out_edge_failure() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(Some(Direction::Output), Some(Edge::Both));
            config.lconfig_add_settings(&[0]);

            assert_eq!(
                config.request_lines().unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipRequestLines, errno::Errno(EINVAL))
            );
        }
    }

    mod verify {
        use super::*;

        #[test]
        #[cfg(feature = "v2_1")]
        fn chip_name() {
            const GPIO: Offset = 2;
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            let arc = config.sim();
            let sim = arc.lock().unwrap();
            let chip_name = sim.chip_name();

            assert_eq!(config.request().chip_name().unwrap(), chip_name);
        }

        #[test]
        fn custom_consumer() {
            const GPIO: Offset = 2;
            const CONSUMER: &str = "foobar";
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.rconfig_set_consumer(CONSUMER);
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            let info = config.chip().line_info(GPIO).unwrap();

            assert!(info.is_used());
            assert_eq!(info.consumer().unwrap(), CONSUMER);
        }

        #[test]
        fn empty_consumer() {
            const GPIO: Offset = 2;
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            let info = config.chip().line_info(GPIO).unwrap();

            assert!(info.is_used());
            assert_eq!(info.consumer().unwrap(), "?");
        }

        #[test]
        fn read_values() {
            let offsets = [7, 1, 0, 6, 2];
            let pulls = [Pull::Up, Pull::Up, Pull::Down, Pull::Up, Pull::Down];
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.set_pull(&offsets, &pulls);
            config.lconfig_val(Some(Direction::Input), None);
            config.lconfig_add_settings(&offsets);
            config.request_lines().unwrap();

            let request = config.request();

            // Single values read properly
            assert_eq!(request.value(7).unwrap(), Value::Active);

            // Values read properly
            let map = request.values().unwrap();
            for i in 0..offsets.len() {
                assert_eq!(
                    *map.get(offsets[i].into()).unwrap(),
                    match pulls[i] {
                        Pull::Down => Value::InActive,
                        _ => Value::Active,
                    }
                );
            }

            // Subset of values read properly
            let map = request.values_subset(&[2, 0, 6]).unwrap();
            assert_eq!(*map.get(2).unwrap(), Value::InActive);
            assert_eq!(*map.get(0).unwrap(), Value::InActive);
            assert_eq!(*map.get(6).unwrap(), Value::Active);

            // Value read properly after reconfigure
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_active_low(true);
            let mut lconfig = line::Config::new().unwrap();
            lconfig.add_line_settings(&offsets, lsettings).unwrap();
            request.reconfigure_lines(&lconfig).unwrap();
            assert_eq!(request.value(7).unwrap(), Value::InActive);
        }

        #[test]
        fn set_output_values() {
            let offsets = [0, 1, 3, 4];
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_val(Some(Direction::Output), Some(Value::Active));
            config.lconfig_add_settings(&offsets);
            config.request_lines().unwrap();

            assert_eq!(config.sim_val(0).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(1).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(3).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::Active);

            // Default
            assert_eq!(config.sim_val(2).unwrap(), SimValue::InActive);
        }

        #[test]
        fn update_output_values() {
            let offsets = [0, 1, 3, 4];
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_val(Some(Direction::Output), Some(Value::InActive));
            config.lconfig_add_settings(&offsets);
            config.request_lines().unwrap();

            // Set single value
            config.request().set_value(1, Value::Active).unwrap();
            assert_eq!(config.sim_val(0).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(1).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(3).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::InActive);
            config.request().set_value(1, Value::InActive).unwrap();
            assert_eq!(config.sim_val(1).unwrap(), SimValue::InActive);

            // Set values of subset
            let mut map = ValueMap::new();
            map.insert(4, Value::Active);
            map.insert(3, Value::Active);
            config.request().set_values_subset(map).unwrap();
            assert_eq!(config.sim_val(0).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(1).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(3).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::Active);

            let mut map = ValueMap::new();
            map.insert(4, Value::InActive);
            map.insert(3, Value::InActive);
            config.request().set_values_subset(map).unwrap();
            assert_eq!(config.sim_val(3).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::InActive);

            // Set all values
            config
                .request()
                .set_values(&[
                    Value::Active,
                    Value::InActive,
                    Value::Active,
                    Value::InActive,
                ])
                .unwrap();
            assert_eq!(config.sim_val(0).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(1).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(3).unwrap(), SimValue::Active);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::InActive);
            config
                .request()
                .set_values(&[
                    Value::InActive,
                    Value::InActive,
                    Value::InActive,
                    Value::InActive,
                ])
                .unwrap();
            assert_eq!(config.sim_val(0).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(1).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(3).unwrap(), SimValue::InActive);
            assert_eq!(config.sim_val(4).unwrap(), SimValue::InActive);
        }

        #[test]
        fn set_bias() {
            let offsets = [3];
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_bias(Direction::Input, Some(Bias::PullUp));
            config.lconfig_add_settings(&offsets);
            config.request_lines().unwrap();
            config.request();

            // Set single value
            assert_eq!(config.sim_val(3).unwrap(), SimValue::Active);
        }

        #[test]
        fn no_events() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Both));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();

            // No events available
            assert!(!config
                .request()
                .wait_edge_events(Some(Duration::from_millis(100)))
                .unwrap());
        }
    }

    mod reconfigure {
        use super::*;

        #[test]
        fn e2big() {
            let offsets = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
            let mut config = TestConfig::new(16).unwrap();
            config.lconfig_add_settings(&offsets);
            config.request_lines().unwrap();

            let request = config.request();

            // Reconfigure
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_direction(Direction::Input).unwrap();
            let mut lconfig = line::Config::new().unwrap();

            // The uAPI config has only 10 attribute slots, this should pass.
            for offset in offsets {
                lsettings.set_debounce_period(Duration::from_millis((100 + offset).into()));
                lconfig
                    .add_line_settings(&[offset as Offset], lsettings.try_clone().unwrap())
                    .unwrap();
            }

            assert!(request.reconfigure_lines(&lconfig).is_ok());

            // The uAPI config has only 10 attribute slots, and this is the 11th entry.
            // This should fail with E2BIG.
            lsettings.set_debounce_period(Duration::from_millis(100 + 11));
            lconfig.add_line_settings(&[11], lsettings).unwrap();

            assert_eq!(
                request.reconfigure_lines(&lconfig).unwrap_err(),
                ChipError::OperationFailed(
                    OperationType::LineRequestReconfigLines,
                    errno::Errno(E2BIG),
                )
            );
        }

        #[test]
        fn bias() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), None);

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::Bias(Some(Bias::PullUp)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), Some(Bias::PullUp));

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::Bias(Some(Bias::PullDown)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), Some(Bias::PullDown));

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::Bias(Some(Bias::Disabled)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), Some(Bias::Disabled));
        }

        #[test]
        fn drive() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::PushPull);

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::Drive(Drive::PushPull),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::PushPull);

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Output),
                    SettingVal::Drive(Drive::OpenDrain),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::OpenDrain);

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Output),
                    SettingVal::Drive(Drive::OpenSource),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::OpenSource);
        }

        #[test]
        fn edge() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), None);

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::EdgeDetection(Some(Edge::Both)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), Some(Edge::Both));

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::EdgeDetection(Some(Edge::Rising)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), Some(Edge::Rising));

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::EdgeDetection(Some(Edge::Falling)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), Some(Edge::Falling));
        }

        #[test]
        fn event_clock() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Monotonic);

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_event_clock(EventClock::Monotonic).unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Monotonic);

            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_event_clock(EventClock::Realtime).unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            config.request().reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Realtime);
        }

        #[test]
        #[ignore]
        fn event_clock_hte() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Monotonic);

            let request = config.request();

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_event_clock(EventClock::HTE).unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            request.reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::HTE);
        }

        #[test]
        fn debounce() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert!(!info.is_debounced());
            assert_eq!(info.debounce_period(), Duration::from_millis(0));

            let request = config.request();

            // Reconfigure
            let mut lconfig = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings
                .set_prop(&[
                    SettingVal::Direction(Direction::Input),
                    SettingVal::DebouncePeriod(Duration::from_millis(100)),
                ])
                .unwrap();
            lconfig.add_line_settings(&[0], lsettings).unwrap();
            request.reconfigure_lines(&lconfig).unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert!(info.is_debounced());
            assert_eq!(info.debounce_period(), Duration::from_millis(100));
        }
    }
}
