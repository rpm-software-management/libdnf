System state
------------


System state vs history database
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

System state and history database hold similar data, but they are not interchangeable.
That's why we must use them for the right purposes.

* System state

  * Holds information about the currently installed software (extends rpmdb and any other state information).
  * Is used to retrieve information about installed software such as
    ``reason``, ``from_repo``, list of installed comps groups and their packages etc.
  * Consistency and reliability of the stored data is crucial for managing a working system.

* History database

  * Holds recorded transactions.
  * Is used in ``history`` operations to rollback/undo/redo transactions.
  * May be used to recover system state if it is lost.
    System state may not be fully recovered in such case because of missing records in the history database.
  * Important for auditing the transaction history.
  * It is not crucial for managing the installed system and may be freely deleted (if auditing is not an issue).


Handling installations made outside libdnf
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sometimes a user installs packages by directly using ``rpm`` command
or an application that is not built on top of libdnf.
By doing that, libdnf's system state data gets out of sync with installed packages.

Consider installing an application and its dependencies using the ``rpm`` command.
In DNF 4, all such packages would have ``unknown`` reason, which is equal to ``user``.
This effectively disables removing the packages as unused dependencies
and we need to explicitly provide all their names to the ``remove`` command.
While this is quite easy for a power user, a normal user doesn't know if it's safe to
remove the packages (especially low-level libraries)
and ends up with a ever-growing package set they frequently solve by reinstalling the system from scratch.

That's why we decided to create a differential transaction and update the system state and history database
any time unexpected installed packages are detected. We detect leaf packages and mark them with reason ``user``,
while keeping the remaining packages with reason ``dependency``.
Removing the leaf package also removes its dependencies as expected.

Unprivileged users can't change the system state and history database.
We have decided to compute and keep the differential transaction in memory for them.
When a privileged user runs a transaction, the differential transaction is stored on disk.
