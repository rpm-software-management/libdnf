#include "ModuleSolver.hpp"

#include <vector>
#include <solv/solver.h>
#include <iostream>
#include <sstream>

ModuleSolver::ModuleSolver(const std::shared_ptr<Pool> &pool)
        : pool(pool)
{
    pool_createwhatprovides(pool.get());
}

void ModuleSolver::addModulePackage(const std::shared_ptr<ModulePackage> &modulePackage)
{
    modules.push_back(modulePackage);
}

std::vector<Id> ModuleSolver::solve()
{
    std::vector<Id> solvedIds;
    pool_set_flag(pool.get(), POOL_FLAG_IMPLICITOBSOLETEUSESCOLORS, 1);

    for (const auto &module : modules) {
        Queue queue;
        queue_init (&queue);

        std::ostringstream ss;
        ss << "module(" << module->getName() << ":" << module->getStream() << ")";
        Id dep = pool_str2id(pool.get(), ss.str().c_str(), 1);
        pool_whatcontainsdep(pool.get(), SOLVABLE_REQUIRES, dep, &queue, 0);
        auto solved = solve(queue);
        solvedIds.insert(std::end(solvedIds), std::begin(solved), std::end(solved));

        queue_empty(&queue);
        ss << "module(" << module->getName() << ")";
        dep = pool_str2id(pool.get(), ss.str().c_str(), 1);
        pool_whatcontainsdep(pool.get(), SOLVABLE_REQUIRES, dep, &queue, 0);
        solved = solve(queue);
        solvedIds.insert(std::end(solvedIds), std::begin(solved), std::end(solved));

        queue_free(&queue);
    }


    return solvedIds;
}

std::vector<Id> ModuleSolver::solve(const Queue &queue) const
{
    std::vector<Id> solvedIds;

    for (int j{0}; j < queue.count; j++) {
        Id id = queue.elements[j];
        Queue job;
        queue_init(&job);
        queue_push2(&job, SOLVER_SOLVABLE | SOLVER_INSTALL, id);

        auto solver = solver_create(pool.get());
        solver_solve(solver, &job);
        auto transaction = solver_create_transaction(solver);

        Queue installed;
        queue_init (&installed);
        transaction_installedresult (transaction, &installed);
        for (int x = 0; x < installed.count; x++)
        {
            Id p = installed.elements[x];
            solvedIds.push_back(p);
        }

        queue_free(&job);
        queue_free(&installed);
        solver_free(solver);
    }

    return solvedIds;
}

