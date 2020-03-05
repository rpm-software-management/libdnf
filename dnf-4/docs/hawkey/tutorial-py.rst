..
  Copyright (C) 2014-2015  Red Hat, Inc.

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

************************
 python-hawkey Tutorial
************************

.. contents::

.. IMPORTANT::

  Please consult every usage of the library with :doc:`reference-py` to be sure
  what are you doing. The examples mentioned here are supposed to be as simple
  as possible and may ignore some minor corner cases.

Setup
=====

First of, make sure hawkey is installed on your system, this should work from your terminal::

  >>> import hawkey

The Sack Object
===============

*Sack* is an abstraction for a collection of packages. Sacks in hawkey are
toplevel objects carrying much of hawkey's of functionality. You'll want to
create one::

   >>> sack = hawkey.Sack()
   >>> len(sack)
   0

Initially, the sack contains no packages.

Loading RPMDB
=============

hawkey is a lib for listing, querying and resolving dependencies of *packages*
from *repositories*. On most linux distributions you always have at least *the
system repo* (in Fedora it is the RPM database). To load it::

  >>> sack.load_system_repo()
  >>> len(sack)
  1683

Hawkey always knows the name of every repository. The system repository is always
set to :const:`hawkey.SYSTEM_REPO_NAME`. and the client is responsible for naming
the available repository metadata.

Loading Repositories
====================

Let's be honest here: all the fun in packaging comes from packages you haven't
installed yet. Information about them, their *metadata*, can be obtained from
different sources and typically they are downloaded from an HTTP mirror (another
possibilities are FTP server, NFS mount, DVD distribution media, etc.). Hawkey
does not provide any means to discover and obtain the metadata locally: it is up
to the client to provide valid readable paths to the repository metadata XML
files. Structures used for passing the information to hawkey are the hawkey
:class:`Repos <hawkey.Repo>`. Suppose we somehow obtained the metadata and placed it in
``/home/akozumpl/tmp/repodata``. We can then load the metadata into hawkey::

  >>> path = "/home/akozumpl/tmp/repodata/%s"
  >>> repo = hawkey.Repo("experimental")
  >>> repo.repomd_fn = path % "repomd.xml"
  >>> repo.primary_fn = path % "f7753a2636cc89d70e8aaa1f3c08413ab78462ca9f48fd55daf6dedf9ab0d5db-primary.xml.gz"
  >>> repo.filelists_fn = path % "0261e25e8411f4f5e930a70fa249b8afd5e86bb9087d7739b55be64b76d8a7f6-filelists.xml.gz"
  >>> sack.load_repo(repo, load_filelists=True)
  >>> len(sack)
  1685

The number of packages in the Sack will increase by the number of packages found
in the repository (two in this case, it is an experimental repo after all).

.. _case_for_loading_the_filelists-label:

Case for Loading the Filelists
==============================

What the ``load_filelists=True`` argument to :meth:`~hawkey.Sack.load_repo` above does is
instruct hawkey to process the ``<hash>filelists.xml.gz`` file we passed in and
which contains structured list of absolute paths to all files of all packages
within the repo. This information can be used for two purposes:

* Finding a package providing given file. For instance, you need the file
  ``/usr/share/man/man3/fprintf.3.gz`` which is not installed. Consulting
  filelists (directly or through hawkey) can reveal the file is in the
  ``man-pages`` package.

* Depsolving. Some packages require concrete files as their dependencies. To
  know if these are resolvable and how, the solver needs to know what package
  provides what files.

Some files provided by a package (e.g those in ``/usr/bin``) are always visible
even without loading the filelists. Well-behaved packages requiring only those
can be thus resolved directly. Unortunately, there are packages that don't
behave and it is hard to tell in advance when you'll deal with one.

The strategy for using ``load_filelists=True`` is thus:

* Use it if you know you'll do resolving (i.e. you'll use :class:`Goal`).

* Use it if you know you'll be trying to match files to their packages.

* Use it if you are not sure.

