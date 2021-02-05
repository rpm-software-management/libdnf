.. libdnf documentation main file, created by
   sphinx-quickstart on Fri Nov 10 18:35:37 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

libdnf C++ documentation
==================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

Public API Reference
=====================

.. doxygenclass:: Swdb
   :members:

.. doxygenclass:: Transaction
   :members:

.. doxygenclass:: RPMItem
   :members:

.. doxygenclass:: Repo
   :members:

.. doxygenclass:: TransactionItem
   :members:

.. doxygenclass:: CompsGroupItem
   :members:

.. doxygenclass:: CompsGroupPackage
   :members:

.. doxygenclass:: CompsEnvironmentItem
   :members:

.. doxygenclass:: CompsEnvironmentGroup
   :members:

.. doxygenenum:: TransactionItemReason

.. doxygenenum:: TransactionItemAction

Internal API Reference
======================

.. doxygenclass:: Item
   :members:

.. doxygenclass:: Transformer
   :members:

.. doxygenclass:: SQLite3
   :members:
