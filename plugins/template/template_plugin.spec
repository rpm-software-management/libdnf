# SPEC file overview:
# https://docs.fedoraproject.org/en-US/quick-docs/creating-rpm-packages/#con_rpm-spec-file-overview
# Fedora packaging guidelines:
# https://docs.fedoraproject.org/en-US/packaging-guidelines/


Name:		template_plugin
Version:	1
Release:	1%{?dist}
Summary:	template_plugin for libdnf

License:	LGPLv2+

BuildRequires:	cmake
BuildRequires:	gcc
Requires:	pkgconfig(libdnf)
Source0:	%{name}.tar.gz

%description
template_plugin for libdnf using cmake for build

%prep
# we expect that our source was packed with (from README.md): tar -czvf ~/rpmbuild/SOURCES/your_name.tar.gz ../template/
# and thefore is inside template directory
%autosetup -n template

%build
%cmake .
%make_build

%install
%make_install

%files
%{_libdir}/libdnf/plugins/template_plugin.so
