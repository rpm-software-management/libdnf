..
  Copyright (C) 2017 Red Hat, Inc.

  This copyrighted material is made available to anyone wishing to use,
  modify, copy, or redistribute it subject to the terms and conditions of
  the GNU General Public License v.2, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY expressed or implied, including the implied warranties of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
  Public License for more details.  You should have received a copy of the
  GNU General Public License along with this program; if not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.  Any Red Hat trademarks that are incorporated in the
  source code or documentation are not subject to the GNU General Public
  License and may only be used or replicated with the express permission of
  Red Hat, Inc.

######################
 LIBDNF Release Notes
######################

====================
0.16.1 Release Notes
====================
* Implement 'module_hotfixes' conf option to skip filtering RPMs from hotfix repos.
* Fix distupgrade filter, allow downgrades.
* Module dependency resolution
* Platform pseudo-module based on /etc/os-release
* Add Goal::listSuggested()

====================
0.16.0 Release Notes
====================
* Fix RHSM plugin
* Add support for logging

====================
0.15.2 Release Notes
====================

Bugs fixed in 0.15.2:

* :rhbug:`1595487`

====================
0.15.0 Release Notes
====================

* Filtering rpms by module metadata
* New SWIG bindings
* New history database
* New config classes
* Query performance improvements
* New query filter nevra_strict

Bugs fixed in 0.15.0:

* :rhbug:`1498207`
* :rhbug:`1500361`
* :rhbug:`1486749`
* :rhbug:`1525542`
* :rhbug:`1550030`
* :rhbug:`1576749`
* :rhbug:`1537981`
* :rhbug:`1588443`
* :rhbug:`1565647`

====================
0.11.1 Release Notes
====================

* Improvement query performance
* Run file query in hy_subject_get_best_solution only for files (arguments that start with ``/`` or ``*/``)

Bugs fixed in 0.11.1:

* :rhbug:`1498207`

====================
0.10.1 Release Notes
====================

It improves query performance with ``name`` and ``arch`` filters. Also ``nevra`` filter will now
handle string with or without ``epoch``.
Additionally for python bindings it renames ``NEVRA._has_just_name()`` to ``NEVRA.has_just_name()``
due to movement of code into c part of library.

Bugs fixed in 0.10.1:

* :rhbug:`1260242`
* :rhbug:`1485881`
* :rhbug:`1361187`

===================
0.9.3 Release Notes
===================

It moves query glob optimization from python code to C part.

Bugs fixed in 0.9.3:

* :rhbug:`1381506`
* :rhbug:`1464249`

===================
0.1.7 Release Notes
===================
Released: 2014-12-19

Notes:
 - librepo >= 1.7.11 is now required

New Features:
 - Add HIF_SOURCE_UPDATE_FLAG_SIMULATE (Richard Hughes)
 - Add a large number of GPG tests (Richard Hughes)
 - Add hif_source_get_filename_md() (Richard Hughes)
 - Add the concept of metadata-only software sources (Richard Hughes)
 - Support appstream and appstream-icons metadata types (Richard Hughes)

Bugfixes:
 - Automatically import public keys into the librepo keyring (Richard Hughes)
 - Call hif_state_set_allow_cancel() when the state is uncancellable (Richard Hughes)
 - Correctly update sources with baseurls ending with a slash (Richard Hughes)
 - Don't unref the HifSource when invalidating as this is not threadsafe (Richard Hughes)
 - Fix crash when parsing the bumblebee.repo file (Richard Hughes)
 - Improve handling of local metadata (Richard Hughes)
 - Only set LRO_GPGCHECK when repo_gpgcheck=1 (Richard Hughes)

===================
0.1.6 Release Notes
===================
Released: 2014-11-10

New Features:
 - Add support for package reinstallation and downgrade (Michal Minar)
 - Copy the vendor cache if present (Richard Hughes)

Bugfixes:
 - Allow to get repo loader out of context (Michal Minar)
 - Ensure created directories are world-readable (Richard Hughes)
 - Support local repositories (Michal Minar)

===================
0.1.5 Release Notes
===================
Released: 2014-09-22

Bugfixes:
 - Add all native architectures for ARM and i386 (Richard Hughes)
 - Check for libQtGui rather than libkde* to detect GUI apps (Kevin Kofler)

===================
0.1.4 Release Notes
===================
Released: 2014-09-12

New Features:
 - Add hif_source_commit() so we don't rewrite the file for each change (Richard Hughes)
 - Allow setting the default lock directory (Richard Hughes)

Bugfixes:
 - Ensure all the required directories exist when setting up the context (Richard Hughes)
 - Use a real path for hy_sack_create() (Richard Hughes)

===================
0.1.3 Release Notes
===================
Released: 2014-09-01

Bugfixes:
 - Add an error path for when the sources are not valid (Richard Hughes)
 - Do not call hif_context_setup_sack() automatically (Richard Hughes)
 - Don't error out for missing treeinfo files (Kalev Lember)
 - Fix a logic error to fix refreshing with HIF_SOURCE_UPDATE_FLAG_FORCE (Richard Hughes)

===================
0.1.2 Release Notes
===================
Released: 2014-07-17

Notes:

New Features:
 - Add HifContext accessor in -private for HifState (Colin Walters)
 - Improve rpm callback handling for packages in the cleanup state (Kalev Lember)

Bugfixes:
 - Add name of failing repository (Colin Walters)
 - Create an initial sack in HifContext (Colin Walters)
 - Error if we can't find any package matching provided name (Colin Walters)
 - Fix a mixup of HifStateAction and HifPackageInfo (Kalev Lember)
 - Only set librepo option if value is set (Colin Walters)
 - Respect install root for rpmdb Packages monitor (Colin Walters)
 - Update Makefile.am (Elan Ruusamäe)

===================
0.1.1 Release Notes
===================
Released: 2014-06-23

New Features:
 - Only add system repository if it exists (Colin Walters)

Bugfixes:
 - Add private accessors for goal/sack (Colin Walters)
 - Fix a potential crash when removing software (Richard Hughes)
 - Pass install root to hawkey (Colin Walters)

===================
0.1.0 Release Notes
===================
Released: 2014-06-10

Notes:
 - This is the first release of a simple library that uses librepo and hawkey
   to do some high level package management tasks.
 - libhif is not 100% API or ABI stable yet.

New Features:
 - Add HifContext as a high level operation (Richard Hughes)

Bugfixes:
 - Add several g-i annotations (Colin Walters)
 - Correctly set the cleanup status (Kalev Lember)
 - Fix a crash when using hif_source_set_keyfile_data() (Richard Hughes)
 - Use GLib version macros to pin to 2.36 by default (Colin Walters)
