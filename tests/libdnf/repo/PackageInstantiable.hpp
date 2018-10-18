#ifndef LIBDNF_PACKAGETESTABLE_HPP
#define LIBDNF_PACKAGETESTABLE_HPP

#include "libdnf/repo/solvable/Package.hpp"

class PackageInstantiable : public Package
{
public:
    PackageInstantiable(DnfSack *sack, HyRepo repo, const char *name, const char *version, const char *arch);
};


#endif //LIBDNF_PACKAGETESTABLE_HPP
