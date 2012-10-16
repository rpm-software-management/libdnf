****************
Design Rationale
****************

.. highlight:: c
.. _rationale_selectors:

Selectors are not Queries
=========================

Since both a Query and a Selector work to limit the set of all Sack's packages
to a subset, it can be suggested the two concepts should be the same and
e.g. Queries should be used for Goal specifications instead of Selectors::

  // create sack, goal, ...
  HyQuery q = hy_query_create(sack);
  hy_query_filter(q, HY_PKG_NAME, HY_EQ, "anaconda")
  hy_goal_install_query(q)

This arrangment was in fact used in hawkey prior to version 0.3.0, just because
Queries looked like a convenient structure to hold this kind of information. It
was unfortunately confusing for the programmers: notice how evaluating the Query
``q`` would generally produce several packages (``anaconda`` for different
architectures and then different versions) but somehow when the same Query is
passed into the goal methods it always results in up to one pacakge selected for
the operation. This is a principal discrepancy. Further, Query is universal and
allows one to limit the package set with all sorts of criteria, matched in
different ways (substrings, globbing, set operation) while Selectors only
support few. Finally, while a fresh Query with no filters applied corresponds to
all packages of the Sack, a fresh Selector with no limits set is of no meaning.

An alternative to introducing a completely different concept was adding a
separate constructor function for Query, one that would from the start designate
the Query to only accept settings compatible with its purpose of becoming the
selecting element in a Goal operation (in Python this would probably be
implemented as a subclass of Query). But that would break client's assumptions
about Query (`the unofficial C++ FAQ
<http://www.parashift.com/c++-faq/circle-ellipse.html>`_ takes up the topic).

*Implementation note*: Selectors reflect the kind of specifications that can be
directly translated into Libsolv jobs, without actually searching for a concrete
package to put there. In other words, Selectors are specifically designed not to
ever iterate over the package data like Queries do. While Hawkey mostly aims to
hide any twists and complexities of the underlying library, in this case the
combined reasons warrant a concession.
