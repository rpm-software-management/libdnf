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

************
Repositories
************

.. class:: hawkey.Repo

  Instances of :class:`hawkey.Repo` point to metadata of packages that are
  available to be installed. The metadata are expected to be in the "rpm-md"
  format. `librepo <https://github.com/Tojaj/librepo>`_ may help you with
  downloading the required files.

  .. attribute:: cost

    An integer specifying a relative cost of accessing this repository. This
    value is compared when the priorities of two repositories are the same. The
    repository with *the lowest cost* is picked. It is useful to make the
    library prefer on-disk repositories to remote ones.

  .. attribute:: filelists_fn

    A valid string path to the readable "filelists" XML file if set.

  .. attribute:: name

    A string name of the repository.

  .. attribute:: presto_fn

    A valid string path to the readable "prestodelta.xml" (also called
    "deltainfo.xml") file if set.

  .. attribute:: primary_fn

    A valid string path to the readable "primary" XML file if set.

  .. attribute:: priority

    An integer priority value of this repository. If there is more than one
    candidate package for a particular operation, the one from the repository
    with *the lowest priority* value is picked, possibly despite being less
    convenient otherwise (e.g. by being a lower version).

  .. attribute:: repomd_fn

    A valid string path to the readable "repomd.xml" file if set.

  .. attribute:: updateinfo_fn

    A valid string path to the readable "updateinfo.xml" file if set.

  .. method:: __init__(name)

    Initialize the repository with empty :attr:`repomd_fn`, :attr:`primary_fn`,
    :attr:`filelists_fn` and :attr:`presto_fn`. The priority and the cost are
    set to ``0``.

    `name` is a string giving the name of the repository. The name should not
    be equal to :const:`hawkey.SYSTEM_REPO_NAME` nor
    :const:`hawkey.CMDLINE_REPO_NAME`
