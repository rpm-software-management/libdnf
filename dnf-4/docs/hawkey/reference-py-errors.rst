..
  Copyright (C) 2015  Red Hat, Inc.

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

**************
Error handling
**************

When an error or an unexpected event occurs during a Hawkey routine, an
exception is raised:

* if it is a general error that could be common to other Python programs, one of
  the standard Python built-in exceptions is raised. For instance, ``IOError``
  and ``TypeError`` can be raised from Hawkey.

* programming errors within Hawkey that cause unexpected or invalid states raise
  the standard ``AssertionError``. These should be reported as bugs against
  Hawkey.

* programming errors due to incorrect use of the library usually produce
  ``hawkey.ValueException`` or one of its subclasses, ``QueryException`` (poorly
  formed Query) or ``ArchException`` (unrecognized architecture).

* sometimes there is a close call between blaming the error on an input
  parameter or on something else, beyond the programmer's
  control. ``hawkey.RuntimeException`` is generally used in this case.

* ``hawkey.ValidationException`` is raised when a function call performs a
  preliminary check before proceeding with the main operation and this check
  fails.

The class hierarchy for Hawkey exceptions is::

  +-- hawkey.Exception
       +-- hawkey.ValueException
       |    +-- hawkey.QueryException
       |    +-- hawkey.ArchException
       +-- hawkey.RuntimeException
       +-- hawkey.ValidationException
