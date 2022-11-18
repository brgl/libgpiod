// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

mod common;

mod chip {
    use libc::{ENODEV, ENOENT, ENOTTY};
    use std::path::PathBuf;

    use gpiosim_sys::Sim;
    use libgpiod::{chip::Chip, Error as ChipError, OperationType};

    mod open {
        use super::*;

        #[test]
        fn nonexistent_file() {
            assert_eq!(
                Chip::open(&PathBuf::from("/dev/nonexistent")).unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipOpen, errno::Errno(ENOENT))
            );
        }

        #[test]
        fn no_dev_file() {
            assert_eq!(
                Chip::open(&PathBuf::from("/tmp")).unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipOpen, errno::Errno(ENOTTY))
            );
        }

        #[test]
        fn non_gpio_char_dev_file() {
            assert_eq!(
                Chip::open(&PathBuf::from("/dev/null")).unwrap_err(),
                ChipError::OperationFailed(OperationType::ChipOpen, errno::Errno(ENODEV))
            );
        }

        #[test]
        fn gpiosim_file() {
            let sim = Sim::new(None, None, true).unwrap();
            assert!(Chip::open(&sim.dev_path()).is_ok());
        }
    }

    mod verify {
        use super::*;
        const NGPIO: usize = 16;
        const LABEL: &str = "foobar";

        #[test]
        fn basic_helpers() {
            let sim = Sim::new(Some(NGPIO), Some(LABEL), true).unwrap();
            let chip = Chip::open(&sim.dev_path()).unwrap();
            let info = chip.info().unwrap();

            assert_eq!(info.label().unwrap(), LABEL);
            assert_eq!(info.name().unwrap(), sim.chip_name());
            assert_eq!(chip.path().unwrap(), sim.dev_path().to_str().unwrap());
            assert_eq!(info.num_lines(), NGPIO as usize);
        }

        #[test]
        fn find_line() {
            let sim = Sim::new(Some(NGPIO), None, false).unwrap();
            sim.set_line_name(0, "zero").unwrap();
            sim.set_line_name(2, "two").unwrap();
            sim.set_line_name(3, "three").unwrap();
            sim.set_line_name(5, "five").unwrap();
            sim.set_line_name(10, "ten").unwrap();
            sim.set_line_name(11, "ten").unwrap();
            sim.enable().unwrap();

            let chip = Chip::open(&sim.dev_path()).unwrap();

            // Success case
            assert_eq!(chip.line_offset_from_name("zero").unwrap(), 0);
            assert_eq!(chip.line_offset_from_name("two").unwrap(), 2);
            assert_eq!(chip.line_offset_from_name("three").unwrap(), 3);
            assert_eq!(chip.line_offset_from_name("five").unwrap(), 5);

            // Success with duplicate names, should return first entry
            assert_eq!(chip.line_offset_from_name("ten").unwrap(), 10);

            // Failure
            assert_eq!(
                chip.line_offset_from_name("nonexistent").unwrap_err(),
                ChipError::OperationFailed(
                    OperationType::ChipGetLineOffsetFromName,
                    errno::Errno(ENOENT),
                )
            );
        }
    }
}
