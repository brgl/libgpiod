..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   Contribution guide.

Contributing
============

Contributions are welcome - please send questions, patches and bug reports to
the `linux-gpio mailing list
<http://vger.kernel.org/vger-lists.html#linux-gpio>`_
by e-mailing to ``linux-gpio@vger.kernel.org`` (add the ``[libgpiod]`` prefix
to the e-mail subject line). Note that the mailing list quietly drops HTML
formatted e-mail, so be sure to `send plain text
<https://docs.kernel.org/process/email-clients.html>`_.

Code submissions should stick to the `linux kernel coding style
<https://docs.kernel.org/process/coding-style.html>`_ and follow the kernel
`patch submission process
<https://docs.kernel.org/process/submitting-patches.html>`_ as applied to the
libgpiod source tree. All shell scripts must pass `shellcheck tests
<https://www.shellcheck.net/>`_. All files must have a license and copyright
SPDX headers and the repo is expected to pass the `reuse lint check
<https://reuse.software/>`_.

The `mailing list archive <https://lore.kernel.org/linux-gpio/>`_ contains all
the historical mails to the list, and is the place to check to ensure your
e-mail has been received.
Search for `"libgpiod"` to filter the list down to relevant messages. Those
also provide examples of the expected formatting. Allow some time for your
e-mail to propagate to the list before retrying, particularly if there are no
e-mails in the list more recent than yours.

There is a `libgpiod github page <https://github.com/brgl/libgpiod>`_
available for reporting bugs and general discussions and although PRs can be
submitted and discussed, upstreambound patches need to go through the mailing
list nevertheless.

.. note::
   For more information, please refer to the ``CONTRIBUTING.md`` file in the
   libgpiod source tree.
