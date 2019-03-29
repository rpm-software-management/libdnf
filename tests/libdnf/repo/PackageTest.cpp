#include "PackageTest.hpp"

#include <libdnf/repo/Repo-private.hpp>
#include <solv/poolarch.h>
#include <solv/solver.h>
#include <solv/selection.h>

CPPUNIT_TEST_SUITE_REGISTRATION(PackageTest);

void PackageTest::setUp()
{
    g_autoptr(GError) error = nullptr;
    sack = dnf_sack_new();
    repo = libdnf::hy_repo_create("repo");
    libdnf::repoGetImpl(repo)->attachLibsolvRepo(repo_create(dnf_sack_get_pool(sack), "repo"));
    dnf_sack_load_repo(sack, repo, 0, &error);
    package = std::unique_ptr<PackageInstantiable>(new PackageInstantiable(sack, repo, "rpm", "1.0", "x86_64"));
}

void PackageTest::tearDown()
{
    hy_repo_free(repo);
    g_object_unref(sack);
}

void PackageTest::testName()
{
    CPPUNIT_ASSERT(strcmp("rpm", package->getName()) == 0);
}

void PackageTest::testVersion()
{
    CPPUNIT_ASSERT(strcmp("1.0", package->getVersion()) == 0);
}

void PackageTest::testArch()
{
    CPPUNIT_ASSERT(strcmp("x86_64", package->getArch()) == 0);
}

void PackageTest::testIsInRepo()
{
    Pool *pool = dnf_sack_get_pool(sack);
    Repo *repo = libdnf::repoGetImpl(this->repo)->libsolvRepo;

    Solvable *solvable = pool_id2solvable(pool, package->getId());

    std::string name = pool_id2str(pool, solvable->name);
    std::string version = pool_id2str(pool, solvable->evr);
    std::string arch = pool_id2str(pool, solvable->arch);

    CPPUNIT_ASSERT("rpm" == name);
    CPPUNIT_ASSERT("1.0" == version);
    CPPUNIT_ASSERT("x86_64" == arch);

    Id *provides = repo->idarraydata + solvable->provides;
    std::string provideName = pool_id2str(pool, *provides);
    std::string provideRelation = pool_id2rel(pool, *provides);
    std::string provideEvr = pool_id2evr(pool, *provides);
    CPPUNIT_ASSERT("rpm" == provideName);
    CPPUNIT_ASSERT(" = " == provideRelation);
    CPPUNIT_ASSERT("1.0" == provideEvr);
}
