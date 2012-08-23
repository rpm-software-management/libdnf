

************************
 python-hawkey Tutorial
************************

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

Hawkey always knows the name of every repository. Repositories loaded from Yum
are named by the user, the system repostiroy is always called ``@System``.

Loading Yum Repositories
========================

Let's be honest here: all the fun in packaging comes from packages you haven't
installed yet. Information about them, their *metadata*, can be obtained from
different sources and typically they are downloaded from an HTTP mirror (another
possibilities are FTP server, NFS mount, DVD distribution media, etc.). Hawkey
does not provide any means to discover and obtain the metadata locally: it is up
to the client to provide valid readable paths to the Yum metadata XML
files. Structures used for passing the information to hawkey are the hawkey
:class:`Repos <Repo>`. Suppose we somehow obtained the metadata and placed it in
``/home/akozumpl/tmp/repodata``. We can then load the metadata into hawkey::

  >>> path = "/home/akozumpl/tmp/repodata/%s"
  >>> repo = hawkey.Repo()
  >>> repo.name = "experimental"
  >>> repo.repomd_fn = path % "repomd.xml"
  >>> repo.primary_fn = path % "f7753a2636cc89d70e8aaa1f3c08413ab78462ca9f48fd55daf6dedf9ab0d5db-primary.xml.gz"
  >>> repo.filelists_fn = path % "0261e25e8411f4f5e930a70fa249b8afd5e86bb9087d7739b55be64b76d8a7f6-filelists.xml.gz"
  >>> sack.load_yum_repo(repo, load_filelists=True)
  >>> len(sack)
  1685

The number of packages in the Sack will increase by the number of packages found
in the repository (two in this case, it is an experimental repo after all).

Case for Loading the Filelists
==============================

What the ``load_filelists=True`` argument to ``load_yum_repo()`` above does is
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
:func:`load_yum_repo` and :func:`load_system_repo`::

  >>> sack.load_system_repo(build_cache=True)

By default, Hawkey creates ``@System.cache`` under the
``/var/tmp/hawkey-<your_login>-<random_hash>`` directory. This is the hawkey
cache directory, which you can always delete later (deleting the cache files in
the process). The ``.solv`` files are picked up automatically the next time you
try to create a hawkey sack. Except for a much higher speed of the operation
this will be completely transparent to you:

  >>> s2 = hawkey.Sack()
  >>> s2.load_system_repo()

By the way, the cache directory also contains a logfile with some boring
debugging information.

Queries
=======

Query is the means in hawkey of finding a package based on one or more criteria
(name, version, repository of origin). Its interface is loosely based on
`Django's QuerySets
<https://docs.djangoproject.com/en/1.4/topics/db/queries/>`_, the main concepts being:

* a fresh Query object matches all packages in the Sack and the selection is
  gradually narrowed down by calls to :meth:`filter`

* applying a :meth:`filter` does not start to evaluate the Query, i.e. the Query
  is lazy. Query is only evaluated when we explicitly tell it to or when we
  start to iterate it.

* use Python keyword arguments to :meth:`filter` to specify the filtering
  criteria.

For instance, let's say I want to find all installed packages which name ends
with ``gtk``::

  >>> q = hawkey.Query(sack)
  >>> q.filter(repo=hawkey.SYSTEM_REPO_NAME, name__glob='*gtk')
  <hawkey.Query object at 0x7fa477e73320>
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
  >>> q.filter(name='python', latest=True)
  <hawkey.Query object at 0x7fa477e73460>
  >>> for pkg in q:
  ...     print str(pkg)
  ... 
  python-2.7.3-6.fc17.x86_64

Goals: "I want to install X, what are the deps I'm missing?"
============================================================

Query Installs
--------------

