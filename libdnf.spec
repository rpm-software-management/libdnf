Name:           libdnf
Version:        5.0.0
Release:        1%{?dist}
Summary:        Package management library
License:        LGPLv2+
URL:            https://github.com/rpm-software-management/libdnf
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz


# ========== build options ==========

%bcond_without dnfdaemon_client
%bcond_without dnfdaemon_server
%bcond_without libdnf_cli
%bcond_without microdnf

%bcond_without comps
%bcond_without modulemd
%bcond_without zchunk

%bcond_with    html
%bcond_without man

%bcond_without go
%bcond_without perl5
%bcond_without python3
%bcond_without ruby

%bcond_with sanitizers
%bcond_with disabled_tests


# ========== versions of dependencies ==========

%global libmodulemd_version 2.5.0
%global librepo_version 1.11.0
%global libsolv_version 0.7.7
%global swig_version 3.0.12
%global zchunk_version 0.9.11


# ========== build requires ==========

BuildRequires:  cmake
BuildRequires:  doxygen
BuildRequires:  gcc-c++
BuildRequires:  gettext

BuildRequires:  pkgconfig(check)
%if ! %{with tests_disabled}
BuildRequires:  pkgconfig(cppunit)
%endif
BuildRequires:  pkgconfig(gpgme)
BuildRequires:  pkgconfig(json-c)
%if %{with comps}
BuildRequires:  pkgconfig(libcomps)
%endif
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(librepo) >= %{librepo_version}
BuildRequires:  pkgconfig(libsolv) >= %{libsolv_version}
BuildRequires:  pkgconfig(libsolvext) >= %{libsolv_version}
%if %{with modulemd}
BuildRequires:  pkgconfig(modulemd-2.0) >= %{libmodulemd_version}
%endif
BuildRequires:  pkgconfig(rpm) >= 4.11.0
BuildRequires:  pkgconfig(sqlite3)
%if %{with zchunk}
BuildRequires:  pkgconfig(zck) >= %{zchunk_version}
%endif

BuildRequires:  python3dist(sphinx)

%if %{with sanitizers}
BuildRequires:  libasan-static
BuildRequires:  liblsan-static
BuildRequires:  libubsan-static
%endif


# ========== libdnf ==========

#Requires:       libmodulemd{?_isa} >= {libmodulemd_version}
Requires:       libsolv%{?_isa} >= %{libsolv_version}
Requires:       librepo%{?_isa} >= %{librepo_version}

%description
Package management library

%files
%{_libdir}/libdnf.so.*


# ========== libdnf-cli ==========

%package -n libdnf-cli
Summary:        CLI
BuildRequires:  pkgconfig(smartcols)

%description -n libdnf-cli
CLI

%files -n libdnf-cli
%{_libdir}/libdnf-cli.so.*


# ========== libdnf-devel ==========

%package devel
Summary:        Development files for libdnf
Requires:       libdnf%{?_isa} = %{version}-%{release}
Requires:       libsolv-devel%{?_isa} >= %{libsolv_version}

%description devel
Development files for libdnf.