.. _building_and_reusing_the_repo_cache-label:

Building and Reusing the Repo Cache
===================================

Internally to hold the package information and perform canonical resolving
hawkey uses `Libsolv`_. One great benefit this library offers is providing
writing and reading of metadata cache files in libsolv's own binary format
(files with ``.solv`` extension, typically). At a cost of few hundreds of
milliseconds, using the solv files reduces repo load times from seconds to tens
of milliseconds. It is thus a good idea to write and use the solv files every
time you plan to use the same repo for more than one Sack (which is at least
every time your hawkey program is run). To do that use ``build_cache=True`` with
:meth:`~.Sack.load_repo` and :meth:`~.Sack.load_system_repo`::

  >>> sack = hawkey.Sack(make_cache_dir=True)
  >>> sack.load_system_repo(build_cache=True)

By default, Hawkey creates ``@System.cache`` under the
``/var/tmp/hawkey-<your_login>-<random_hash>`` directory. This is the hawkey
cache directory, which you can always delete later (deleting the cache files in
the process). The ``.solv`` files are picked up automatically the next time you
try to create a hawkey sack. Except for a much higher speed of the operation
this will be completely transparent to you:

  >>> s2 = hawkey.Sack()
  >>> s2.load_system_repo()

By the way, the cache directory (if not set otherwise) also contains a logfile
with some boring debugging information.

Queries
=======

Query is the means in hawkey of finding a package based on one or more criteria
(name, version, repository of origin). Its interface is loosely based on
`Django's QuerySets
<https://docs.djangoproject.com/en/1.4/topics/db/queries/>`_, the main concepts being:

* a fresh Query object matches all packages in the Sack and the selection is
  gradually narrowed down by calls to :meth:`Query.filter`

* applying a :meth:`Query.filter` does not start to evaluate the Query, i.e. the
  Query is lazy. Query is only evaluated when we explicitly tell it to or when
  we start to iterate it.

* use Python keyword arguments to :meth:`Query.filter` to specify the filtering
  criteria.

For instance, let's say I want to find all installed packages which name ends
with ``gtk``::

  >>> q = hawkey.Query(sack).filter(reponame=hawkey.SYSTEM_REPO_NAME, name__glob='*gtk')
  >>> for pkg in q:
  ...     print str(pkg)
  ... 
  NetworkManager-gtk-1:0.9.4.0-9.git20120521.fc17.x86_64
  authconfig-gtk-6.2.1-1.fc17.x86_64
  clutter-gtk-1.2.0-1.fc17.x86_64
  libchamplain-gtk-0.12.2-1.fc17.x86_64
  libreport-gtk-2.0.10-3.fc17.x86_64
  pinentry-gtk-0.8.1-6.fc17.x86_64
  python-slip-gtk-0.2.20-2.fc17.noarch
  transmission-gtk-2.50-2.fc17.x86_64
  usermode-gtk-1.109-1.fc17.x86_64
  webkitgtk-1.8.1-2.fc17.x86_64
  xdg-user-dirs-gtk-0.9-1.fc17.x86_64

Or I want to find the latest version of all ``python`` packages the Sack knows of::

  >>> q.clear()
  >>> q = q.filter(name='python', latest_per_arch=True)
  >>> for pkg in q:
  ...     print str(pkg)
  ... 
  python-2.7.3-6.fc17.x86_64

You can also test a :class:`Query` for its truth value. It will be true whenever
the query matched at least one package::

  >>> q = hawkey.Query(sack).filter(file='/boot/vmlinuz-3.3.4-5.fc17.x86_64')
  >>> if q:
  ...     print 'match'
  ... 
  match
  >>> q = hawkey.Query(sack).filter(file='/booty/vmlinuz-3.3.4-5.fc17.x86_64')
  >>> if q:
  ...     print 'match'
  ... 
  >>> if not q:
  ...     print 'no match'
  ... 
  no match

