// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod line_info {
    use libc::EINVAL;
    use std::time::Duration;

    use crate::common::*;
    use gpiosim_sys::{Direction as SimDirection, Sim};
    use libgpiod::{
        Error as ChipError, OperationType,
        chip::Chip,
        line::{Bias, Direction, Drive, Edge, EventClock},
    };

    const NGPIO: usize = 8;

    mod properties {
        use std::thread;

        use libgpiod::{line, request};

        use super::*;

        #[test]
        fn default() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();
            sim.set_line_name(4, "four").unwrap();
            sim.hog_line(4, "hog4", SimDirection::OutputLow).unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            let info4 = chip.line_info(4).unwrap();
            assert_eq!(info4.offset(), 4);
            assert_eq!(info4.name().unwrap(), "four");
            assert!(info4.is_used());
            assert_eq!(info4.consumer().unwrap(), "hog4");
            assert_eq!(info4.direction().unwrap(), Direction::Output);
            assert!(!info4.is_active_low());
            assert_eq!(info4.bias().unwrap(), None);
            assert_eq!(info4.drive().unwrap(), Drive::PushPull);
            assert_eq!(info4.edge_detection().unwrap(), None);
            assert_eq!(info4.event_clock().unwrap(), EventClock::Monotonic);
            assert!(!info4.is_debounced());
            assert_eq!(info4.debounce_period(), Duration::from_millis(0));

            assert_eq!(
                chip.line_info(NGPIO as u32).unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipGetLineInfo, errno::Errno(EINVAL))
            );
        }

        #[test]
        fn name_and_offset() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();

            // Line 0 has no name
            for i in 1..NGPIO {
                sim.set_line_name(i as u32, &i.to_string()).unwrap();
            }
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();
            let info = chip.line_info(0).unwrap();

            assert_eq!(info.offset(), 0);
            assert_eq!(
                info.name().unwrap_err(),
                ChipError::NullString("GPIO line's name")
            );

            for i in 1..NGPIO {
                let info = chip.line_info(i as u32).unwrap();

                assert_eq!(info.offset(), i as u32);
                assert_eq!(info.name().unwrap(), &i.to_string());
            }
        }

        #[test]
        fn is_used() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();
            sim.hog_line(0, "hog", SimDirection::OutputHigh).unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            let info = chip.line_info(0).unwrap();
            assert!(info.is_used());

            let info = chip.line_info(1).unwrap();
            assert!(!info.is_used());
        }

        #[test]
        fn consumer() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();
            sim.hog_line(0, "hog", SimDirection::OutputHigh).unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            let info = chip.line_info(0).unwrap();
            assert_eq!(info.consumer().unwrap(), "hog");

            let info = chip.line_info(1).unwrap();
            assert_eq!(
                info.consumer().unwrap_err(),
                ChipError::NullString("GPIO line's consumer name")
            );
        }

        #[test]
        fn direction() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();
            sim.hog_line(0, "hog", SimDirection::Input).unwrap();
            sim.hog_line(1, "hog", SimDirection::OutputHigh).unwrap();
            sim.hog_line(2, "hog", SimDirection::OutputLow).unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            let info = chip.line_info(0).unwrap();
            assert_eq!(info.direction().unwrap(), Direction::Input);

            let info = chip.line_info(1).unwrap();
            assert_eq!(info.direction().unwrap(), Direction::Output);

            let info = chip.line_info(2).unwrap();
            assert_eq!(info.direction().unwrap(), Direction::Output);
        }

        #[test]
        fn bias() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), None);

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_bias(Direction::Input, Some(Bias::PullUp));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), Some(Bias::PullUp));

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_bias(Direction::Input, Some(Bias::PullDown));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.bias().unwrap(), Some(Bias::PullDown));

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_bias(Direction::Input, Some(Bias::Disabled));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
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

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_drive(Direction::Input, Drive::PushPull);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::PushPull);

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_drive(Direction::Output, Drive::OpenDrain);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.drive().unwrap(), Drive::OpenDrain);

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_drive(Direction::Output, Drive::OpenSource);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
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

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(Some(Direction::Input), Some(Edge::Both));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), Some(Edge::Both));

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(Some(Direction::Input), Some(Edge::Rising));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.edge_detection().unwrap(), Some(Edge::Rising));

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(Some(Direction::Input), Some(Edge::Falling));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
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

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_clock(EventClock::Monotonic);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Monotonic);

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_clock(EventClock::Realtime);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert_eq!(info.event_clock().unwrap(), EventClock::Realtime);
        }

        #[test]
        #[ignore]
        fn event_clock_hte() {
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_clock(EventClock::HTE);
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
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

            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_debounce(Duration::from_millis(100));
            config.lconfig_add_settings(&[0]);
            config.request_lines().unwrap();
            let info = config.chip().line_info(0).unwrap();
            assert!(info.is_debounced());
            assert_eq!(info.debounce_period(), Duration::from_millis(100));
        }

        fn generate_line_event(chip: &Chip) {
            let mut line_config = line::Config::new().unwrap();
            line_config
                .add_line_settings(&[0], line::Settings::new().unwrap())
                .unwrap();

            let mut request = chip
                .request_lines(Some(&request::Config::new().unwrap()), &line_config)
                .unwrap();

            let mut new_line_config = line::Config::new().unwrap();
            let mut settings = line::Settings::new().unwrap();
            settings.set_direction(Direction::Output).unwrap();
            new_line_config.add_line_settings(&[0], settings).unwrap();
            request.reconfigure_lines(&new_line_config).unwrap();
        }

        #[test]
        fn ownership() {
            let sim = Sim::new(Some(1), None, false).unwrap();
            sim.set_line_name(0, "Test line").unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            // start watching line
            chip.watch_line_info(0).unwrap();

            generate_line_event(&chip);

            // read generated event
            let event = chip.read_info_event().unwrap();
            let info = event.line_info().unwrap();
            assert_eq!(info.name().unwrap(), "Test line");

            // clone info and move to separate thread
            let info = info.try_clone().unwrap();

            // drop the original event with the associated line_info
            drop(event);

            // assert that we can still read the name
            thread::scope(|s| {
                s.spawn(move || {
                    assert_eq!(info.name().unwrap(), "Test line");
                });
            });
        }
    }
}