%files devel
%{_libdir}/*.so


# ========== perl5-libdnf ==========

%if %{with perl5}
%package -n perl5-libdnf
Summary:        Perl 5 for the libdnf library.
Requires:       libdnf%{?_isa} = %{version}-%{release}
BuildRequires:  perl-devel
BuildRequires:  swig >= %{swig_version}

%description -n perl5-libdnf
Perl 5 bindings for the libdnf library.

%files -n perl5-libdnf
#{perl_vendorarch}/libdnf/*
%endif


# ========== python3-libdnf ==========

%if %{with python3}
%package -n python3-libdnf
%{?python_provide:%python_provide python3-libdnf}
Summary:        Python 3 bindings for the libdnf library.
Requires:       libdnf%{?_isa} = %{version}-%{release}
BuildRequires:  python3-devel
BuildRequires:  swig >= %{swig_version}

%description -n python3-libdnf
Python 3 bindings for the libdnf library.

%files -n python3-libdnf
%{python3_sitearch}/libdnf/
%endif


# ========== python3-libdnf-cli ==========

%if %{with python3} && %{with libdnf_cli}
%package -n python3-libdnf-cli
%{?python_provide:%python_provide python3-libdnf-cli}
Summary:        Python 3 bindings for the libdnf library.
Requires:       libdnf%{?_isa} = %{version}-%{release}
BuildRequires:  python3-devel
BuildRequires:  swig >= %{swig_version}

%description -n python3-libdnf-cli
Python 3 bindings for the libdnf library.

%files -n python3-libdnf-cli
%{python3_sitearch}/libdnf_cli/
%endif


# ========== dnfdaemon-client ==========

%package -n dnfdaemon-client
Summary:        Command-line interface for dnfdaemon-server
Requires:       libdnf%{?_isa} = %{version}-%{release}
Requires:       libdnf-cli%{?_isa} = %{version}-%{release}

%description -n dnfdaemon-client
Command-line interface for dnfdaemon-server

%files -n dnfdaemon-client
%{_bindir}/dnfdaemon-client


# ========== dnfdaemon-server ==========

%package -n dnfdaemon-server
Summary:        Package management service with a DBus interface
Requires:       libdnf%{?_isa} = %{version}-%{release}
Requires:       libdnf-cli%{?_isa} = %{version}-%{release}
Requires:       dnf-data

%description -n dnfdaemon-server
Package management service with a DBus interface

%files -n dnfdaemon-server
%{_bindir}/dnfdaemon-server


# ========== microdnf ==========

%package -n microdnf
Summary:        Package management service with a DBus interface
Requires:       libdnf%{?_isa} = %{version}-%{release}
Requires:       libdnf-cli%{?_isa} = %{version}-%{release}
Requires:       dnf-data

%description -n microdnf
Package management service with a DBus interface

%files -n microdnf
%{_bindir}/microdnf


# ========== unpack, build, check & install ==========

%prep
%autosetup -p1


%build
%cmake \
    -DWITH_DNFDAEMON_CLIENT=%{?with_dnfdaemon_client:ON}%{!?with_dnfdaemon_client:OFF} \
    -DWITH_DNFDAEMON_SERVER=%{?with_dnfdaemon_server:ON}%{!?with_dnfdaemon_server:OFF} \
    -DWITH_LIBDNF_CLI=%{?with_libdnf_cli:ON}%{!?with_libdnf_cli:OFF} \
    -DWITH_MICRODNF=%{?with_microdnf:ON}%{!?with_microdnf:OFF} \
    \
    -DWITH_COMPS=%{?with_comps:ON}%{!?with_comps:OFF} \
    -DWITH_MODULEMD=%{?with_modulemd:ON}%{!?with_modulemd:OFF} \
    -DWITH_ZCHUNK=%{?with_zchunk:ON}%{!?with_zchunk:OFF} \
    \
    -DWITH_HTML=%{?with_html:ON}%{!?with_html:OFF} \
    -DWITH_MAN=%{?with_man:ON}%{!?with_man:OFF} \
    \
    -DWITH_GO=%{?with_go:ON}%{!?with_go:OFF} \
    -DWITH_PERL5=%{?with_perl5:ON}%{!?with_perl5:OFF} \
    -DWITH_PYTHON3=%{?with_python3:ON}%{!?with_python3:OFF} \
    -DWITH_RUBY=%{?with_ruby:ON}%{!?with_ruby:OFF} \
    \
    -DWITH_SANITIZERS=%{?with_sanitizers:ON}%{!?with_sanitizers:OFF} \
    -DWITH_TESTS_DISABLED=%{?with_tests_disabled:ON}%{!?with_tests_disabled:OFF}
%make_build


%check
%if ! %{with disabled_tests}
    make ARGS="-V" test
%endif


%install
%make_install


#find_lang {name}


%ldconfig_scriptlets


%changelog
