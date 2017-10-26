Summary: Library contains modules of communication interface between TRAP modules.
Name: libtrap
Version: 0.11.2
Release: 1
URL: http://www.liberouter.org/
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/libtrap/%{name}-%{version}.tar.gz
Group: Liberouter
License: BSD
Vendor: CESNET, z.s.p.o.
Packager: Travis CI User <travis@example.org>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}

Requires: openssl
BuildRequires: gcc make doxygen pkgconfig openssl-devel
Provides: libtrap

%description

%package devel
Summary: Libtrap development package
Group: Liberouter
Requires: libtrap = %{version}-%{release}
Requires: openssl-devel
Provides: libtrap-devel

%description devel
This package contains header files for libtrap library. Install this package
if you want to develop your own TRAP module.

%prep
%setup

%build
./configure --prefix=%{_prefix} --sysconfdir=%{_sysconfdir} --libdir=%{_libdir} --bindir=%{_bindir}/nemea --docdir=%{_docdir}/libtrap --with-defaultsocketdir=%{_localstatedir}/run/libtrap --disable-doxygen-pdf --disable-doxygen-ps --disable-tests -q;
make -j5
make doc

%install
make DESTDIR=$RPM_BUILD_ROOT install

%post
ldconfig

%files
%{_bindir}/nemea/trap_stats
%{_bindir}/nemea/trap2man.sh
%{_libdir}/libtrap.so.*
%{_sysconfdir}/tmpfiles.d/libtrap-varrun.conf
%defattr(-,-,-,1777)
%dir %{_localstatedir}/run/libtrap

%files devel
%{_libdir}/pkgconfig/libtrap.pc
%{_libdir}/libtrap.so
%{_libdir}/libtrap.a
%{_libdir}/libtrap.la
%{_prefix}/include/libtrap/*
%{_docdir}/libtrap/*

