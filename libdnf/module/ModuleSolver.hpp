#ifndef LIBDNF_MODULESOLVER_HPP
#define LIBDNF_MODULESOLVER_HPP

#include "ModulePackage.hpp"

#include <solv/pool.h>
#include <memory>


class ModuleSolver
{
public:
    explicit ModuleSolver(const std::shared_ptr<Pool> &pool);
    ~ModuleSolver() = default;

    void addModulePackage(const std::shared_ptr<ModulePackage> &modulePackage);
    std::vector<Id> solve();

private:
    std::vector<Id> solve(const Queue &queue) const;

    std::shared_ptr<Pool> pool;
    std::vector<std::shared_ptr<ModulePackage>> modules;
};


#endif //LIBDNF_MODULESOLVER_HPP
