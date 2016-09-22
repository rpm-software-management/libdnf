%global libsolv_version 0.6.21-1

%bcond_with valgrind

# Do not build bindings for python3 for RHEL <= 7
%if 0%{?rhel} && 0%{?rhel} <= 7
%bcond_with python3
%else
%bcond_without python3
%endif

Name:           libhif
Version:        0.7.0
Release:        1%{?dist}
Summary:        Library providing simplified C and Python API to libsolv
License:        LGPLv2+
URL:            https://github.com/rpm-software-management/%{name}
Source0:        https://github.com/rpm-software-management/%{name}/archive/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  libsolv-devel >= %{libsolv_version}
BuildRequires:  pkgconfig(librepo)
BuildRequires:  pkgconfig(check)
%if %{with valgrind}
BuildRequires:  valgrind
%endif
BuildRequires:  pkgconfig(gio-unix-2.0) >= 2.44.0
BuildRequires:  pkgconfig(gtk-doc)
BuildRequires:  pkgconfig(gobject-introspection-1.0)
BuildRequires:  rpm-devel >= 4.11.0
BuildRequires:  sqlite-devel

Requires:       libsolv%{?_isa} >= %{libsolv_version}

%description
A Library providing simplified C and Python API to libsolv.

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       libsolv-devel%{?_isa} >= %{libsolv_version}

%description devel
Development files for %{name}.

%package -n hawkey-man
Summary:        Documentation for the hawkey python bindings
BuildRequires:  python-nose
BuildRequires:  python-sphinx
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n hawkey-man
Python 2 bindings for the hawkey library.

%package -n python2-hawkey
Summary:        Python 2 bindings for the hawkey library
%{?python_provide:%python_provide python2-hawkey}
BuildRequires:  python2-devel
BuildRequires:  python-nose
Requires:       %{name}%{?_isa} = %{version}-%{release}
%if 0%{?fedora}
Recommends:     hawkey-man = %{version}-%{release}
%endif

%description -n python2-hawkey
Python 2 bindings for the hawkey library.

%if %{with python3}
%package -n python3-hawkey
Summary:        Python 3 bindings for the hawkey library
%{?python_provide:%python_provide python3-hawkey}
BuildRequires:  python3-devel
BuildRequires:  python3-nose
Requires:       %{name}%{?_isa} = %{version}-%{release}
%if 0%{?fedora}
Recommends:     hawkey-man = %{version}-%{release}
%endif

%description -n python3-hawkey
Python 3 bindings for the hawkey library.
%endif

%prep
%autosetup
mkdir build-py2
%if %{with python3}
mkdir build-py3
%endif

%build
pushd build-py2
  %cmake ../ %{!?_with_valgrind:-DDISABLE_VALGRIND=1}
  %make_build
popd

%if %{with python3}
pushd build-py3
  %cmake -DPYTHON_DESIRED:str=3 -DWITH_GIR=0 -DWITH_MAN=0 -Dgtkdoc=0 ../ %{!?_with_valgrind:-DDISABLE_VALGRIND=1}
  %make_build
popd
%endif

%check
if [ "$(id -u)" == "0" ] ; then
        cat <<ERROR 1>&2
Package tests cannot be run under superuser account.
Please build the package as non-root user.
ERROR
        exit 1
fi
pushd build-py2
  make ARGS="-V" test
popd
%if %{with python3}
# Run just the Python tests, not all of them, since
# we have coverage of the core from the first build
pushd build-py3/python/hawkey/tests
  make ARGS="-V" test
popd
%endif

%install
pushd build-py2
  %make_install
popd
%if %{with python3}
pushd build-py3
  %make_install
popd
%endif

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license COPYING
%doc README.md AUTHORS NEWS
%{_libdir}/%{name}.so.*
%{_libdir}/girepository-1.0/Hif-*.typelib

%files devel
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/%{name}/
%doc %{_datadir}/gtk-doc/html/%{name}/
%{_datadir}/gir-1.0/Hif-*.gir

%files -n hawkey-man
%{_mandir}/man3/hawkey.3.gz

%files -n python2-hawkey
%{python2_sitearch}/hawkey/

%if %{with python3}
%files -n python3-hawkey
%{python3_sitearch}/hawkey/
%endif

%changelog
* Sat Sep 05 2015 Kalev Lember <klember@redhat.com> - 0.2.1-4
- Rebuilt for librpm soname bump

* Tue Jul 28 2015 Kalev Lember <klember@redhat.com> 0.2.1-3
- Rebuild once more for fixed hawkey ABI

* Sun Jul 26 2015 Kevin Fenzi <kevin@scrye.com> 0.2.1-2
- Update for new librpm

* Mon Jul 13 2015 Richard Hughes <richard@hughsie.com> 0.2.1-1
- Update to new upstream version

* Thu Jul 09 2015 Colin Walters <walters@redhat.com> - 0.2.0-7
- Add patch to make baseurl work

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.2.0-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Mon Jun 15 2015 Colin Walters <walters@redhat.com> - 0.2.0-5
- Backport patch to add set_required() API so that rpm-ostree
  can use it, which will help avoid it breaking for use in
  Bodhi which points to file:// URIs with old repodata.

