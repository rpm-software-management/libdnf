#include "PackageInstantiable.hpp"

PackageInstantiable::PackageInstantiable(DnfSack *sack,
                                 const HyRepo repo,
                                 const char *name,
                                 const char *version,
                                 const char *arch)
        : Package(sack, repo, name, version, arch)
{
    addProvides(std::make_shared<Dependency>(sack, "rpm = 1.0"));
}

const char *PackageInstantiable::getName() const
{
    return getSolvableName();
}

const char *PackageInstantiable::getVersion() const
{
    return getSolvableEvr();
}
