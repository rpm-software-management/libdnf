%global libsolv_version 0.6.4-1

%if 0%{?rhel} != 0 && 0%{?rhel} <= 7
# Do not build bindings for python3 for RHEL <= 7
%bcond_with python3
%else
%bcond_without python3
%endif

Name:		hawkey
Version:	0.6.1
Release:	1%{?snapshot}%{?dist}
Summary:	Library providing simplified C and Python API to libsolv
Group:		System Environment/Libraries
License:	LGPLv2+
URL:		https://github.com/rpm-software-management/%{name}
# git clone https://github.com/rpm-software-management/hawkey.git && cd hawkey && tito build --tgz
Source0:	https://github.com/rpm-software-management/%{name}/archive/%{name}-%{version}.tar.gz
BuildRequires:	libsolv-devel >= %{libsolv_version}
BuildRequires:	cmake expat-devel rpm-devel zlib-devel check-devel valgrind
Requires:	libsolv%{?_isa} >= %{libsolv_version}
# prevent provides from nonstandard paths:
%filter_provides_in %{python_sitearch}/.*\.so$
%if %{with python3}
%filter_provides_in %{python3_sitearch}/.*\.so$
%endif
# filter out _hawkey_testmodule.so DT_NEEDED _hawkeymodule.so:
%filter_requires_in %{python_sitearch}/hawkey/test/.*\.so$
%if %{with python3}
%filter_requires_in %{python3_sitearch}/hawkey/test/.*\.so$
%endif
%filter_setup

%description
A Library providing simplified C and Python API to libsolv.

%package devel
Summary:	A Library providing simplified C and Python API to libsolv
Group:		Development/Libraries
Requires:	hawkey%{?_isa} = %{version}-%{release}
Requires:	libsolv-devel

%description devel
Development files for hawkey.

%package -n python-hawkey
Summary:	Python 2 bindings for the hawkey library
Group:		Development/Languages
BuildRequires:  python2-devel
BuildRequires:  python-nose
%if %{with python3}
BuildRequires:	python-sphinx >= 1.1.3-9
%else
BuildRequires:	python-sphinx
%endif
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n python-hawkey
Python 2 bindings for the hawkey library.

%if %{with python3}
%package -n python3-hawkey
Summary:	Python 3 bindings for the hawkey library
Group:		Development/Languages
BuildRequires:	python3-devel
BuildRequires:	python3-nose
BuildRequires:	python3-sphinx >= 1.1.3-9
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n python3-hawkey
Python 3 bindings for the hawkey library.
%endif

%prep
%setup -q -n %{name}-%{version}

%if %{with python3}
rm -rf py3
mkdir ../py3
cp -a . ../py3/
mv ../py3 ./
%endif

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make %{?_smp_mflags}
make doc-man

%if %{with python3}
pushd py3
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPYTHON_DESIRED:str=3.
make %{?_smp_mflags}
make doc-man
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
make ARGS="-V" test
%if %{with python3}
# Run just the Python tests, not all of them, since
# we have coverage of the core from the first build
pushd py3/tests/python
make ARGS="-V" test
popd
%endif

%install
make install DESTDIR=$RPM_BUILD_ROOT
%if %{with python3}
pushd py3
make install DESTDIR=$RPM_BUILD_ROOT
popd
%endif

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%doc COPYING README.rst
%{_libdir}/libhawkey.so.*

%files devel
%{_libdir}/libhawkey.so
%{_libdir}/pkgconfig/hawkey.pc
%{_includedir}/hawkey/
%{_mandir}/man3/hawkey.3.gz

%files -n python-hawkey
%{python_sitearch}/

%if %{with python3}
%files -n python3-hawkey
%{python3_sitearch}/
%exclude %{python3_sitearch}/hawkey/__pycache__
%exclude %{python3_sitearch}/hawkey/test/__pycache__
%endif

%changelog
* Tue Sep 22 2015 Michal Luscon <mluscon@redhat.com> 0.6.1-1
- fixed memleaks from 2f5b9af (Jan Silhan)
- Support list of strings in provides/requires. (RhBug:1243005)(RhBug:1243002)
  (Valentina Mukhamedzhanova)
- Add globbing support to dependency queries.
  (RhBug:Related:1259650)(RhBug:Related:1249073) (Valentina Mukhamedzhanova)
- package: filter out solvable:prereqmarker (RhBug:1186721) (Michal Luscon)
- skip already filtered items (Michael Mraka)

* Fri Aug 07 2015 Jan Silhan <jsilhan@redhat.com> 0.6.0-1
- Fixed a tiny typo in the RuntimeException message. (Yavor Atanasov)
- query: return empty query if we can't make reldep (RhBug:1244544) (Igor
  Gnatenko)
- Add support for little endian MIPS (Michal Toman)
- py: goal.run supports ignore_weak_deps param (Related:RhBug:1221635) (Jan
  Silhan)
- goal: added HY_IGNORE_WEAK_DEP flag (Jan Silhan)

