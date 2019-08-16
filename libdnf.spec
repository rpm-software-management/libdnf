%global libsolv_version 0.7.4-1
%global libmodulemd_version 1.6.1
%global librepo_version 1.10.0
%global dnf_conflict 4.2.11
%global swig_version 3.0.12

%bcond_with valgrind

# Do not build bindings for python3 for RHEL <= 7
%if 0%{?rhel} && 0%{?rhel} <= 7
%bcond_with python3
%else
%bcond_without python3
%endif

%if 0%{?rhel}
    %global rpm_version 4.14.2
%else
    %global rpm_version 4.14.2.1-4
%endif

%if 0%{?rhel} > 7 || 0%{?fedora} > 29
# Disable python2 build by default
%bcond_with python2
%else
%bcond_without python2
%endif

%if 0%{?rhel} && ! 0%{?centos}
%bcond_without rhsm
%else
%bcond_with rhsm
%endif

%if 0%{?rhel}
%bcond_with zchunk
%else
%bcond_without zchunk
%endif

%global _cmake_opts \\\
    -DENABLE_RHSM_SUPPORT=%{?with_rhsm:ON}%{!?with_rhsm:OFF} \\\
    %{nil}

Name:           libdnf
Version:        0.35.6
Release:        1%{?dist}
Summary:        Library providing simplified C and Python API to libsolv
License:        LGPLv2+
URL:            https://github.com/rpm-software-management/libdnf
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  libsolv-devel >= %{libsolv_version}
BuildRequires:  pkgconfig(librepo) >= %{librepo_version}
BuildRequires:  pkgconfig(check)
%if %{with valgrind}
BuildRequires:  valgrind
%endif
BuildRequires:  pkgconfig(gio-unix-2.0) >= 2.46.0
BuildRequires:  pkgconfig(gtk-doc)
BuildRequires:  rpm-devel >= %{rpm_version}
%if %{with rhsm}
BuildRequires:  pkgconfig(librhsm) >= 0.0.3
%endif
%if %{with zchunk}
BuildRequires:  pkgconfig(zck) >= 0.9.11
%endif
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(cppunit)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(modulemd) >= %{libmodulemd_version}
BuildRequires:  pkgconfig(smartcols)
BuildRequires:  gettext
BuildRequires:  gpgme-devel

Requires:       libmodulemd%{?_isa} >= %{libmodulemd_version}
Requires:       libsolv%{?_isa} >= %{libsolv_version}
Requires:       librepo%{?_isa} >= %{librepo_version}

%if %{without python2}
# Obsoleted from here so we can track the fast growing version easily.
# We intentionally only obsolete and not provide, this is a broken upgrade
# prevention, not providing the removed functionality.
Obsoletes:      python2-%{name} < %{version}-%{release}
Obsoletes:      python2-hawkey < %{version}-%{release}
Obsoletes:      python2-hawkey-debuginfo < %{version}-%{release}
Obsoletes:      python2-libdnf-debuginfo < %{version}-%{release}
%endif

%description
A Library providing simplified C and Python API to libsolv.

%package devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       libsolv-devel%{?_isa} >= %{libsolv_version}

%description devel
Development files for %{name}.

%if %{with python2}
%package -n python2-%{name}
%{?python_provide:%python_provide python2-%{name}}
Summary:        Python 2 bindings for the libdnf library.
Requires:       %{name}%{?_isa} = %{version}-%{release}
BuildRequires:  python2-devel
%if 0%{?rhel} == 7
BuildRequires:  python-sphinx
BuildRequires:  swig3 >= %{swig_version}
%else
BuildRequires:  python2-sphinx
BuildRequires:  swig >= %{swig_version}
%endif

%description -n python2-%{name}
Python 2 bindings for the libdnf library.
%endif # with python2

%if %{with python3}
%package -n python3-%{name}
%{?python_provide:%python_provide python3-%{name}}
Summary:        Python 3 bindings for the libdnf library.
Requires:       %{name}%{?_isa} = %{version}-%{release}
BuildRequires:  python3-devel
BuildRequires:  python3-sphinx
BuildRequires:  swig >= %{swig_version}

