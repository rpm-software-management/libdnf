%global gitrev 04ecf00
%global libsolv_version 0.0.0-11

Name:		hawkey
Version:	0.2.4
Release:	4.git%{gitrev}%{?dist}
Summary:	A Library providing simplified C and Python API to libsolv.
Group:		Development/Libraries
License:	LGPLv2+
URL:		https://github.com/akozumpl/hawkey
# git archive %{gitrev} --prefix=hawkey/ | xz > hawkey-%{gitrev}.tar.xz
Source0:	hawkey-%{gitrev}.tar.xz
BuildRequires:	libsolv-devel >= %{libsolv_version}
BuildRequires:  cmake expat-devel rpm-devel zlib-devel check-devel
BuildRequires: 	python2 python2-devel
Requires:	libsolv >= %{libsolv_version}

%description
A Library providing simplified C and Python API to libsolv

%package devel
Summary:	A Library providing simplified C and Python API to libsolv.
Group:		Development/Libraries
Requires:	hawkey%{?_isa} = %{version}-%{release}

%description devel
Development files for hawkey.

%package -n python-hawkey
Summary:	Python bindings for the hawkey library.
Group:		Development/Languages
Requires:	python
Requires:	hawkey = %{version}-%{release}

%description -n python-hawkey
Python bindings for the hawkey library.

%prep
%setup -q -n hawkey

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%files
%{_libdir}/libhawkey.so*

%files devel
%{_libdir}/libhawkey.so
%_includedir/hawkey

%files -n python-hawkey
%{python_sitearch}/*

%changelog
* Wed Jun 13 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-4.git04ecf00{?dist}
- Downgrades.

* Fri Jun 8 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-2.git1f198aa{?dist}
- Handling presto metadata.

* Wed May 16 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.3-1.git6083b79{?dist}
- Support libsolv's SOLVER_FLAGS_ALLOW_UNINSTALL.

* Mon May 14 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.2-1.git46bc9ec{?dist}
- Api cleanups.

* Fri May 4 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.1-1.gita59de8c0{?dist}
- Goal.update() takes flags to skip checking a pkg is installed.

* Tue Apr 24 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.0-4.gita7fafb2%{?dist}
- hy_query_filter_in()
- Better unit test support.

* Thu Apr 12 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.1-6.git0e6805c%{?dist}
- Initial package.
