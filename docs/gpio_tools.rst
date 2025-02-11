..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   GPIO tools documentation

Command-line tools
==================

Overview
--------

The **libgpiod** project includes a suite of **command-line tools** to
facilitate GPIO manipulation from console and shell scripts.

There are currently six command-line tools available:

* **gpiodetect**: list all gpiochips present on the system, their names, labels
  and number of GPIO lines
* **gpioinfo**: list lines, their gpiochip, offset, name, and direction, and if
  in use then the consumer name and any other configured attributes, such as
  active state, bias, drive, edge detection and debounce period
* **gpioget**: read values of specified GPIO lines
* **gpioset**: set values of specified GPIO lines, holding the lines until the
  process is killed or otherwise exits
* **gpiomon**: wait for edge events on GPIO lines, specify which edges to watch
  for, how many events to process before exiting, or if the events should be
  reported to the console
* **gpionotify**: wait for changed to the info for GPIO lines, specify which
  changes to watch for, how many events to process before exiting, or if the
  events should be reported to the console

.. toctree::
   :maxdepth: 1
   :caption: Manual entries

   gpiodetect<gpiodetect>
   gpioinfo<gpioinfo>
   gpioget<gpioget>
   gpioset<gpioset>
   gpiomon<gpiomon>
   gpionotify<gpionotify>

Examples
--------

.. note::
   The following examples where creating using a Raspberry Pi 4B. The pins
   will be different on other board.

Detect the available gpiochips:

.. code-block:: none

   $ gpiodetect
   gpiochip0 [pinctrl-bcm2711] (58 lines)
   gpiochip1 [raspberrypi-exp-gpio] (8 lines)

Read the info for all the lines on a gpiochip:

.. code-block:: none

   $ gpioinfo -c 1
   gpiochip1 - 8 lines:
    line   0: "BT_ON"          output
    line   1: "WL_ON"          output
    line   2: "PWR_LED_OFF"    output active-low consumer="led1"
    line   3: "GLOBAL_RESET"   output
    line   4: "VDD_SD_IO_SEL"  output consumer="vdd-sd-io"
    line   5: "CAM_GPIO"       output consumer="cam1_regulator"
    line   6: "SD_PWR_ON"      output consumer="sd_vcc_reg"
    line   7: "SD_OC_N"        input

Read the info for particular lines:

.. code-block:: none

   $ ./gpioinfo PWR_LED_OFF STATUS_LED_G_CLK GLOBAL_RESET
   gpiochip0 42 "STATUS_LED_G_CLK" output consumer="led0"
   gpiochip1 2 "PWR_LED_OFF"    output active-low consumer="led1"
   gpiochip1 3 "GLOBAL_RESET"   output

Read the value of a single GPIO line by name:

.. code-block:: none

   $ gpioget RXD1
   "RXD1"=active

Read the value of a single GPIO line by chip and offset:

.. code-block:: none

   $ gpioget -c 0 15
   "15"=active

Read the value of a single GPIO line as a numeric value:

.. code-block:: none

   $ gpioget --numeric RXD1
   1

Read two values at the same time, set the active state of the lines to low and
without quoted names:

.. code-block:: none

   $ gpioget --active-low --unquoted GPIO23 GPIO24
   GPIO23=active GPIO24=active

Set the value of a line and hold the line until killed:

.. code-block:: none

   $ gpioset GPIO23=1

Set values of two lines, then daemonize and hold the lines:

.. code-block:: none

   $ gpioset --daemonize GPIO23=1 GPIO24=0

Set the value of a single line, hold it for 20ms, then exit:

.. code-block:: none

   $ gpioset --hold-period 20ms -t0 GPIO23=1

Blink an LED on GPIO22 at 1Hz:

.. code-block:: none

   $ gpioset -t500ms GPIO22=1

Blink an LED on GPIO22 at 1Hz with a 20% duty cycle:

.. code-block:: none

   $ gpioset -t200ms,800ms GPIO22=1

Set some lines interactively (requires ``--enable-gpioset-interactive``):

.. code-block:: none

   $ gpioset --interactive --unquoted GPIO23=inactive GPIO24=active
   gpioset> get
   GPIO23=inactive GPIO24=active
   gpioset> toggle
   gpioset> get
   GPIO23=active GPIO24=inactive
   gpioset> set GPIO24=1
   gpioset> get
   GPIO23=active GPIO24=active
   gpioset> toggle
   gpioset> get
   GPIO23=inactive GPIO24=inactive
   gpioset> toggle GPIO23
   gpioset> get
   GPIO23=active GPIO24=inactive
   gpioset> exit

Wait for three rising edge events on a single GPIO line, then exit:

.. code-block:: none

   $ gpiomon --num-events=3 --edges=rising GPIO22
   10002.907638045     rising  "GPIO22"
   10037.132562259     rising  "GPIO22"
   10047.179790748     rising  "GPIO22"

Wait for three edge events on a single GPIO line, with time in local time and
with unquoted line name, then exit:

.. code-block:: none

   $ gpiomon --num-events=3 --edges=both --localtime --unquoted GPIO22
   2022-11-15T10:36:59.109615508       rising  GPIO22
   2022-11-15T10:36:59.129681898       falling GPIO22
   2022-11-15T10:36:59.698971886       rising  GPIO22

Wait for falling edge events with a custom output format:

.. code-block:: none

   $ gpiomon --format="%e %c %o %l %S" --edges=falling -c gpiochip0 22
   2 gpiochip0 22 GPIO22 10946.693481859
   2 gpiochip0 22 GPIO22 10947.025347604
   2 gpiochip0 22 GPIO22 10947.283716669
   2 gpiochip0 22 GPIO22 10947.570109430
   ...

Block until an edge event occurs, don't print anything:

.. code-block:: none

   $ gpiomon --num-events=1 --quiet GPIO22

Monitor multiple lines, exit after the first edge event:

.. code-block:: none

   $ gpiomon --quiet --num-events=1 GPIO5 GPIO6 GPIO12 GPIO17

Monitor a line for changes to info:

.. code-block:: none

   $ gpionotify GPIO23
   11571.816473718     requested       "GPIO23"
   11571.816535124     released        "GPIO23"
   11572.722894029     requested       "GPIO23"
   11572.722932843     released        "GPIO23"
   11573.222998598     requested       "GPIO23"
   ...

Monitor a line for requests, reporting UTC time and unquoted line name:

.. code-block:: none

   $ gpionotify --utc --unquoted GPIO23
   2022-11-15T03:05:23.807090687Z      requested       GPIO23
   2022-11-15T03:05:23.807151390Z      released        GPIO23
   2022-11-15T03:05:24.784984280Z      requested       GPIO23
   2022-11-15T03:05:24.785023873Z      released        GPIO23
   ...

Monitor multiple lines, exit after the first is requested:

.. code-block:: none

   $ gpionotify --quiet --num-events=1 --event=requested GPIO5 GPIO6 GPIO12 GPIO17

Block until a line is released:

.. code-block:: none

   $ gpionotify --quiet --num-events=1 --event=released GPIO6
