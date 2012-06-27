%global gitrev 04ecf00
%global libsolv_version 0.0.0-11

Name:		hawkey
Version:	0.2.4
Release:	8.git%{gitrev}%{?dist}
Summary:	Library providing simplified C and Python API to libsolv
Group:		System Environment/Libraries
License:	LGPLv2+
URL:		https://github.com/akozumpl/hawkey
# git clone https://github.com/akozumpl/hawkey.git && cd hawkey && package/archive
Source0:	hawkey-%{gitrev}.tar.xz
BuildRequires:	libsolv-devel >= %{libsolv_version}
BuildRequires:	cmake expat-devel rpm-devel zlib-devel check-devel
BuildRequires:	python2 python2-devel
# explicit dependency: libsolv occasionally goes through ABI changes without
# bumping the .so number:
Requires:	libsolv%{?_isa} >= %{libsolv_version}

# prevent provides from nonstandard paths:
%filter_provides_in %{python_sitearch}/.*\.so$
# filter out _hawkey_testmodule.so DT_NEEDED _hawkeymodule.so:
%filter_requires_in %{python_sitearch}/hawkey/test/.*\.so$
%filter_setup

%description
A Library providing simplified C and Python API to libsolv.

%package devel
Summary:	A Library providing simplified C and Python API to libsolv
Group:		Development/Libraries
Requires:	hawkey%{?_isa} = %{version}-%{release}

%description devel
Development files for hawkey.

%package -n python-hawkey
Summary:	Python bindings for the hawkey library
Group:		Development/Languages
Requires:	%{name}%{?_isa} = %{version}-%{release}

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

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%doc COPYING README.md
%{_libdir}/libhawkey.so.*

%files devel
%{_libdir}/libhawkey.so
%{_libdir}/pkgconfig/hawkey.pc
%_includedir/hawkey/

%files -n python-hawkey
%{python_sitearch}/

%changelog
* Mon Jun 25 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-8.git04ecf00
- More package review issues.

* Fri Jun 22 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-7.git04ecf00
- More package review issues.

* Wed Jun 20 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-6.git04ecf00
- Prevent requires in the hawkey.test .so.

* Tue Jun 19 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.4-5.git04ecf00
- Fix rpmlint issues.

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