* Fri May 29 2015 Kalev Lember <kalevlember@gmail.com> 0.2.0-4
- Switch to using dnf's yumdb, instead of yum's yumdb

* Wed May 13 2015 Kalev Lember <kalevlember@gmail.com> 0.2.0-3
- context: Fix rpmdb file monitor with / installs (#1221158)

* Wed Apr 15 2015 Kalev Lember <kalevlember@gmail.com> 0.2.0-2
- Remove obsoleted packages when committing transaction (#1211991)

* Wed Apr 08 2015 Richard Hughes <richard@hughsie.com> 0.2.0-1
- Update to new upstream version
- Add new API required for ostree

* Sat Mar 28 2015 Kalev Lember <kalevlember@gmail.com> - 0.1.8-7
- Fix broken -devel package requires

* Mon Mar 16 2015 Than Ngo <than@redhat.com> - 0.1.8-6
- bump release and rebuild so that koji-shadow can rebuild it
  against new gcc on secondary arch

* Fri Feb 06 2015 Richard Hughes <richard@hughsie.com> 0.1.8-5
- Adapt to the new hawkey API.

* Tue Feb 03 2015 Richard Hughes <richard@hughsie.com> 0.1.8-4
- Do not crash when file:///tmp exists, but file:///tmp/repodata does not

* Tue Feb 03 2015 Richard Hughes <richard@hughsie.com> 0.1.8-3
- Do not add the source to the sack if it does not exist, which fixes a crash
  for repo files with things like baseurl=file:///notgoingtoexist
- Resolves: #1135740

* Tue Feb 03 2015 Richard Hughes <richard@hughsie.com> 0.1.8-2
- Ensure the mirror list is set before trying to download packages
- This works around a librepo behaviour change in the latest update
- Resolves: #1188600

* Mon Jan 19 2015 Richard Hughes <richard@hughsie.com> 0.1.8-1
- Update to new upstream version
- Do not attempt to read from a closed file descriptor
- Do not use hif_context_get_transaction() from multiple threads
- Ensure HifContext can be finalised to prevent rpmdb index corruption
- Resolves: #1183010

* Thu Jan 15 2015 Richard Hughes <richard@hughsie.com> 0.1.7-3
- Do not download the mirrorlists when checking for updates
- Resolves: #1181501

* Wed Jan 14 2015 Richard Hughes <richard@hughsie.com> 0.1.7-2
- Fix auto-importing of GPG keys during installing
- Resolves: #1182090 and #1182156

* Fri Dec 19 2014 Richard Hughes <richard@hughsie.com> 0.1.7-1
- Update to new upstream version
- Add the concept of metadata-only software sources
- Correctly update sources with baseurls ending with a slash
- Don't unref the HifSource when invalidating as this is not threadsafe
- Improve handling of local metadata
- Support appstream and appstream-icons metadata types

* Wed Nov 26 2014 Richard Hughes <richard@hughsie.com> 0.1.6-2
- Do not crash when trying to parse pathological files like bumblebee.repo
- Resolves: #1164330

* Tue Nov 11 2014 Richard Hughes <richard@hughsie.com> 0.1.6-1
- Update to new upstream version
- Add support for package reinstallation and downgrade
- Copy the vendor cache if present
- Ensure created directories are world-readable
- Support local repositories

* Mon Sep 22 2014 Richard Hughes <richard@hughsie.com> 0.1.5-1
- Update to new upstream version
- Add all native architectures for ARM and i386
- Check for libQtGui rather than libkde* to detect GUI apps

* Fri Sep 12 2014 Richard Hughes <richard@hughsie.com> 0.1.4-1
- Update to new upstream version
- Allow setting the default lock directory
- Ensure all the required directories exist when setting up the context

* Mon Sep 01 2014 Richard Hughes <richard@hughsie.com> 0.1.3-1
- Update to new upstream version
- Add an error path for when the sources are not valid
- Do not call hif_context_setup_sack() automatically
- Fix a logic error to fix refreshing with HIF_SOURCE_UPDATE_FLAG_FORCE

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.1.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Mon Jul 28 2014 Kalev Lember <kalevlember@gmail.com> - 0.1.2-4
- Rebuilt for hawkey soname bump

* Tue Jul 22 2014 Kalev Lember <kalevlember@gmail.com> - 0.1.2-3
- Rebuilt for gobject-introspection 1.41.4

* Sat Jul 19 2014 Kalev Lember <kalevlember@gmail.com> 0.1.2-2
- Fix a PK crash with locally mounted iso media (#1114207)

* Thu Jul 17 2014 Richard Hughes <richard@hughsie.com> 0.1.2-1
- Update to new upstream version
- Add HifContext accessor in -private for HifState
- Add name of failing repository
- Create an initial sack in HifContext
- Error if we can't find any package matching provided name
- Fix a mixup of HifStateAction and HifPackageInfo
- Improve rpm callback handling for packages in the cleanup state
- Only set librepo option if value is set
- Respect install root for rpmdb Packages monitor

* Mon Jun 23 2014 Richard Hughes <richard@hughsie.com> 0.1.1-1
- Update to new upstream version
- Fix a potential crash when removing software
- Only add system repository if it exists
- Pass install root to hawkey

* Tue Jun 10 2014 Richard Hughes <richard@hughsie.com> 0.1.0-1
- Initial version for Fedora package review
