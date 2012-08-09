

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
  1683


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

* Use it if you know you'll do resolving (use :class:`Goal`).

* Use it if you know you'll be trying to match files to their packages.

* Use it if you are not sure.


Building and Reusing the Repo Cache
===================================

`Libsolv`_ 

Queries
=======

Goals: "I want to install X, what are the deps I'm missing?"
============================================================

Query Installs
--------------

