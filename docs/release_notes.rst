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

###################
 LIBDNF Release Notes
###################

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