.. NOTE::

   If the Query hasn't been evaluated already then it is evaluated whenever it's
   length is taken (either via ``len(q)`` or ``q.count()``), when it is tested for
   truth and when it is explicitly evaluated with ``q.run()``.

Resolving things with Goals
===========================

Many :class:`~.Sack` sessions culminate in a bout of dependency resolving, that
is answering a question along the lines of "I have a package X in a repository
here, what other packages do I need to install/update to have X installed and
all its dependencies recursively satisfied?" Suppose we want to install `the RTS
game Spring <http://springrts.com/>`_. First let's locate the latest version of
the package in repositories::

  >>> q = hawkey.Query(sack).filter(name='spring', latest_per_arch=True)
  >>> pkg = hawkey.Query(sack).filter(name='spring', latest_per_arch=True)[0]
  >>> str(pkg)
  'spring-88.0-2.fc17.x86_64'
  >>> pkg.reponame
  'fedora'

Then build the :class:`Goal` object and tell it our goal is installing the
``pkg``. Then we fire off the libsolv's dependency resolver by running the
goal::

  >>> g = hawkey.Goal(sack)
  >>> g.install(pkg)
  >>> g.run()
  True

``True`` as a return value here indicates that libsolv could find a solution to
our goal. This is not always the case, there are plenty of situations when there
is no solution, the most common one being a package should be installed but one
of its dependencies is missing from the sack.

The three methods :meth:`Goal.list_installs`, :meth:`Goal.list_upgrades` and
:meth:`Goal.list_erasures` can show which packages should be
installed/upgraded/erased to satisfy the packaging goal we set out to achieve
(the mapping of :func:`str` over the results below ensures human readable
package names instead of numbers are presented)::

  >>> map(str, g.list_installs())
  ['spring-88.0-2.fc17.x86_64', 'spring-installer-20090316-10.fc17.x86_64', 'springlobby-0.139-3.fc17.x86_64', 'spring-maps-default-0.1-8.fc17.noarch', 'wxBase-2.8.12-4.fc17.x86_64', 'wxGTK-2.8.12-4.fc17.x86_64', 'rb_libtorrent-0.15.9-1.fc17.x86_64', 'GeoIP-1.4.8-2.1.fc17.x86_64']
  >>> map(str, g.list_upgrades())
  []
  >>> map(str, g.list_erasures())
  []

So what does it tell us? That given the state of the given system and the given
repository we used, 8 packages need to be installed,
``spring-88.0-2.fc17.x86_64`` itself included. No packages need to be upgraded
or erased.

Selector Installs
-----------------

For certain simple and commonly used queries we can do installs
directly. Instead of executing a query however we instantiate and pass the
:meth:`Goal.install` method a :class:`Selector`:

  >>> g = hawkey.Goal(sack)
  >>> sltr = hawkey.Selector(sack).set(name='emacs-nox')
  >>> g.install(select=sltr)
  >>> g.run()
  True
  >>> map(str, g.list_installs())
  ['spring-88.0-2.fc17.x86_64', 'spring-installer-20090316-10.fc17.x86_64', 'springlobby-0.139-3.fc17.x86_64', 'spring-maps-default-0.1-8.fc17.noarch', 'wxBase-2.8.12-4.fc17.x86_64', 'wxGTK-2.8.12-4.fc17.x86_64', 'rb_libtorrent-0.15.9-1.fc17.x86_64', 'GeoIP-1.4.8-2.1.fc17.x86_64']
  >>> len(g.list_upgrades())
  0
  >>> len(g.list_erasures())
  0

Notice we arrived at the same result as before, when a query was constructed and
iterated first. What :class:`Selector` does when passed to :meth:`Goal.install`
is tell hawkey to examine its settings and without evaluating it as a
:class:`Query` it instructs libsolv to find *the best matching package* for it
and add that for installation. It saves user some decisions like which version
should be installed or what architecture (this gets very relevant with multiarch
libraries).

So Selectors usually only install a single package. If you mean to install *all
packages* matching an arbitrarily complex query, just use the method describe
above::

  >>> map(goal.install, q)
