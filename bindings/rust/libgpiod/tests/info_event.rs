// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod info_event {
    use libc::EINVAL;
    use std::{
        sync::{
            mpsc::{self, Receiver, Sender},
            Arc, Mutex,
        },
        thread,
        time::Duration,
    };

    use gpiosim_sys::Sim;
    use libgpiod::{
        chip::Chip,
        line::{self, Direction, InfoChangeKind, Offset},
        request, Error as ChipError, OperationType,
    };

    fn request_reconfigure_line(chip: Arc<Mutex<Chip>>, tx: Sender<()>, rx: Receiver<()>) {
        thread::spawn(move || {
            let mut lconfig1 = line::Config::new().unwrap();
            let lsettings = line::Settings::new().unwrap();
            lconfig1.add_line_settings(&[7], lsettings).unwrap();
            let rconfig = request::Config::new().unwrap();

            let mut request = chip
                .lock()
                .unwrap()
                .request_lines(Some(&rconfig), &lconfig1)
                .unwrap();

            // Signal the parent to continue
            tx.send(()).expect("Could not send signal on channel");

            // Wait for parent to signal
            rx.recv().expect("Could not receive from channel");

            let mut lconfig2 = line::Config::new().unwrap();
            let mut lsettings = line::Settings::new().unwrap();
            lsettings.set_direction(Direction::Output).unwrap();
            lconfig2.add_line_settings(&[7], lsettings).unwrap();

            request.reconfigure_lines(&lconfig2).unwrap();

            // Signal the parent to continue
            tx.send(()).expect("Could not send signal on channel");

            // Wait for parent to signal
            rx.recv().expect("Could not receive from channel");
        });
    }

    mod watch {
        use super::*;
        const NGPIO: usize = 8;
        const GPIO: Offset = 7;

        #[test]
        fn line_info() {
            let sim = Sim::new(Some(NGPIO), None, true).unwrap();
            let chip = Chip::open(&sim.dev_path()).unwrap();

            assert_eq!(
                chip.watch_line_info(NGPIO as u32).unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipWatchLineInfo, errno::Errno(EINVAL))
            );

            let info = chip.watch_line_info(GPIO).unwrap();
            assert_eq!(info.offset(), GPIO);

            // No events available
            assert!(!chip
                .wait_info_event(Some(Duration::from_millis(100)))
                .unwrap());
        }

        #[test]
        fn reconfigure() {
            let sim = Sim::new(Some(NGPIO), None, true).unwrap();
            let chip = Arc::new(Mutex::new(Chip::open(&sim.dev_path()).unwrap()));
            let info = chip.lock().unwrap().watch_line_info(GPIO).unwrap();

            assert_eq!(info.direction().unwrap(), Direction::Input);

            // Thread synchronizing mechanism
            let (tx_main, rx_thread) = mpsc::channel();
            let (tx_thread, rx_main) = mpsc::channel();

            // Generate events
            request_reconfigure_line(chip.clone(), tx_thread, rx_thread);

            // Wait for thread to signal
            rx_main.recv().expect("Could not receive from channel");

            // Line requested event
            assert!(chip
                .lock()
                .unwrap()
                .wait_info_event(Some(Duration::from_secs(1)))
                .unwrap());
            let event = chip.lock().unwrap().read_info_event().unwrap();
            let ts_req = event.timestamp();

            assert_eq!(event.event_type().unwrap(), InfoChangeKind::LineRequested);
            assert_eq!(
                event.line_info().unwrap().direction().unwrap(),
                Direction::Input
            );

            // Signal the thread to continue
            tx_main.send(()).expect("Could not send signal on channel");

            // Wait for thread to signal
            rx_main.recv().expect("Could not receive from channel");

            // Line changed event
            assert!(chip
                .lock()
                .unwrap()
                .wait_info_event(Some(Duration::from_secs(1)))
                .unwrap());
            let event = chip.lock().unwrap().read_info_event().unwrap();
            let ts_rec = event.timestamp();

            assert_eq!(
                event.event_type().unwrap(),
                InfoChangeKind::LineConfigChanged
            );
            assert_eq!(
                event.line_info().unwrap().direction().unwrap(),
                Direction::Output
            );

            // Signal the thread to continue
            tx_main.send(()).expect("Could not send signal on channel");

            // Line released event
            assert!(chip
                .lock()
                .unwrap()
                .wait_info_event(Some(Duration::from_secs(1)))
                .unwrap());
            let event = chip.lock().unwrap().read_info_event().unwrap();
            let ts_rel = event.timestamp();

            assert_eq!(event.event_type().unwrap(), InfoChangeKind::LineReleased);

            // No events available
            assert!(!chip
                .lock()
                .unwrap()
                .wait_info_event(Some(Duration::from_millis(100)))
                .unwrap());

            // Check timestamps are really monotonic.
            assert!(ts_rel > ts_rec);
            assert!(ts_rec > ts_req);
        }
    }
}
