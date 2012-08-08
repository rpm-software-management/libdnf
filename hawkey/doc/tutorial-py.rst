.. default-domain:: py

************************
 python-hawkey Tutorial
************************

Setup
=====

First of, make sure hawkey is installed on your system, this should work from your terminal::

  >>> import hawkey

The Sack object
===============

*Sack* is an abstraction for a collection of packages. Sacks in hawkey are
powerful toplevel objects hiding lots of functionality. You'll want to create
one::

   >>> sack = hawkey.Sack()
   >>> len(sack)
   2

See that initially the sack contains no packages.


Loading RPMDB
=============

hawkey is a lib for listing, querying and resolving dependencies of *packages*
from *repositories*. On most linux distributions you always have at least *the
system repo* (in Fedora it is most often called "The RPM database"). To load it::

  >>> sack.load_system_repo()

Loading Yum Repositories
========================

Let's be honest here: all the fun in packaging comes from packages you haven't
installed yet. Information about them, their *metadata*, can be obtained from
different sources and typically however they are downloaded from a http mirror
(another possibilities are FTP server, NFS mount, DVD distribution media,
etc.). Hawkey does not provide any means to discover and obtain the metadata
locally: it is up to the client to provide valid paths to the metadata.


Building and reusing repo cache
===============================


