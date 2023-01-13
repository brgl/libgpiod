// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation to show handling of info events, that are
// generated from another thread.

use std::{
    env,
    sync::{
        mpsc::{self, Receiver, Sender},
        Arc, Mutex,
    },
    thread,
    time::Duration,
};

use libgpiod::{
    chip::Chip,
    line::{self, Direction, InfoChangeKind, Offset},
    request, Error, Result,
};

fn usage(name: &str) {
    println!("Usage: {} <chip> <offset>", name);
}

fn request_reconfigure_line(
    chip: Arc<Mutex<Chip>>,
    offset: Offset,
    tx: Sender<()>,
    rx: Receiver<()>,
) {
    thread::spawn(move || {
        let mut lconfig = line::Config::new().unwrap();
        let lsettings = line::Settings::new().unwrap();
        lconfig.add_line_settings(&[offset], lsettings).unwrap();
        let rconfig = request::Config::new().unwrap();

        let mut request = chip
            .lock()
            .unwrap()
            .request_lines(Some(&rconfig), &lconfig)
            .unwrap();

        // Signal the parent to continue
        tx.send(()).expect("Could not send signal on channel");

        // Wait for parent to signal
        rx.recv().expect("Could not receive from channel");

        let mut lconfig = line::Config::new().unwrap();
        let mut lsettings = line::Settings::new().unwrap();
        lsettings.set_direction(Direction::Output).unwrap();
        lconfig.add_line_settings(&[offset], lsettings).unwrap();

        request.reconfigure_lines(&lconfig).unwrap();

        // Signal the parent to continue
        tx.send(()).expect("Could not send signal on channel");

        // Wait for parent to signal
        rx.recv().expect("Could not receive from channel");
    });
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        usage(&args[0]);
        return Err(Error::InvalidArguments);
    }

    let path = format!("/dev/gpiochip{}", args[1]);
    let offset = args[2]
        .parse::<Offset>()
        .map_err(|_| Error::InvalidArguments)?;

    let chip = Arc::new(Mutex::new(Chip::open(&path)?));
    chip.lock().unwrap().watch_line_info(offset)?;

    // Thread synchronizing mechanism
    let (tx_main, rx_thread) = mpsc::channel();
    let (tx_thread, rx_main) = mpsc::channel();

    // Generate events
    request_reconfigure_line(chip.clone(), offset, tx_thread, rx_thread);

    // Wait for thread to signal
    rx_main.recv().expect("Could not receive from channel");

    // Line requested event
    assert!(chip
        .lock()
        .unwrap()
        .wait_info_event(Some(Duration::from_secs(1)))?);
    let event = chip.lock().unwrap().read_info_event()?;
    assert_eq!(event.event_type()?, InfoChangeKind::LineRequested);

    // Signal the thread to continue
    tx_main.send(()).expect("Could not send signal on channel");

    // Wait for thread to signal
    rx_main.recv().expect("Could not receive from channel");

    // Line changed event
    assert!(chip
        .lock()
        .unwrap()
        .wait_info_event(Some(Duration::from_millis(10)))?);
    let event = chip.lock().unwrap().read_info_event()?;
    assert_eq!(event.event_type()?, InfoChangeKind::LineConfigChanged);

    // Signal the thread to continue
    tx_main.send(()).expect("Could not send signal on channel");

    // Line released event
    assert!(chip
        .lock()
        .unwrap()
        .wait_info_event(Some(Duration::from_millis(10)))?);
    let event = chip.lock().unwrap().read_info_event()?;
    assert_eq!(event.event_type().unwrap(), InfoChangeKind::LineReleased);

    // No events available
    assert!(!chip
        .lock()
        .unwrap()
        .wait_info_event(Some(Duration::from_millis(100)))?);

    Ok(())
}
