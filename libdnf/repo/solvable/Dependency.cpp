#include "Dependency.hpp"
#include "libdnf/utils/utils.hpp"

/* workaround, libsolv lacks 'extern "C"' in its header file */
extern "C" {
#   include <solv/pool_parserpmrichdep.h>
}


#define NUMBER_OF_ITEMS_IN_RELATIONAL_DEPENDENCY 3

Dependency::Dependency(DnfSack *sack, Id id)
        : sack(sack)
        , id(id)
{}

Dependency::Dependency(DnfSack *sack, const char *name, const char *version, int solvComparisonOperator)
        : sack(sack)
{
    Pool *pool = dnf_sack_get_pool(sack);
    id = pool_str2id(pool, name, 1);

    if (version) {
        Id evrId = pool_str2id(pool, version, 1);
        id = pool_rel2id(pool, id, evrId, solvComparisonOperator, 1);
    }
}

Dependency::Dependency(DnfSack *sack, const std::string &dependency)
        : sack(sack)
{
    parseAndCreateDependency(dependency);
}

Dependency::Dependency(const Dependency &dependency)
        : sack(dependency.sack)
        , id(dependency.id)
{}

Dependency::~Dependency() = default;
const char *Dependency::getName() const { return pool_id2str(dnf_sack_get_pool(sack), id); }
const char *Dependency::getRelation() const { return pool_id2rel(dnf_sack_get_pool(sack), id); }
const char *Dependency::getVersion() const { return pool_id2evr(dnf_sack_get_pool(sack), id); }

const char *Dependency::toString() const { return pool_dep2str(dnf_sack_get_pool(sack), id); }

void Dependency::parseAndCreateDependency(const std::string &dependency)
{
    std::vector<std::string> results = string::split(dependency, " ");

    Pool *pool = dnf_sack_get_pool(sack);
    if (isRichDependency(dependency)) {
        id = pool_parserpmrichdep(pool, dependency.c_str());
    } else if (results.size() == NUMBER_OF_ITEMS_IN_RELATIONAL_DEPENDENCY) { // hasRelationalOperator
        createRelationalDependency(results);
    } else {
        id = pool_str2id(pool, results[0].c_str(), 1 /* create */);
    }
}

void Dependency::createRelationalDependency(const std::vector<std::string> &results)
{
    Pool *pool = dnf_sack_get_pool(sack);
    id = pool_str2id(pool, results[0].c_str(), 1);

    if (!results[2].empty()) {
        Id evrId = pool_str2id(pool, results[2].c_str(), 1);
        id = pool_rel2id(pool, id, evrId, determineComparisonType(results[1]), 1);
    }
}

bool Dependency::isRichDependency(const std::string &dependency) const
{
    return dependency.find('(', 0) != std::string::npos;
}

int Dependency::determineComparisonType(const std::string &type)
{
    if (type == "<=") {
        return REL_LT | REL_EQ;
    } else if (type == ">=") {
        return REL_GT | REL_EQ;
    } else if (type == "==") {
        return REL_EQ;
    } else if (type == ">") {
        return REL_GT;
    } else if (type == "<") {
        return REL_LT;
    }

    throw "Unknown comparison: " + type;
}
