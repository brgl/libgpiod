<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->
<!-- SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> -->
<!-- Based on text originally written by Grant Likely <grant.likely@linaro.org> -->

Contributing
============

Master copy of this project is hosted at kernel.org:
https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/

Anyone may contribute to this project. Contributions are licensed under GPLv2
for programs and LGPLv2.1 for libraries and must be made with a Developer
Certificate of Origin [DCO] "Signed-off-by:" attestation as described below,
indicating that you wrote the code and have the right to pass it on as an open
source patch under the GPLv2 license. Patches that are not signed off will not
be accepted.

Send patches using `git send-email` to the linux-gpio mailing list[2] by
e-mailing to linux-gpio@vger.kernel.org (add the [libgpiod] prefix to the
e-mail subject line). Note that the mailing list quietly drops HTML formatted
e-mail, so be sure to send plain text[3].

Also, please write good git commit messages. A good commit message looks like
this:

```
section: explain the commit in one line (use the imperative)

Body of commit message is a few lines of text, explaining things
in more detail, possibly giving some background about the issue
being fixed, etc etc.

The body of the commit message can be several paragraphs, and
please do proper word-wrap and keep columns shorter than about
74 characters or so. That way "git log" will show things
nicely even when it's indented.

Make sure you explain your solution and why you're doing what you're
doing, as opposed to describing what you're doing. Reviewers and your
future self can read the patch, but might not understand why a
particular solution was implemented.

Reported-by: whoever-reported-it
Signed-off-by: Your Name <you@example.com>
```

Where that header line really should be meaningful, and really should be just
one line. That header line is what is shown by tools like gitk and shortlog,
and should summarize the change in one readable line of text, independently of
the longer explanation. Please use verbs in the imperative in the commit
message, as in "Fix bug that...", "Add file/feature ...", or "Make plugin ...".

DCO Attestation
---------------

To help track the origin of contributions, this project uses the same [DCO]
"sign-off" process as used by the Linux kernel. The sign-off is a simple line
at the end of the explanation for the patch, which certifies that you wrote it
or otherwise have the right to pass it on as an open-source patch. The rules
are pretty simple: if you can certify the below:

### Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

        (a) The contribution was created in whole or in part by me and I
            have the right to submit it under the open source license
            indicated in the file; or

        (b) The contribution is based upon previous work that, to the best
            of my knowledge, is covered under an appropriate open source
            license and I have the right under that license to submit that
            work with modifications, whether created in whole or in part
            by me, under the same open source license (unless I am
            permitted to submit under a different license), as indicated
            in the file; or

        (c) The contribution was provided directly to me by some other
            person who certified (a), (b) or (c) and I have not modified
            it.

        (d) I understand and agree that this project and the contribution
            are public and that a record of the contribution (including all
            personal information I submit with it, including my sign-off) is
            maintained indefinitely and may be redistributed consistent with
            this project or the open source license(s) involved.

then you just add a line saying:

        Signed-off-by: Random J Developer <random@developer.example.org>

[DCO]: https://developercertificate.org/
