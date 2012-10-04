******************************
python-hawkey Reference Manual
******************************

.. contents::


Error handling
==============

When an error or an unexpected event occurs during a hawkey routine, an
exception is raised:

* if it is a general error that could be common to other Python programs, one of
  the standard Python built-in exceptions is raised. For instance, ``IOError``
  and ``TypeError`` can be raised from Hawkey.

* if the error is due to a Hawkey-specific value/parameter that was passed,
  ``hawkey.ValueException`` or one of its subclasses, ``QueryException`` (poorly
  formed Query) or ``ArchException`` (unrecognized architecture), is used.

* sometimes there is a close call between blaming the error on an input
  parameter or on something else, beyond the programmer's
  control. ``hawkey.RuntimeException`` is generally used in this case.

* ``hawkey.ValidationException`` is raised when a function call performs a
  preliminary check before proceeding with the main operation and this check
  fails.

The class hierarchy for Hawkey exceptions is::

  +-- hawkey.Exception
       +-- hawkey.ValueException
       |    +-- hawkey.QueryException
       |    +-- hawkey.ArchException
       +-- hawkey.RuntimeException
       +-- hawkey.ValidationException
