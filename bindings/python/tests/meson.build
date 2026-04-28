# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

subdir('gpiosim')
subdir('system')

fs = import('fs')

foreach f : [
  '__init__.py',
  '__main__.py',
  'helpers.py',
  'tests_chip.py',
  'tests_chip_info.py',
  'tests_edge_event.py',
  'tests_info_event.py',
  'tests_line.py',
  'tests_line_info.py',
  'tests_line_request.py',
  'tests_line_settings.py',
  'tests_module.py',
]
  fs.copyfile(f)
endforeach

test_env = environment()
test_env.set('PYTHONPATH', meson.global_build_root() / 'bindings' / 'python')
test_env.append('LD_LIBRARY_PATH', meson.global_build_root() / 'lib')
test_env.append('LD_LIBRARY_PATH', meson.global_build_root() / 'tests' / 'gpiosim')

test('python-tests',
  py,
  args: ['-B', '-m', 'tests'],
  env: test_env,
  is_parallel: false,
)
