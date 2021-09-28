Packages
--------


Working with multiple package types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible to load multiple package types (such as rpm, deb, arch, ...)
in a single libsolv Pool and make them available via PackageSack.
We recommend doing so only for querying purposes.
We MUST avoid running a transaction across multiple package types
because the respective databases (rpmdb, dpkg database, ...) are designed as
self-contained and error out on performing a transaction with broken dependencies
that would be tracked in the other database(s) in this case.

