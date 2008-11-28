%define tgz_version 9.03-beta-15
%define rpm_version 9.03b15

Summary: An X Windows System news reader.
Name: xrn
Version: %{rpm_version}
Release: 6jik
License: UCB
Group: Applications/Internet
Source: xrn-%{tgz_version}.tar.gz
Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: libXaw-devel
%description
A simple Usenet News reader for the X Window System.  Xrn allows you to
point and click your way through reading, replying and posting news
messages.

Install the xrn package if you need a simple news reader for X.

%prep
%setup -n xrn-%{tgz_version}
%build
%configure
make
%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
%files
%defattr(-,root,root)
%doc COMMON-PROBLMS COPYRIGHT CREDITS README
%{_bindir}/*
%{_sysconfdir}/X11/app-defaults/*
%{_mandir}/man1/*