* Tue Jul 28 2015 Michal Luscon <mluscon@redhat.com> 0.5.9-3
- Add explicit values to all public enumerations (Colin Walters)
- types: Revert unintentional ABI break in _hy_key_name_e (RhBug:1247335)
  (Colin Walters)

* Tue Jul 21 2015 Jan Silhan <jsilhan@redhat.com> 0.5.9-2
- spec: builds python3-hawkey in Fedora distro (Jan Silhan)

* Fri Jul 17 2015 Michal Luscon <mluscon@redhat.com> 0.5.9-1
- don't require python3 in rhel (Jan Silhan)
- depracate hy_goal_has* functions and Goal.req_has_* methods (Jan Silhan)
- goal: py: implemented __deepcopy__ (Jan Silhan)
- goal: implement clone (Jan Silhan)
- goal: py: added actions attribute (Jan Silhan)
- goal: added hy_goal_has_action function (Jan Silhan)
- Add weak deps queries (Michal Luscon)
- spec: fix the command that starts Python 3 tests (Radek Holy)

* Thu Jun 04 2015 Jan Silhan <jsilhan@redhat.com> 0.5.8-1
- added implicit-function-declaration compile flag (Jan Silhan)
- subject: Fix compiler warning introduced by previous commit (Colin Walters)
- python: Verify that nosetest actually ran any tests (Colin Walters)
- AUTHORS: updated (Jan Silhan)
- subject: Remove internal header includes from public header (Colin Walters)
- maintain result map in query (RhBug:1049205) (Jan Silhan)
- AUTHORS: updated (Jan Silhan)
- package: Don't assume the same pool in hy_package_cmp() (Matthew Barnes)

* Wed May 20 2015 Michal Luscon <mluscon@redhat.com> 0.5.7-1
- spec: add a %%{snapshot} macro for easier snapshot building (Radek Holy)
- doc: sack: deep copy added to warning section (Jan Silhan)
- doc: sack: warning about multiple Sack usage (Jan Silhan)
- doc: sack: len(sack) -> method __len__ (Jan Silhan)
- Package.files returns list of Unicode objects (Jan Silhan)

* Thu May 07 2015 Michal Luscon <mluscon@redhat.com> 0.5.6-1
- Revert "sack: force recomputing excludes" (RhUbg:1218650) (Jan Silhan)

* Wed Apr 29 2015 Michal Luscon <mluscon@redhat.com> 0.5.5-1
- get rid of yum references (Jan Silhan)
- sack: force recomputing excludes (Jan Silhan)
- doc: cosmetic: made Sack headline more readable (Jan Silhan)
- doc: sack: warning about using excludes, includes, disabling and enabling
  repos (Jan Silhan)
- cosmetic: removed commented code (Jan Silhan)
- sack: calls reinitiate provides after changing considered map (RhBug:1099342)
  (Jan Silhan)
- fixed memleak from d8f2ca7 (Jan Silhan)
- doc: add to CMDLINE_REPO_NAME and SYSTEM_REPO_NAME the Python API reference
  manual. (Radek Holy)
- doc: add Repo to the Python API reference manual. (Radek Holy)
- updated load_test_repo() to be able to load non-standard system repo (Michael
  Mraka)
- python tests for goal.run(verify=True) (Michael Mraka)
- test for HY_VERIFY flag (Michael Mraka)
- introduced verify option for goal.run() (Michael Mraka)
- AUTHORS: fixed name (Michael Mraka)
- AUTHORS: added 3 Michaels (Jan Silhan)
- Build for x86_64, correction for C++ (Michal Ruprich)

* Tue Mar 31 2015 Michal Luscon <mluscon@redhat.com> 0.5.4-1
- setup tito to bump version in VERSION.cmake (Michal Luscon)
- initialize to use tito (Michal Luscon)
- prepare repo for tito build system (Michal Luscon)
- New version 0.5.4 (Michal Luscon)
- goal: implement methods for optional installation (RhBug:1167881) (Michal Luscon)
- setup tito to bump version in VERSION.cmake (Michal Luscon)
- initialize to use tito (Michal Luscon)
- prepare repo for tito build system (Michal Luscon)
- New version 0.5.4 (Michal Luscon)
- goal: implement methods for optional installation (RhBug:1167881) (Michal Luscon)

* Wed Mar 25 2015 Jan Silhan <jsilhan@redhat.com> - 0.5.3-3
- new release

* Mon Feb 23 2015 Jan Silhan <jsilhan@redhat.com> - 0.5.3-2
- bumped release to be greater than f21 release
- Add Peter to Authors (Peter Hjalmarsson)
- Add support for armv6hl (Peter Hjalmarsson)

