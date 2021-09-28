Reasons
-------


Resolving reasons
~~~~~~~~~~~~~~~~~

By ``package`` in this section, we mean package ``name`` or ``name.arch``.
Version fields such as ``epoch``, ``version`` and ``release``
are not used when working with reasons because reasons are inherited through package upgrades.

System state tracks lists of userinstalled::
  * packages
  * comps groups
  * comps environments
  * module streams (explicitly enabled by user)
  * module profiles
  * ... and any other software that will be supported by dnf

In addition to that, system state tracks comps group packages and environment groups (see "Tracking installed comps" section).

Package reason is resolved the following way:
  * If a package is userinstalled, return reason ``user``.
  * If a package is tracked as a part of an installed group, return reason ``group``.

    * When a group changes (is upgraded for example) and a package is no longer part of it, it becomes a ``dependency``.

  * Otherwise return reason ``dependency``.
