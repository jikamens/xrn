Summary: An X Windows System news reader.
Name: xrn
Version: 9.02
Release: 1jik
Copyright: UCB
Group: Applications/Internet
Source: xrn-9.02.tgz
%description
A simple Usenet News reader for the X Window System.  Xrn allows you to
point and click your way through reading, replying and posting news
messages.

Install the xrn package if you need a simple news reader for X.

%prep
%setup
%build
`xmkmf | tail -1` -DLINUX_DIST
make depend
make all
%install
make install install.man
%files
%doc COMMON-PROBLMS COPYRIGHT CREDITS README
/usr/X11R6/bin/xrn
/usr/X11R6/lib/X11/app-defaults/XRn
/usr/X11R6/man/man1/xrn.1x
