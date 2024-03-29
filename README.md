# XRN - a Usenet News reader for the X window system.

XRN strives to adhere to all relevant standards and to help users
avoid the common pitfalls of participation in the Usenet. To that end,
all releases of XRN starting with 9.02 are compliant with the Good
Net-Keeping Seal of Approval (GNKSA), a standard which defines how
News readers should behave to make the Usenet a better place for
everyone. More information about the GNKSA can be found [here][gnksa].
XRN's GNKSA review can be viewed [here][gnksa-xrn].

XRN is maintained by Jonathan Kamens.  Bug reports or comments about
it should be sent to bug-xrn@kamens.us.

XRN knows how to do threading, but it doesn't do MIME, e.g., it
properly display postings that are formatted in HTML and it doesn't
know how to reassemble multipart postings.  If you need these
features, you should use another News reader, e.g., Mozilla
Thunderbird.

Heck, you should probably just go ahead and use Mozilla Thunderbird in
any case.  XRN is creaky old software that hardly anybody uses
anymore.  Why are you bothering with it?

Seriously, XRN does have some advantages over other News readers
available nowadays.  These include:

- It's fast.
- It uses much, much less memory.  It was written back in the day when
  people actually cared about how much memory their applications used.
- You can do a lot with the keyboard -- virtually all buttons in XRN
  are bound to keyboard shortcuts.
- It has relatively powerful KILL-file functionality (although there
  are probably News readers out there which can do better).
- It's rock solid.  It hardly ever crashes, and when it does, it's
  usually because the server did something really obnoxious.

XRN's biggest disadvantage is that very few people use it, I'm hardly
supporting it, and I'm not really doing any new development for it.
If you're looking for a supported, evolving News reader with an active
user community, you should probably look elsewhere.

# Where to get XRN

The XRN home page is <https://stuff.mit.edu/~jik/software/xrn.html>.

XRN's Github repository is <https://github.com/jikamens/xrn>. Releases are available for download at <https://github.com/jikamens/xrn/releases>.

# Compiling and installing XRN

There's an RPM spec file in the tarball, though it hasn't been tested
in a while. If you're lucky, you can download the source code and run
"rpmbuild -tb xrn-<version>.tar.gz", and it will "just work." Of
course, this assume that you have "rpmbuild" installed on your system.
You may also need to install some "devel" packages that XRN needs; if
so, then rpmbuild will tell you what to install.

If you're not on an RPM-based system, then you should be able to
download the source tarball, unpack it, run "./configure" in the
source directory, run "make", and finally run "make install" as root.

If you have no idea whatsoever what the last two paragraphs mean, then
you probably shouldn't be using XRN.  As I mentioned above, I'm hardly
supporting XRN nowadays, so the only people I expect to be using it
are people who can pretty much figure out how to compile it for
themselves.  If you have trouble compiling it, feel free to send me
email, but you may get back the response, "Sorry, but I can't help."

The only UNIX system I work on nowadays in Linux, and therefore the
only UNIX system on which building XRN has been tested in quite a
while is Linux.  If you're trying to build it on some other kind of
UNIX, you're using a recent version of X with that uses autoconf
support, and it's not working for you, then I'd like to hear from you.

# If you have problems...

If you have problems compiling or running XRN, you should check the
COMMON-PROBLMS file in the XRN distribution to see if your problem is
documented there. If not, write to bug-xrn@kamens.us for assistance.

# The XRN mailing list

If you would like to be informed of new releases of XRN and of any
major bugs that are discovered in between releases, you can subscribe
to the xrn-users@kamens.us mailing list. It's an announcement list,
not a discussion list, so the traffic on it is very low. Send mail to
xrn-users-request@kamens.us to ask to be added to the list.

# The future of XRN

Release 10.00 of XRN is the first general-availability release in
almost ten years.  The next major release may take just as long, or
even longer.  I have very little time to work on XRN.

However, I do not want XRN to be orphaned or to deteriorate.
Therefore, I will continue to make bug-fix releases as necessary, and
I will devote as much time as I can to making enhancements.

If you are interested in volunteering to take over, I'd love to hear
from you.

# Interesting files in the XRN distribution

There are a number of files in the XRN distribution besides the actual
source files for the program.  I maintain some of these files
actively.  Others are left over from before I started maintaining XRN,
and I have not reviewed them for correctness or updated them; use the
information in them at your own risk.  In the following list,
asterisks indicate files which I have tried to keep up-to-date (this
more files may be updated in future releases, especially if people
send me feedback about the out-of-date ones):

- **COMMON-PROBLMS** -- A list of common problems and questions about
  the program, with explanations and solutions.
- **COPYRIGHT** -- The XRN copyright notice.
- **README.md** -- This file.
- **TODO** -- A list of pending bug fixes and improvements to XRN. If
  you're interested in helping to develop XRN, picking one of the
  items in this list and digging around in the source code to see what
  you can do about it is a good way to start.
  This file is sort of "half up-to-date," because I've been adding new
  items to the beginning of it, but the items at the end of it are
  left over from before I took over maintenance of XRN, and some of
  them may be inaccurate.
- **contrib/\*** -- Various scripts, hints, etc. contributed by XRN users
  and not "officially" part of XRN.
- **doc/rfc977.txt** -- The Internet RFC governing NNTP, the Network
  News Transport Protocol, which XRN users to talk to the server.
- **still-to-do/\*** Various bug fixes that have not yet been examined
  or incorporated into the program. Some of these are probably
  obsolete.  I inherited them when I took over maintenance of the
  program, and I haven't done anything with them.

# Copyright

Copyright (c) 1988-1993, Ellen M. Sentovich and Rick L. Spickelmier.  
Copyright (c) 1994-2023, Jonathan Kamens.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of the University of California not
be used in advertising or publicity pertaining to distribution of 
the software without specific, written prior permission.  The University
of California makes no representations about the suitability of this
software for any purpose.  It is provided "as is" without express or
implied warranty.

THE UNIVERSITY OF CALIFORNIA DISCLAIMS ALL WARRANTIES WITH REGARD TO 
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


I hope you find XRN useful!

  Jonathan Kamens
  jik@kamens.us

[gnksa]: https://web.archive.org/web/20160417105503/http://www.gnksa.org/gnksa.txt
[gnksa-xrn]: https://web.archive.org/web/http://js.home.xs4all.nl/gnksa/Evaluations/xrn-9.02beta4.txt
