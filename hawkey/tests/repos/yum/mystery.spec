Summary: mystery package
Name: mystery
Version: 19.67
Release: 1
Group: Utilities
License: GPLv2+
Distribution: Hawkey test suite.
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-hawkey

%description
Hawkey mystery package to test filelists handling.

%package devel
Summary:	Devel subpackage.
Group:		Development/Libraries

%description devel
Magical development files for mystery.

%prep

%build

%install
mkdir -p %{buildroot}/%_bindir
echo "thats an invitation" > %{buildroot}/%_bindir/my
echo "make a reservation" > %{buildroot}/%_bindir/ste
echo "step right this way" > %{buildroot}/%_bindir/ry

%files devel
%_bindir/my
%_bindir/ste
%_bindir/ry
