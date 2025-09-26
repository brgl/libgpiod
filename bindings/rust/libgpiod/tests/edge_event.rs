// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod edge_event {
    use std::time::Duration;

    use crate::common::*;
    use gpiosim_sys::{Pull, Sim};
    use libgpiod::{
        line::{Edge, EdgeKind, Offset},
        request,
    };

    const NGPIO: usize = 8;

    mod buffer_capacity {
        use super::*;

        #[test]
        fn default_capacity() {
            assert_eq!(request::Buffer::new(0).unwrap().capacity(), 64);
        }

        #[test]
        fn user_defined_capacity() {
            assert_eq!(request::Buffer::new(123).unwrap().capacity(), 123);
        }

        #[test]
        fn max_capacity() {
            assert_eq!(request::Buffer::new(1024 * 2).unwrap().capacity(), 1024);
        }
    }

    mod trigger {
        use super::*;
        use std::{
            sync::{Arc, Mutex},
            thread,
        };

        // Helpers to generate events
        fn trigger_falling_and_rising_edge(sim: Arc<Mutex<Sim>>, offset: Offset) {
            thread::spawn(move || {
                thread::sleep(Duration::from_millis(30));
                sim.lock().unwrap().set_pull(offset, Pull::Up).unwrap();

                thread::sleep(Duration::from_millis(30));
                sim.lock().unwrap().set_pull(offset, Pull::Down).unwrap();
            });
        }

        fn trigger_rising_edge_events_on_two_offsets(sim: Arc<Mutex<Sim>>, offset: [Offset; 2]) {
            thread::spawn(move || {
                thread::sleep(Duration::from_millis(30));
                sim.lock().unwrap().set_pull(offset[0], Pull::Up).unwrap();

                thread::sleep(Duration::from_millis(30));
                sim.lock().unwrap().set_pull(offset[1], Pull::Up).unwrap();
            });
        }

        fn trigger_multiple_events(sim: Arc<Mutex<Sim>>, offset: Offset) {
            sim.lock().unwrap().set_pull(offset, Pull::Up).unwrap();
            thread::sleep(Duration::from_millis(10));

            sim.lock().unwrap().set_pull(offset, Pull::Down).unwrap();
            thread::sleep(Duration::from_millis(10));

            sim.lock().unwrap().set_pull(offset, Pull::Up).unwrap();
            thread::sleep(Duration::from_millis(10));
        }

        #[test]
        fn both_edges() {
            const GPIO: Offset = 2;
            let mut buf = request::Buffer::new(0).unwrap();
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Both));
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            // Generate events
            trigger_falling_and_rising_edge(config.sim(), GPIO);

            // Rising event
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            let ts_rising = event.timestamp();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Rising);
            assert_eq!(event.line_offset(), GPIO);

            // Falling event
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            let ts_falling = event.timestamp();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Falling);
            assert_eq!(event.line_offset(), GPIO);

            // No events available
            assert!(
                !config
                    .request()
                    .wait_edge_events(Some(Duration::from_millis(100)))
                    .unwrap()
            );

            assert!(ts_falling > ts_rising);
        }

        #[test]
        fn rising_edge() {
            const GPIO: Offset = 6;
            let mut buf = request::Buffer::new(0).unwrap();
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Rising));
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            // Generate events
            trigger_falling_and_rising_edge(config.sim(), GPIO);

            // Rising event
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Rising);
            assert_eq!(event.line_offset(), GPIO);

            // No events available
            assert!(
                !config
                    .request()
                    .wait_edge_events(Some(Duration::from_millis(100)))
                    .unwrap()
            );
        }

        #[test]
        fn falling_edge() {
            const GPIO: Offset = 7;
            let mut buf = request::Buffer::new(0).unwrap();
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Falling));
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            // Generate events
            trigger_falling_and_rising_edge(config.sim(), GPIO);

            // Falling event
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Falling);
            assert_eq!(event.line_offset(), GPIO);

            // No events available
            assert!(
                !config
                    .request()
                    .wait_edge_events(Some(Duration::from_millis(100)))
                    .unwrap()
            );
        }

        #[test]
        fn edge_sequence() {
            const GPIO: [u32; 2] = [0, 1];
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Both));
            config.lconfig_add_settings(&GPIO);
            config.request_lines().unwrap();

            // Generate events
            trigger_rising_edge_events_on_two_offsets(config.sim(), GPIO);

            // Rising event GPIO 0
            let mut buf = request::Buffer::new(0).unwrap();
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Rising);
            assert_eq!(event.line_offset(), GPIO[0]);
            assert_eq!(event.global_seqno(), 1);
            assert_eq!(event.line_seqno(), 1);

            // Rising event GPIO 1
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let mut events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 1);

            let event = events.next().unwrap().unwrap();
            assert_eq!(event.event_type().unwrap(), EdgeKind::Rising);
            assert_eq!(event.line_offset(), GPIO[1]);
            assert_eq!(event.global_seqno(), 2);
            assert_eq!(event.line_seqno(), 1);

            // No events available
            assert!(
                !config
                    .request()
                    .wait_edge_events(Some(Duration::from_millis(100)))
                    .unwrap()
            );
        }

        #[test]
        fn multiple_events() {
            const GPIO: Offset = 1;
            let mut buf = request::Buffer::new(0).unwrap();
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Both));
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            // Generate events
            trigger_multiple_events(config.sim(), GPIO);

            // Read multiple events
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 3);

            let mut global_seqno = 1;
            let mut line_seqno = 1;

            // Verify sequence number of events
            for event in events {
                let event = event.unwrap();
                assert_eq!(event.line_offset(), GPIO);
                assert_eq!(event.global_seqno(), global_seqno);
                assert_eq!(event.line_seqno(), line_seqno);

                global_seqno += 1;
                line_seqno += 1;
            }
        }

        #[test]
        fn over_capacity() {
            const GPIO: Offset = 2;
            let mut buf = request::Buffer::new(2).unwrap();
            let mut config = TestConfig::new(NGPIO).unwrap();
            config.lconfig_edge(None, Some(Edge::Both));
            config.lconfig_add_settings(&[GPIO]);
            config.request_lines().unwrap();

            // Generate events
            trigger_multiple_events(config.sim(), GPIO);

            // Read multiple events
            assert!(
                config
                    .request()
                    .wait_edge_events(Some(Duration::from_secs(1)))
                    .unwrap()
            );

            let events = config.request().read_edge_events(&mut buf).unwrap();
            assert_eq!(events.len(), 2);
        }
    }
}
