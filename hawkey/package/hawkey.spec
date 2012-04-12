%global gitrev 1c03a0b
%global libsolv_version 0.0.0-7

Name:		hawkey
Version:	0.1
Release:	5.%{gitrev}%{?dist}
Summary:	A Library providing simplified C and Python API to libsolv.
Group:		Development/Libraries
License:	none-yet
URL:		https://github.com/akozumpl/hawkey
# git archive %{gitrev} --prefix=hawkey/ | xz > hawkey-%{gitrev}.tar.xz
Source0:	hawkey-%{gitrev}.tar.xz
BuildRequires:	libsolv-devel >= %{libsolv_version}
BuildRequires:  cmake expat-devel rpm-devel zlib-devel check-devel
BuildRequires: 	python2-devel
Requires:	libsolv >= %{libsolv_version}

%description
A Library providing simplified C and Python API to libsolv

%package devel
Summary:	A Library providing simplified C and Python API to libsolv.
Group:		Development/Libraries

%description devel
Development files for hawkey.

%prep
%setup -q -n hawkey

%build
%cmake .
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%{_libdir}/libhawkey.so*

%files devel
%{_libdir}/libhawkey.so
%_includedir/hawkey

%changelog
* Thu Apr 11 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.1-1.git7b6932f%{?dist}
- Initial package.
