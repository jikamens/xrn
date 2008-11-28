%define tgz_version 9.03-beta-12
%define rpm_version 9.03b12

Summary: An X Windows System news reader.
Name: xrn
Version: %{rpm_version}
Release: 1jik
Copyright: UCB
Group: Applications/Internet
Source: xrn-%{tgz_version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
%description
A simple Usenet News reader for the X Window System.  Xrn allows you to
point and click your way through reading, replying and posting news
messages.

Install the xrn package if you need a simple news reader for X.

%prep
%setup -n xrn-%{tgz_version}
%build
`xmkmf | tail -1` -DLINUX_DIST
make depend
make all EXTRA_DEFINES="$RPM_OPT_FLAGS"
%install
make install install.man DESTDIR=$RPM_BUILD_ROOT
%files
%doc COMMON-PROBLMS COPYRIGHT CREDITS README
/usr/X11R6/bin/xrn
/usr/X11R6/lib/X11/app-defaults/XRn
/usr/X11R6/man/man1/*
