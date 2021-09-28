Libsolv
-------

Single Pool instance
~~~~~~~~~~~~~~~~~~~~

We have decided to keep all packages incl. comps in a single Pool instance.
Lookups are slower due to iterating through a larger data set
(using smaller dedicated Pool instances per package type would be faster),
but we don't have to duplicate repo definitions, keep the repos in sync
across all Pool instances etc.

Note there is a second libsolv Pool in implementation of modularity,
which is used to resolve module dependencies. This is a separate
resolution namespace from the main libsolv Pool,
unrelated to and incompatible with rpm packages, and needs to be kept separate.
