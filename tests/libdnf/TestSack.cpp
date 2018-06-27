#include "TestSack.hpp"
#include "CppUnitMacros.hpp"
#include "TestMacros.hpp"
#include "libdnf/hy-repo.h"
#include "libdnf/hy-repo-private.hpp"
#include "libdnf/repo/RpmPackage.hpp"

#include <wordexp.h>
#include <cerrno>

extern "C" {
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/testcase.h>
}

bool TestSack::loadRepo(Pool *pool, const char *name, const char *path, bool installed)
{
    HyRepo hyRepo = hy_repo_create(name);
    Repo *repo = repo_create(pool, name);
    hyRepo->libsolv_repo = repo;
    repo->appdata = hyRepo;

    FILE *fp = fopen(path, "r");
    if (!fp)
        return false;
    testcase_add_testtags(repo, fp, 0);
    if (installed)
        pool_set_installed(pool, repo);

    fclose(fp);
    return true;
}

void TestSack::repoSetString(HyRepo repo, const char *path, _hy_repo_param_e type)
{
    wordexp_t word_vector;
    if (wordexp(path, &word_vector, 0) || word_vector.we_wordc < 1) {
        wordfree(&word_vector);
        hy_repo_free(repo);
        CPPUNIT_FAIL(std::string("Failed to set string: ") + path);
    }

    hy_repo_set_string(repo, type, word_vector.we_wordv[0]);
    wordfree(&word_vector);
}

HyRepo TestSack::globForRepoFiles(Pool *pool, const char *repo_name, const char *path)
{
    HyRepo repo = hy_repo_create(repo_name);

    repoSetString(repo, pool_tmpjoin(pool, path, "/repomd.xml", nullptr), HY_REPO_MD_FN);
    repoSetString(repo, pool_tmpjoin(pool, path, "/*primary.xml.gz", nullptr), HY_REPO_PRIMARY_FN);
    repoSetString(repo, pool_tmpjoin(pool, path, "/*filelists.xml.gz", nullptr), HY_REPO_FILELISTS_FN);
    repoSetString(repo, pool_tmpjoin(pool, path, "/*prestodelta.xml.gz", nullptr), HY_REPO_PRESTO_FN);
    repoSetString(repo, pool_tmpjoin(pool, path, "/*updateinfo.xml.gz", nullptr), HY_REPO_UPDATEINFO_FN);

    return repo;
}

TestSack::TestSack(const char *repoDirPath, const char *tmpDirPath)
        : repoDirPath{strdup(repoDirPath)}
        , tmpDirPath{strdup(tmpDirPath)}
{
    setUpSack();
}

TestSack::~TestSack()
{
    free(tmpDirPath);
    free(repoDirPath);
}

void TestSack::setUpSack()
{
    char *tmpPath = strdup(UNITTEST_DIR);
    auto dir = mkdtemp(tmpPath);

    if (dir == nullptr) {
        CPPUNIT_FAIL(std::string{"Failed to create tmpdir. Reason: "} + strerror(errno));
    }

    sack = std::shared_ptr<DnfSack>(dnf_sack_new(), &g_object_unref);
    dnf_sack_set_cachedir(sack.get(), tmpPath);
    dnf_sack_set_arch(sack.get(), TEST_FIXED_ARCH, nullptr);

    free(tmpPath);
    CPPUNIT_ASSERT(dnf_sack_setup(sack.get(), DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, nullptr));
}

void TestSack::loadRepos(...)
{
    Pool *pool = dnf_sack_get_pool(sack.get());
    va_list names;

    va_start(names, nullptr);
    const char *name = va_arg(names, const char *);
    while (name) {
        const char *path = pool_tmpjoin(pool, repoDirPath, name, ".repo");
        bool installed = !strncmp(name, HY_SYSTEM_REPO_NAME, strlen(HY_SYSTEM_REPO_NAME));

        loadRepo(pool, name, path, installed);
        name = va_arg(names, const char *);
    }
    va_end(names);
}

void TestSack::loadRepo(const char *name, const char *path, bool installed)
{
    if (!loadRepo(dnf_sack_get_pool(sack.get()), name, path, installed))
        CPPUNIT_FAIL(std::string{"Failed to load repo: "} + name);
}

HyRepo TestSack::globForRepoFiles(const char *repo_name, const char *path)
{
    return globForRepoFiles(dnf_sack_get_pool(sack.get()), repo_name, path);
}

void TestSack::setUpYumSack(const char *yum_repo_name)
{
    Pool *pool = dnf_sack_get_pool(sack.get());
    const char *repo_path = pool_tmpjoin(pool, repoDirPath, YUM_DIR_SUFFIX, nullptr);
    CPPUNIT_ASSERT_FAIL(access(repo_path, X_OK));
    HyRepo repo = globForRepoFiles(yum_repo_name, repo_path);

    CPPUNIT_ASSERT_FAIL(!dnf_sack_load_repo(sack.get(), repo,
                                DNF_SACK_LOAD_FLAG_BUILD_CACHE |
                                DNF_SACK_LOAD_FLAG_USE_FILELISTS |
                                DNF_SACK_LOAD_FLAG_USE_UPDATEINFO |
                                DNF_SACK_LOAD_FLAG_USE_PRESTO, nullptr));
    CPPUNIT_ASSERT(dnf_sack_count(sack.get()) == TEST_EXPECT_YUM_NSOLVABLES);
    hy_repo_free(repo);
}

void TestSack::addComandline()
{
    Pool *pool = dnf_sack_get_pool(sack.get());
    const char *path = pool_tmpjoin(pool, repoDirPath, "yum/tour-4-6.noarch.rpm", nullptr);
    auto package = dnf_sack_add_cmdline_package(sack.get(), path);
    delete package;
}
