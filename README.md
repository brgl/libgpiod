libgpiod
========

  libgpiod - C library and tools for interacting with the linux GPIO
             character device

Since linux 4.7 the GPIO sysfs interface is deprecated. User space should use
the character device instead. This library encapsulates the ioctl calls and
data structures behind a straightforward API.

RATIONALE
---------

The new character device interface guarantees all allocated resources are
freed after closing the device file descriptor and adds several new features
that are not present in the obsolete sysfs interface (like event polling,
setting/reading multiple values at once or open-source and open-drain GPIOs).

Unfortunately interacting with the linux device file can no longer be done
using only standard command-line tools. This is the reason for creating a
library encapsulating the cumbersome, ioctl-based kernel-userspace interaction
in a set of convenient functions and opaque data structures.

Additionally this project contains a set of command-line tools that should
allow an easy conversion of user scripts to using the character device.

BUILDING
--------

This is a pretty standard autotools project. It does not depend on any
libraries other than the standard C library with GNU extensions.

The autoconf version needed to compile the project is 2.61.

Recent (as in >= v4.7) kernel headers are also required for the GPIO user
API definitions.

To build the project (including command-line utilities) run:

    ./autogen.sh
    ./configure --enable-tools=yes --prefix=<install path>
    make
    make install

TOOLS
-----

There are currently six command-line tools available:

* gpiodetect - list all gpiochips present on the system, their names, labels
               and number of GPIO lines

* gpioinfo   - list all lines of specified gpiochips, their names, consumers,
               direction, active state and additional flags

* gpioget    - read values of specified GPIO lines

* gpioset    - set values of specified GPIO lines, potentially keep the lines
               exported and wait until timeout, user input or signal

* gpiofind   - find the gpiochip name and line offset given the line name

* gpiomon    - wait for events on a GPIO line, specify which events to watch,
               how many events to process before exiting or if the events
               should be reported to the console

CONTRIBUTING
------------

Contributions are welcome - please use github pull-requests and issues and
stick to the linux kernel coding style when submitting new code.
