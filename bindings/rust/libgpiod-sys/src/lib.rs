// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

#[allow(non_camel_case_types, non_upper_case_globals)]
#[cfg_attr(test, allow(deref_nullptr, non_snake_case))]

mod bindings_raw {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
pub use bindings_raw::*;