%description -n python3-%{name}
Python 3 bindings for the libdnf library.
%endif

%if %{with python2}
%package -n python2-hawkey
Summary:        Python 2 bindings for the hawkey library
%{?python_provide:%python_provide python2-hawkey}
BuildRequires:  python2-devel
%if 0%{?rhel} && 0%{?rhel} <= 7
BuildRequires:  python-nose
%else
BuildRequires:  python2-nose
%endif
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       python2-%{name} = %{version}-%{release}
# Fix problem with hawkey - dnf version incompatibility
# Can be deleted for distros where only python2-dnf >= 2.0.0
Conflicts:      python2-dnf < %{dnf_conflict}
Conflicts:      python-dnf < %{dnf_conflict}

%description -n python2-hawkey
Python 2 bindings for the hawkey library.
%endif # with python2

%if %{with python3}
%package -n python3-hawkey
Summary:        Python 3 bindings for the hawkey library
%{?python_provide:%python_provide python3-hawkey}
BuildRequires:  python3-devel
BuildRequires:  python3-nose
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       python3-%{name} = %{version}-%{release}
# Fix problem with hawkey - dnf version incompatibility
# Can be deleted for distros where only python3-dnf >= 2.0.0
Conflicts:      python3-dnf < %{dnf_conflict}
# Obsoletes F27 packages
Obsoletes:      platform-python-hawkey < %{version}-%{release}

%description -n python3-hawkey
Python 3 bindings for the hawkey library.
%endif

%prep
%autosetup
%if %{with python2}
mkdir build-py2
%endif # with python2
%if %{with python3}
mkdir build-py3
%endif

%build
%if %{with python2}
pushd build-py2
  %cmake -DPYTHON_DESIRED:FILEPATH=%{__python2} -DWITH_MAN=OFF ../ %{!?with_zchunk:-DWITH_ZCHUNK=OFF} %{!?with_valgrind:-DDISABLE_VALGRIND=1} %{_cmake_opts}
  %make_build
popd
%endif # with python2

%if %{with python3}
pushd build-py3
  %cmake -DPYTHON_DESIRED:FILEPATH=%{__python3} -DWITH_GIR=0 -DWITH_MAN=0 -Dgtkdoc=0 ../ %{!?with_zchunk:-DWITH_ZCHUNK=OFF} %{!?with_valgrind:-DDISABLE_VALGRIND=1} %{_cmake_opts}
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

%if %{with python2}
pushd build-py2
  make ARGS="-V" test
popd
%endif # with python2
%if %{with python3}
# If we didn't run the general tests yet, do it now.
%if %{without python2}
pushd build-py3
  make ARGS="-V" test
popd
%else
# Otherwise, run just the Python tests, not all of
# them, since we have coverage of the core from the
# first build
pushd build-py3/python/hawkey/tests
  make ARGS="-V" test
popd
%endif
%endif

%install
%if %{with python2}
pushd build-py2
  %make_install
popd
%endif # with python2
%if %{with python3}
pushd build-py3
  %make_install
popd
%endif

%find_lang %{name}

%if 0%{?rhel} && 0%{?rhel} <= 7
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%else
%ldconfig_scriptlets
%endif

%files -f %{name}.lang
%license COPYING
%doc README.md AUTHORS
%{_libdir}/%{name}.so.*
%dir %{_libdir}/libdnf/
%dir %{_libdir}/libdnf/plugins/
%{_libdir}/libdnf/plugins/README

%files devel
%doc %{_datadir}/gtk-doc/html/%{name}/
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/%{name}/

%if %{with python2}
%files -n python2-%{name}
%{python2_sitearch}/%{name}/
%endif # with python2

%if %{with python3}
%files -n python3-%{name}
%{python3_sitearch}/%{name}/
%endif

%if %{with python2}
%files -n python2-hawkey
%{python2_sitearch}/hawkey/
%endif # with python2

%if %{with python3}
%files -n python3-hawkey
%{python3_sitearch}/hawkey/
%endif

%changelog
