Comps
-----

Comps standard only defines a XML format describing groups and environments
that are available in the repodata.
It doesn't define how the installations or upgrades should look like,
neither how to track the installed comps on the system.
This section defines comps behavior for the DNF ecosystem.


Package types
~~~~~~~~~~~~~

Package types in comps groups say nothing about if a package must be part of a transaction,
they only determine package selection in user interface such as old version of Anaconda Installer:

  * ``mandatory`` - packages are always installed with the group and cannot be unselected
  * ``default`` - packages are selected by default and can be unselected
  * ``optional`` - packages are unselected by default and can be selected
  * ``conditional`` - packages get installed if the packages in the condition get installed or are present on the system already

In all cases comps have weak behavior - if a package is not available, it's skipped in the transaction.
On the other hand, if a package is available but has broken dependencies, the transaction fails.


Merging comps
~~~~~~~~~~~~~

How it works:
  * Comps can be part of multiple repositories.
  * Comps do not have a version that would define which one takes the precedence.
  * We merge them by repo priority and repo order in Pool:
    * Include new groups or environments.
    * Override values in existing groups (names, descriptions, translations).
    * Merge package lists in the groups.
    * Merge group lists in the environments.

What's not supported:
  * Remove anything during the merge, only additions and overrides are supported.
  * Change group or environment ``id``.


Transaction actions - groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  * ``install`` -- Install a group and all its packages that are available. If the group is installed already, do nothing.
  * ``remove`` -- Remove a group and all its packages that are not userinstalled or part of other groups.
  * ``upgrade`` -- Peform ``group remove`` on the installed group and ``group install`` using fresh group definitions.
    If the group is not installed, do nothing.

Good to know:
  * TODO: All these actions manipulate with groups. They do not serve for upgrading all packages in a group for example.
    In such case we need to run an upgrade on packages and use a filter option to narrow the package selection down: ``dnf upgrade --f-group=ID``.
  * Groups are not upgraded automatically during ``dnf upgrade``. They are upgraded automatically during ``dnf system-upgrade``.


Transaction actions - environments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  * ``install`` -- Install an environment and all its groups that are available. If the environment is installed already, do nothing.
  * ``remove`` -- Remove an environment and all its groups that are not userinstalled or part of other environments.
  * ``upgrade`` -- Peform ``environment remove`` on the installed environment and ``environment install`` using fresh environment definitions.
    If the environment is not installed, do nothing.

Good to know:
  * All group actions imply package actions as described in the ``Transaction actions - groups`` section.
  * Environments are not upgraded automatically during ``dnf upgrade``. They are upgraded automatically during ``dnf system-upgrade``.


Tracking installed comps
~~~~~~~~~~~~~~~~~~~~~~~~
Comps don't define any database that would hold installed groups and environments.
We have decided to store their merged forms in a single XML file (``/var/lib/dnf/installed/comps.xml.zst``),
because libsolv can easily load the file into the ``@System`` repository
and make the content available to the user in a similar fashion as installed packages from rpmdb.

We remove the group packages and environment groups that were not available at the install time
from the stored comps. This applies to packages that were not available because their repositories were disabled
and the packages that were excluded during the transaction.
Group and environment reasons are stored outside the comps.xml similarly to package reasons.


.. note::
    **TODO:**
        * How to handle removal of an installed group package? ``dnf install @core; dnf remove firewalld``.
        * How to handle overriding a reason of a group package?
        * Should we keep the group intact and pull the packages back on ``dnf group reinstall``?
        * Should we remove the removed package from the related installed groups?


