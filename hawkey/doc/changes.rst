*************
 API Changes
*************

This document describes the API changes the library users should be aware of
before upgrading to each respective version. It is our plan to have the amount
of changes requiring changing the client code go to a minimum after the library
hits the 1.0.0 version.

Changes in 0.2.10
=================

Python bindings
---------------

:meth:`Query.filter` now returns a new instance of :class:`Query`, the same as
the original with the new filtering applied. This allows for greater flexibility
handling the :class:`Query` objects and resembles the way ``QuerySets`` behave in
Django.

In practice the following code will stop working as expected::

  q = hawkey.Query(self.sack)
  q.filter(name__eq="flying")
  # processing the query ...

It needs to be changed to::

  q = hawkey.Query(self.sack)
  q = q.filter(name__eq="flying")
  # processing the query ...

The original semantics is now available via the :meth:`Query.filterm` method, so
the following will also work::

  q = hawkey.Query(self.sack)
  q.filterm(name__eq="flying")
  # processing the query ...
