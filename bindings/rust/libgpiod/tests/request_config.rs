// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod request_config {
    use libgpiod::{Error as ChipError, OperationType, request};

    mod verify {
        use super::*;

        #[test]
        fn default() {
            let rconfig = request::Config::new().unwrap();

            assert_eq!(rconfig.event_buffer_size(), 0);
            assert_eq!(
                rconfig.consumer().unwrap_err(),
                ChipError::OperationFailed(
                    OperationType::RequestConfigGetConsumer,
                    errno::Errno(0),
                )
            );
        }

        #[test]
        fn initialized() {
            const CONSUMER: &str = "foobar";
            let mut rconfig = request::Config::new().unwrap();
            rconfig.set_consumer(CONSUMER).unwrap();
            rconfig.set_event_buffer_size(64);

            assert_eq!(rconfig.event_buffer_size(), 64);
            assert_eq!(rconfig.consumer().unwrap(), CONSUMER);
        }
    }
}