* Wed Feb 4 2015 Jan Silhan <jsilhan@redhat.com> - 0.5.3-1
- README: made readthedoc documentation official (Jan Silhan)
- sack: deprecation of create_cmdline_repo (Jan Silhan)
- does not break Sack.__init__ API from 8ce3ce7 (Jan Silhan)
- doc: document the new logdir parameter of Sack.__init__. (Radek Holy)
- New version: 0.5.3 (Jan Silhan)
- apichange: sack: added optional param logdir (Related:RhBug:1175434) (Jan Silhan)
- apichange: py: rename: Sack.cache_path -> Sack.cache_dir (Radek Holy)
- doc: add Sack to the Python API reference manual. (Radek Holy)
- cosmetic: autopep8 applied on __init__.py (Jan Silhan)
- query: support multiple flags in filter (RhBug:1173027) (Jan Silhan)
- packaging: make the spec file compatible with GitHub packaging guideliness. (Radek Holy)
- New version: 0.5.2 (Michal Luscon)
- hy_chksum_str() returns NULL in case of incorrect type (Michal Luscon)
- Fix defects found by coverity scan (Michal Luscon)
- selector: allow selecting provides with globs (RhBug: 1148353) (Michal Luscon)
- py: nevra_init() references possibly uninitialized variable. (Ales Kozumplik)
- package: add weak deps attributes. (Ales Kozumplik)

* Thu Sep 18 2014 Aleš Kozumplik <ales@redhat.com> - 0.5.1-1
- pool_split_evr() assert if we hit unexpected data. (Related:RhBug:1141634) (Ales Kozumplik)
- README: changed references to new repo location (Jan Silhan)
- iutil-py: removed pyseq_to_packagelist function (Jan Silhan)
- improved performance of python sequence iteration (RhBug:1109554) (Jan Silhan)
- reldep: constructor accepts unicode strings (RhBug:1124968) (Jan Silhan)
- Fix pool_split_evr's handling of EVRs without releases. (Radek Holy)
- added sha512 support (RhBug:1082658) (Jan Silhan)
- cosmetic: removed unneeded semicolon (Jan Silhan)
- goal: does not raise exception on empty selector (Related:RhBug:1127206) (Jan Silhan)

* Tue Aug 12 2014 Aleš Kozumplik <ales@redhat.com> - 0.5.0-1
- sack: include directive support added (Related:RhBug:1055910) (Jan Silhan)
- sack: using pool->considered instead of SOLVER_LOCK for excludes (RhBug:1099342) (Jan Silhan)
- cosmetic: replaced fail_unless with ck_assert_int_eq (Jan Silhan)

* Mon Jul 28 2014 Aleš Kozumplik <ales@redhat.com> - 0.4.19-1
- packaging: bump the SONAME as there are dropped API calls. (Ales Kozumplik)
- Support package splitting via obsoletes. (RhBug:1107973) (Ales Kozumplik)
- api change: py: convert Advisory, AdvisoryRef and AdvisoryPkg attributes to Unicode. (Radek Holy)
- hy_err_str: it's best to make it static. (Ales Kozumplik)
- Hide hy_err_str from errno.h (Ales Kozumplik)
- py: detailed error reporting. (Ales Kozumplik)
- doc: deprecation policy. (Ales Kozumplik)

* Wed Jul 16 2014 Aleš Kozumplik <ales@redhat.com> - 0.4.18-1
- api change: py: deprecate _hawkey.Advisory.filenames. (Radek Holy)
- api change: drop deprecated hy_package_get_update_*. (Radek Holy)
- api change: deprecate hy_advisory_get_filenames. (Radek Holy)
- tests: py: add tests for _hawkey.AdvisoryPkg type. (Radek Holy)
- py: add _hawkey.Advisory.packages attribute. (Radek Holy)
- py: add _hawkey.AdvisoryPkg type. (Radek Holy)
- tests: add tests for advisorypkg object. (Radek Holy)
- Add hy_advisory_get_packages method. (Radek Holy)
- Add advisorypkglist object. (Radek Holy)
- Add advisorypkg object. (Radek Holy)
- selector: added file filter (Related: RhBug:1100946) (Jan Silhan)
- priorities: change the meaning of the setting---lower number=better prio. (Ales Kozumplik)
- py: better error checking in repo-py.c:set_int(). (Ales Kozumplik)
- py: api: hawkey.Repo() does not accept cost keyword arg. (Ales Kozumplik)
- fix: nevra: hy_nevra_cmp (Jan Silhan)
- repos: priorities. (Ales Kozumplik)
- py3: Sack: accepts unicoded cachedir (Related: RhBug:1108908) (Jan Silhan)

* Thu Jul 3 2014  Aleš Kozumplik <ales@redhat.com> - 0.4.17-1
- sack: add a public function to get the running kernel package. (Ales Kozumplik)
- query: fix querying for string provides. (RhBug:1114483) (Ales Kozumplik)
- fix: commandline RPMs do not provide their files (RhBug:1112810) (Ales Kozumplik)
- tests: prevent automatic Python deps in tour.rpm. (Ales Kozumplik)
- deepcopy of sack raises error (RhBug:1059149) (Jan Silhan)

