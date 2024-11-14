<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->
<!-- SPDX-FileCopyrightText: 2023 Phil Howard <phil@gadgetoid.com> -->

# gpiod

These are the official Python bindings for [libgpiod](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/).

The gpiod library has been vendored into this package for your convenience and
this version of gpiod is independent from your system package.

Binary wheels are not provided. The source package requires python3-dev.

## Rationale

The new character device interface guarantees all allocated resources are
freed after closing the device file descriptor and adds several new features
that are not present in the obsolete sysfs interface (like event polling,
setting/reading multiple values at once or open-source and open-drain GPIOs).

Unfortunately interacting with the linux device file can no longer be done
using only standard command-line tools. This is the reason for creating a
library encapsulating the cumbersome, ioctl-based kernel-userspace interaction
in a set of convenient functions and opaque data structures.

## Breaking Changes

As of v2.0.2 we have replaced the unofficial, pure-Python "gpiod". The official
gpiod is not backwards compatible.

You should ensure you specify at least v2.0.2 for the official API. Versions
1.5.4 and prior are the deprecated, unofficial, pure-Python bindings.

## Installing

You will need `python3-dev`, on Debian/Ubuntu you can install this with:

```
sudo apt install python3-dev
```

And then install gpiod with:

```
pip install gpiod
```

You can optionally depend upon your system gpiod by installing with:

```
LINK_SYSTEM_LIBGPIOD=1 pip install gpiod
```

If you still need the deprecated pure-Python bindings, install with:

```
pip install gpiod==1.5.4
```

## Examples

Check a GPIO chip character device exists:

```python
import gpiod

gpiod.is_gpiochip_device("/dev/gpiochip0")

```

Get information about a GPIO chip character device:

```python
import gpiod

with gpiod.Chip("/dev/gpiochip0") as chip:
    info = chip.get_info()
    print(f"{info.name} [{info.label}] ({info.num_lines} lines)")
```

Blink an LED, or toggling a GPIO line:

```python
import time

from gpiod.line import Direction, Value

LINE = 5

with gpiod.request_lines(
    "/dev/gpiochip0",
    consumer="blink-example",
    config={
        LINE: gpiod.LineSettings(
            direction=Direction.OUTPUT, output_value=Value.ACTIVE
        )
    },
) as request:
    while True:
        request.set_value(LINE, Value.ACTIVE)
        time.sleep(1)
        request.set_value(LINE, Value.INACTIVE)
        time.sleep(1)
```

## Testing

The test suite for the python bindings can be run by calling:

```
make python-tests-run
```

from the `libgpiod/bindings/python` directory as root (necessary to be able
to create the **gpio-sims** used for testing).

## Linting/Formatting

When making changes, ensure type checks and linting still pass:

```
python3 -m venv venv
. venv/bin/activate
pip install mypy ruff
mypy; ruff format; ruff check
```

Ideally the gpiod library will continue to pass strict checks:

```
mypy --strict
```