..
  Copyright (C) 2014  Red Hat, Inc.

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

****
FAQ
****

.. contents::

Getting Started
===============

How do I build it?
------------------

See the `README <https://github.com/akozumpl/hawkey/tree/master/README.rst>`_.

Are there examples using hawkey?
--------------------------------

Yes, look at:

* `unit tests <https://github.com/akozumpl/hawkey/tree/master/tests>`_
* `The Hawkey Testing Hack <https://github.com/akozumpl/hawkey/blob/master/src/hth.c>`_
* a more complex example is `DNF <https://github.com/akozumpl/dnf/>`_, the Yum fork using hawkey for backend.

Using Hawkey
============

How do I obtain the repo metadata files to feed to Hawkey?
----------------------------------------------------------

It is entirely up to you. Hawkey does not provide any means to do this
automatically, for instance from your `/etc/yum.repos.d` configuration. Use or
build tools to do that. For instance, both Yum and DNF deals with the same
problem and inside they employ `urlgrabber <http://urlgrabber.baseurl.org/>`_ to
fetch the files. A general solution if you work in C is for instance `libcurl
<http://libcurl.org/>`_.  If you are building a nice downloading library that
integrates well with hawkey, let us know.

Why is a tool to do the downloads not integrated into Hawkey?
-------------------------------------------------------------

Because downloading things from remote servers is a differnt domain full of its
own complexities like HTTPS, parallel downloads, error handling and error
recovery to name a few. Downloading is a concern that can be naturally separated
from other parts of package metadata managing.