* Tue May 27 2014 Aleš Kozumplik <ales@redhat.com> - 0.4.16-1
- py3: use sphinx-build-3 (which doesn't encode the minor py version) (RhBug:1098098) (Ales Kozumplik)
- tests: fix test_list_arches(), there's new architectures listed now. (Ales Kozumplik)
- doc: packaging: add license block to each .rst. (Ales Kozumplik)
- Subject: accepts/returns pattern in unicode (Related: RhBug:1092777) (Jan Silhan)
- fix kernel detection by being more strict what we look for. (RhBug:1087534) (Ales Kozumplik)
- [kernel] look for the installed kernel in @System. (Ales Kozumplik)
- fix: sigsegv when wrong reldep is passed to selector as a provide. (Ales Kozumplik)
- .gitignore: __pycache__ (Ales Kozumplik)
- installonlies: log the discovered running kernel. (Ales Kozumplik)
- py: Package: all string attributes are in Unicode (RhBug:1093887) (Jan Silhan)
- py: fix: certain string assignments should raise TypeError, never SystemError. (Ales Kozumplik)
- cleanup: tweak declarations in pycomp.h. (Ales Kozumplik)
- goal: hy_goal_write_debugdata() takes target dir parameter. (Ales Kozumplik)
- iutil.c: simplify queue2plist() implementation. (Ales Kozumplik)
- Goal: listing unneeded packages. (Ales Kozumplik)
- doc: make the documentation generation independent of hawkey the python module. (Ales Kozumplik)
- removed unused imports (Jan Silhan)
- distro-sync doesn't replace arch (RhBug:1054909) (Jan Silhan)
- replaced deprecated fail_unless with ck_assert_int_eq (Jan Silhan)
- added python bindings to nevra attribute filter (Jan Silhan)
- added nevra filter flag to query C API (Jan Silhan)

* Fri May 2 2014 Aleš Kozumplik <ales@redhat.com> - 0.4.14-1
- py: cosmetic: drop py_ prefixes from static functions in hawkeymodule.c. (Ales Kozumplik)
- Expose hy_arch_detect(). (Ales Kozumplik)
- fixed clang warning of uninitialized variable (Jan Silhan)
- doc: fixed typo (Jan Silhan)
- logging: log checksums of written/loaded repositories. (RhBug:1071404) (Ales Kozumplik)
- logging hawkey version number. (Ales Kozumplik)
- implement updateinfo caching (Michael Schroeder)
- cosmetic: fixed sliced changelog lines in spec file (Jan Silhan)

* Fri Apr 11 2014 Jan Silhan <jsilhan@redhat.com> - 0.4.13-1
- Add forgotten queue_free()s from bd3a2ae. (Ales Kozumplik)
- cosmetic: some cleanups of 0e4327c. (Ales Kozumplik)
- refactor rewrite_repos function (Michael Schroeder)
- rewrite repos after calling addfileprovides (Michael Schroeder)
- also set the repodata id if an extension is loaded from the cache (Michael Schroeder)
- call hy_repo_link when setting the appdata of the system repo (Michael Schroeder)
- use REPO_LOCALPOOL when loading the filelist extension (Michael Schroeder)
- switch over to the written solv files to save memory (RhBug:1084174) (Michael Schroeder)
- py: add downgradable and upgradable kwargs to _hawkey.Query.filter. (Radek Holy)
- Fix comments in query.c (Radek Holy)
- Add hy_query_filter_downgradable and hy_query_filter_upgradable. (Radek Holy)
- tests: py: add tests for _hawkey.AdvisoryRef type. (Radek Holy)
- tests: py: add tests for hawkey.Advisory type. (Radek Holy)
- py: add _hawkey.Package.get_advisories method. (Radek Holy)
- py: add _hawkey.Advisory type. (Radek Holy)
- py: add _hawkey.AdvisoryRef type. (Radek Holy)
- tests: add tests for advisoryref object. (Radek Holy)
- tests: add tests for advisory object. (Radek Holy)
- api change: deprecate hy_package_get_update_*. (Radek Holy)
- Add hy_package_get_advisories method. (Radek Holy)
- Add advisoryreflist object. (Radek Holy)
- Add advisoryref object. (Radek Holy)
- Add advisorylist object. (Radek Holy)
- Add advisory object. (Radek Holy)
- Rename SOLVABLE_NAME_UPDATE_PREFIX to SOLVABLE_NAME_ADVISORY_PREFIX. (Radek Holy)
- sack: Also look in /usr/share/rpm for Packages (Colin Walters)
- py: add load_updateinfo kwarg to _hawkey.Sack.load_yum_repo. (Radek Holy)
- py: add _hawkey.Repo.updateinfo_fn getsetter. (Radek Holy)
- py: more detailed error string in Sack.add_cmdline_package(). (Ales Kozumplik)
- Fix hy_query_run to list only packages. (Radek Holy)
- Fix goal to add only packages if name glob selector is given. (Radek Holy)
- Fix hy_goal_run_all_flags to resolve only package installonlies. (Radek Holy)
- Fix sack_knows to check packages only. (Radek Holy)
- Add is_package function. (Radek Holy)
- Fix typo in filter_rco_reldep's assertion. (Radek Holy)

* Fri Mar 14 2014 Jan Silhan <jsilhan@redhat.com> - 0.4.12-1
- Fix hy_stringarray_length. (Radek Holy)
- tests: bring tests/repos/yum/recreate to a workable state again. (Ales Kozumplik)
- added distupgrade function (Related:963710) (Jan Silhan)
- remove: _HyPackageList.left. (Ales Kozumplik)
- goal: track changes (as reinstalls) (RhBug:1068982) (Ales Kozumplik)

* Mon Feb 24 2014 Aleš Kozumplik <akozumpl@redhat.com> - 0.4.11-1
- fixed typos in tutorial-py.rst (Jan Silhan)
- added glob pattern search for arch to nevra_possibilities_real (RhBug:1048788) (Jan Silhan)
- Left behind references to README.md from 3b47a13. (Ales Kozumplik)
- Add Radek to AUTHORS. (Ales Kozumplik)
- update the README. (Ales Kozumplik)
- sack: write_*() should also check fclose(). (Ales Kozumplik)

* Mon Feb 17 2014 Radek Holý <rholy@redhat.com> - 0.4.10-1
- tests: add a negative test for reponame. (Radek Holy)
- Add reponame into selector. (Radek Holy)
- write_main() and write_ext(): even on error do not leave the temporary file behind. (Ales Kozumplik)
- write_main() should do a better job erroring out on write errors. (Ales Kozumplik)
- Fix vsnprintf SIGSEGV passing "%s" with no va_list args to pool_debug. (RhBug:1064459) (Ales Kozumplik)
- Save the cache atomically. (RhBug:1047087) (Ales Kozumplik)
- package: call repo_internalize_trigger in get_files() (RhBug:1062703) (Ales Kozumplik)
- fixed reldep pointer NULL comparison (Jan Silhan)
- fixed indentation in subject-py.c (Jan Silhan)
- moved TEST_COND macro to iutil-py.h (Jan Silhan)
- moved subject and nevra from python to C (Jan Silhan)
- subject in C: work with full reldeps (Jan Silhan)

* Thu Jan 30 2014 Aleš Kozumplík <ales@redhat.com> - 0.4.9-1
- selectors: allow selecting provides with full Reldep string. (Ales Kozumplik)
- subject: work with full reldeps (containing the CMP flags). (Ales Kozumplik)
- package: hy_package_get_hdr_end(). (Ales Kozumplik)
- added subject C API (Jan Silhan)
- added nevra C API (Jan Silhan)
- fix not accepting numeric version in reldep (RhBug:1052961) (Jan Silhan)
- fix Reldep inicialization without sack crash (RhBug:1052947) (Jan Silhan)
- tests: make test_goal_selector_upgrade() less assuming. (Ales Kozumplik)

* Tue Jan 21 2014  Aleš Kozumplík <ales@redhat.com> - 0.4.8-1
- installonlies: erase packages depending on a kernel to be erased. (RhBug:1033881) (Ales Kozumplik)
- fix: latest_per_arch on incompatible arches. (RhBug:1049226) (Ales Kozumplik)

* Tue Dec 17 2013  Aleš Kozumplík <ales@redhat.com> - 0.4.7-1
- Fix malfunction of Package.__lt__ and Package.__gt__ (RhBug:1014963) (Radek Holy)
- Do not crash when querying provides that do not exist (Richard Hughes)

* Wed Dec 4 2013  Aleš Kozumplík <ales@redhat.com> - 0.4.6-1
- remove: packageDelta_new (Zdenek Pavlas)
- get_delta_from_evr(): create the python object only when delta exists (Zdenek Pavlas)
- fix pycomp_get_string(), pycomp_get_string_from_unicode() (Zdenek Pavlas)
- fix get_str() in packagedelta-py (Zdenek Pavlas)
- fix: spec: running tests in python3 after build (Jan Silhan)
- tests: order packages in .repo files by name. (Ales Kozumplik)
- fix: goal: reason for installing when more packages are available to a selector. (Ales Kozumplik)
- tests: add a package that is not installed yet available in main, updates. (Ales Kozumplik)
- add hy_packagedelta_get_chksum() (Zdenek Pavlas)
- add hy_packagedelta_get_downloadsize() (Zdenek Pavlas)
- add hy_packagedelta_get_baseurl() (Zdenek Pavlas)
- test_query_provides_in: avoid ck_assert_int_eq() as it evaluates args twice (Zdenek Pavlas)
- installonlies: fix sorting packages depending on the running kernel. (Ales Kozumplik)
- use pool_lookup_deltalocation() (Zdenek Pavlas)
- initialize _hawkey.PackageDelta type (Zdenek Pavlas)
- delta_create(): fix the sizeof() (Zdenek Pavlas)
- parse_reldep_str(): fix buffer overflow (Zdenek Pavlas)
- string reldep parsing using parse_reldep_str (Jan Silhan)
- added hy_query_filter_provides_in function (RhBug:1019168) (Jan Silhan)
- added parse_reldep_str function (Jan Silhan)
- fix: py: abort() from python when writing the system .solv cache fails. (Ales Kozumplik)
- fix forgotten include causing a compiler warning in testsys.c. (Ales Kozumplik)

* Fri Nov 8 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.5-1
- goal: installonly_limit = 0 means it is disabled. (Ales Kozumplik)
- written API changes for Query filter latest option (RhBug:1025650) (Jan Silhan)
- tests: superfluous query.run() calls. (Ales Kozumplik)
- removed define PyString_AsString in pycomp.h (Jan Silhan)
- replaced PyInt_FromLong with PyLong_FromLong (Jan Silhan)
- replaced PyInt_AsLong with PyLongAs_Long (Jan Silhan)
- added latest to query ignoring architectures (Jan Silhan)
- renamed hy_query_filter_latest to hy_query_filter_latest_per_arch (Jan Silhan)
- logging: additional logging output on repo loading errors. (Ales Kozumplik)
- logging: refactor and add a loglevel. (Ales Kozumplik)
- queries: allow glob matching in query. (Ales Kozumplik)
- tests: slightly simplify test_subject.py. (Ales Kozumplik)
- subject: yield correct results when globbing over a version. (Ales Kozumplik)
- subject: globbing for sack._knows. (Ales Kozumplik)
- py: subject: sack._knows doesn't need to take sack. (Ales Kozumplik)

* Tue Oct 29 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.4-1
- With the current libsolv there's no need to reinit solver for re-resolving. (Ales Kozumplik)
- speedup fetching rpmdb a bit by reusing what we can from the old cache. (Ales Kozumplik)
- adapt to libsolv 3b3dd72: obsoleting by an installonly package is erasing. (Ales Kozumplik)
- tests: slim test_goal.c by using a testsys function instead of its reimplementation. (Ales Kozumplik)
- tests: shave some lines off test_goal.c by using smarter Goal results assertion. (Ales Kozumplik)
- installonlines: python bindings for installonly_limit. (Ales Kozumplik)
- goal: when sorting the installonly candidates, consider the running kernel. (Ales Kozumplik)
- Limit the number of installed installonlies. (RhBug:880524) (Ales Kozumplik)
- iutil.c: dump_solvables_queue. (Ales Kozumplik)
- refactor: concentrate all libsolv solver initialization into the static solve(). (Ales Kozumplik)
- refactor: goal: reinit_solver() (Ales Kozumplik)
- tests: dump_packagelist() can free the list too. (Ales Kozumplik)
- iutil: running_kernel(). (Ales Kozumplik)

* Tue Oct 15 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.3-1
- methods get_delta_from_evr from package and add_cmdline_package from sack can take unicode string as argument (Jan Sil
- tests: move TestSack out of the testing module into tests. (Ales Kozumplik)

* Mon Sep 30 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.2-1.git4c51f65
- Goal: excluding and then installing results in incomprehenisble problem desc. (RhBug:995459) (Ales Kozumplik)
- added support of cost option in repos (Jan Silhan)

* Mon Sep 16 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.1-1.git6f35513
- spec file also generates python3-hawkey rpm (Jan Silhan)
- fixed package object rich comparision (Jan Silhan)
- Add libsolv-devel as a hard requires for hawkey-devel (Richard Hughes)
- Python 3 bindings added (Jan Silhan)

* Wed Jul 31 2013 Aleš Kozumplík <ales@redhat.com> - 0.4.0-1.git0e5506a
- Detect the variant of armv7l. (RhBug:915269) (Ales Kozumplik)
- add package.downloadsize and package.installsize. (Ales Kozumplik)

* Mon Jul 22 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.16-1.git4e79abc
- Correctly find the installed package when looking for updates (Richard Hughes)
- Change the hy_package_get_update_severity() API to return an enum value (Richard Hughes)
- Do not enforce all repos load all kinds of specified metadata (Richard Hughes)
- Fix a tiny memory leak introduced in 68ebca4a80aec636d30a9fd4fb9aa2d9bf9a8eca (Richard Hughes)
- Add methods to get details about package updates (Richard Hughes)
- Add updateinfo support to hawkey, using the existing parser in libsolv (Richard Hughes)
- Do not count updates when counting the number of packages in a sack (Richard Hughes)

* Wed Jul 17 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.15-1.git996cd40
- py: fix memory leak in sack-py.c:new_package (Ales Kozumplik)
- rebuild the package, the previous version does not correspond to an existing commit.

* Mon Jun 24 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.14-1.git78b3aa0
- tests: test_get_files(): test against a package with files outside /usr/bin and /etc. (Ales Kozumplik)
- py: simplify exception throwing in load_system_repo(). (Ales Kozumplik)
- Install stringarray.h so client programs can use hy_stringarray_free() (Richard Hughes)
- py: bindings for package.files. (Ales Kozumplik)
- add hy_package_get_files. (Ales Kozumplik)
- Fix three trivial comment mis-spellings (Richard Hughes)
- Set required python version to 2 (Richard Hughes)
- Add a HY_VERSION_CHECK macro (Richard Hughes)
- packaging: add license information to every file. (Ales Kozumplik)
- py: add 'installed' property to hawkey.Package (Panu Matilainen)
- tests: add test-case for hy_package_installed() (Panu Matilainen)
- Add .baseurl getter to Python and C APIs. (Zdenek Pavlas)
- tests: fix a memory leak revealed by libsolv commit 0804020. (Ales Kozumplik)
- Return the installed size for installed packages in hy_package_get_size() (Panu Matilainen)
- Add a function for determining whether HyPackage is installed or not (Panu Matilainen)

* Mon May 27 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.13-2.git15db39f
- goal: running the same Goal instance twice or more. (Ales Kozumplik)
- sack._knows can now determine if a particular 'name-version' sounds familiar. (Ales Kozumplik)
- Goal: do not set the 'keepexplicitobsoletes' flag. (Ales Kozumplik)
- tests: fixtures for upgrade_all() with installonly packages. (Ales Kozumplik)

* Mon May 13 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.12-1.git60cc1cc
- goal: fix assertions about the job queue when translating selectors. (Ales Kozumplik)
- SOLVER_NOOBSOLETES is SOLVER_MULTIVERSION. (Ales Kozumplik)
- goal: testing number of requests and presence of certain kinds of requests. (Ales Kozumplik)

* Thu May 2 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.11-1.gitffe0dac
- obsoletes: do not report obsoleted packages in hy_goal_list_erasures(). (Ales Kozumplik)
- rename: goal: list_obsoletes -> list_obsoleted. (Ales Kozumplik)
- rename: hy_goal_package_all_obsoletes() -> hy_goal_list_obsoleted_by_package(). (Ales Kozumplik)
- apichange: remove: hy_goal_package_obsoletes(). (Ales Kozumplik)
- tests: simplify test_goal_upgrade_all() somehwat. (Ales Kozumplik)
- goal: add ability to list all of the package's and transaction's obsoletes. (Ales Kozumplik)
- py: allow directly comparing NEVRAs by their EVRs. (RhBug:953203) (Ales Kozumplik)
- add hy_sack_evr_cmp(). (Ales Kozumplik)
- py: fix SIGSEGV in unchecked hy_goal_describe_problem() call. (Ales Kozumplik)
- doc: update the Tutorial for the current version of the API. (Ales Kozumplik)
- subject parsing: recognize "pyton-hawkey" is a name in "python-hawkey-0.3.10". (Ales Kozumplik)

* Mon Apr 8 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.10-1.git1d51b83
- hy_goal_write_debugdata() (Ales Kozumplik)

* Wed Mar 20 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.9-1.gitc0c16c0
- refactoring: hy_sack_get_cache_path -> hy_sack_get_cache_dir. (Ales Kozumplik)
- hy_sack_create() now accepts a flag to disable automatic cachedir creation. (Ales Kozumplik)
- fix crashes when the logfile can not be initialized. (Ales Kozumplik)

* Fri Mar 1 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.8-1.git046ab1c
- py: expose Subject.pattern (Ales Kozumplik)
- doc: added the rootdir parameter to hy_sack_create(). (Ales Kozumplik)
- sack: allow specifying a different rootdir (AKA "installroot") (Ales Kozumplik)
- Forms recognized by ``Subject`` are no longer an instance-scope setting. (RhBug:903687) (Ales Kozumplik)

* Mon Feb 11 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.7-2.gitdd10ac7
- Selector: allow constraining by version only (without the release). (Ales Kozumplik)
- python: reldep_repr() outputs a valid number. (Ales Kozumplik)
- Add pkg.conflicts and pkg.provides. (RhBug:908406) (Ales Kozumplik)
- hy_query_filter_requires() internally converts to a reldep. (Ales Kozumplik)
- support filtering by 'obsoletes' and 'conflicts' reldeps. (RhBug:908372) (Ales Kozumplik)
- allow filtering by requires with reldeps. (RhBug:908372) (Ales Kozumplik)
- py: Query.filter() returns instance of the same type as the original query. (Ales Kozumplik)
- sack_knows() does case-insensitive matching too (pricey yet needed). (Ales Kozumplik)
- subject: best shot at 'some-lib-devel' is not that EVR is 'lib-devel'. (RhBug:903687) (Ales Kozumplik)

* Wed Jan 30 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.6-2.gita53a6b1
- subject: best shot at 'some-lib-devel' is not that EVR is 'lib-devel'. (Ales Kozumplik)
- cosmetic: put HY_PKG_LOCATION into the lists alphabetically. (Ales Kozumplik)
- New key HY_PKG_LOCATION for query (Tomas Mlcoch)
- querying for upgrades: do not include arbitrary arch changes. (Ales Kozumplik)

* Fri Jan 18 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.6-1.gitc8365fa
- excludes: Query respects the exclude list. (related RhBug:884617)
- excludes: apply excludes in Goal. (related RhBug:884617)
- goal: support forcebest flag. (related RhBug:882211)
- disabling/enabling entire repositories.
- selector: preview possibly matched packages with hy_selector_matches(). (related RhBug:882851)

* Thu Jan 3 2013 Aleš Kozumplík <ales@redhat.com> - 0.3.5-3.gitf981c48
- Rebuild with proper git revision.

* Fri Dec 21 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.5-1.gitd735540
- Move to libsolv-0.2.3 (suit minor API change there)

* Mon Dec 17 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.4-1.gitb3fcf21
- Subject: infrastructure for discovering NEVRA explanations of what user's input meant.
- fix: cloning an evaluated Query should copy the result set too.
- Reldeps: creating custom-specified reldeps (name, evr).
- Goal: accept a selector targeting a provide.
- delete goal_internal.h, not needed.
- Goal: give the solver SOLVER_FLAG_ALLOW_VENDORCHANGE (RhBug:885646)
- fix crash when hash for an invalid Reldep is requested.

* Mon Nov 26 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.3-1.git4e41b7f
- Python: improve Query result caching (uses the C facility now).
- packageset: add internal function for getting elements with a hint.
- Python, performance: Query.run() internally uses a set for the results instead of a list.
- Query: fix selecting upgrades for packages of changing architecture.
- Goal: add upgrade_to_selector() (EVR specs in selectors)
- checksums: do not assert() when the pkg hasn't got the asked checksum. (RhBug:878823)
- API change: rename: hy_package_get_nvra() -> hy_package_get_nevra().
- Goal: support distupgrade of all packages.

* Thu Nov 15 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.2-1.gite883549
- fix: hy_package_cmp() shouldn't compare packages of different arch equal.
- Goal: support reinstalls.

* Thu Nov 8 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.1-2.git6f9df85
- py: add __all__ to the hawkey module.
- API cleanup: give checksumming functions the 'hy_' prefix.
- Add HyPackageSet.
- Make hy_query_filter_package_in() general enough to handle the relations too.
- Py: filter by relation and a set of target packages.
- remove: hy_query_filter_obsoleting().
- query: implement an empty Query filter.
- Add the reldep objects, reldep containers, and hy_package_get_requires(). (RhBug:847006)
- Query: filter provides by reldeps. (RhBug:847006)
- cleanup header files inclusions.
- py: fix memory leak package_str().
- hy_package_get_obsoletes().
- Query: filter with ORed reldep lists.

* Wed Oct 17 2012 Aleš Kozumplík <ales@redhat.com> - 0.3.0-1.gitafa7717
- API change: Query: repo filter is called REPONAME now, now just REPO.
- python: isinstance check for hawkey.Package fails for package objects.
- Simplification of archive script (tmlcoch)
- API change: hy_repo_create() now takes the repo name as a parameter.
- API change: Use Selector for what used to be "Query installs".
- py: use general keyword arguments to Goal.install() etc. to construct a Selector.
- goal: improve error reporting when Goal failed/was not executed.
- selectors: glob matching the package name.

* Fri Oct 5 2012 Aleš Kozumplík <ales@redhat.com> - 0.2.12-2.git7fa7aa9
- fix sigsegv in query.c:filter_sourcerpm().
- doc: move the hawkey reference to man section 3.
- query: filter by description or URL.
- fix: FOR_PACKAGELIST(pkg,list,i) offsets the 'i' by one.
- Query: hy_query_filter_package_in() limits filtering to an arbitrary set of pkgs.
- Query: filtering by epoch.
- py: Query: make sure filterm() clears the result cache.
- py: fix: memory leaks with PySequence_GetItem().

* Sat Sep 22 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.11-4.git687ceab
- py: hawkey.test should not depend on libcheck.so.

* Fri Sep 21 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.11-1.git545a461
- py: Goal.run_all() returns True if a solution was found. (RhBug: 856615)
- py: Goal.run() accepts callback parameter too. (RhBug: 856615)
- query: filtering by version and release. (RhBug: 856612)
- Flag an error if Sack is created with an invalid arch. (RhBug: 857944)
- fix hy_get_sourcerpm() when the package has no sourcerpm. (RhBug: 858207)
- Query: filter by source rpm. (RhBug: 857941)
- Run 'make check' when building the RPM.

* Mon Sep 10 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.10-2.gita198dea
- Fix build that now needs python-sphinx.

* Thu Aug 30 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.10-1.gita198dea
- Query cloning.
- Query: full version filtery is supported now.
- py: query.filter() now returns a cloned Query.
- py: len(query) and bool(query) now work as expected.

* Thu Aug 23 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.9-2.gitefeb04c
- Add manpage.

* Thu Aug 23 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.9-1.git8599c55
- Finding all solutions in Goal.
- hy_goal_reason() no longer depends on Fedora-specific hacks in libsolv.
- hy_package_get_sourcerpm()

* Mon Aug 6 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.8-1.gite6734fb
- repo loading API changed, hy_sack_load_yum_repo() now accepts flags to build
  cache, load filelists, etc.
- fixed 843487: hawkey query.filter() ends with assertion.

* Tue Jul 24 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.7-1.git41b39ba
- Package description, license, url support.
- python: Unicode fixes in Query.

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.2.6-3.gitea88ad5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon Jul 16 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.6-2.gitea88ad5
- HY_CLEAN_DEPS support.

* Mon Jul 16 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.6-1.git76a5b8c
- Use libsolv-0.0.0-13.
- hy_goal_get_reason().

* Sun Jul 1 2012 Aleš Kozumplík <akozumpl@redhat.com> - 0.2.5-1.git042738b
- Use libsolv-0.0.0-12.
- Added hy_package_get_hdr_checkum().

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
