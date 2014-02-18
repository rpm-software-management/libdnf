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
