#ifndef LIBDNF_TESTSACK_HPP
#define LIBDNF_TESTSACK_HPP

#include <memory>
#include "libdnf/dnf-sack.h"

class TestSack
{
public:
    static bool loadRepo(Pool *pool, const char *name, const char *path, bool installed);
    static HyRepo globForRepoFiles(Pool *pool, const char *repo_name, const char *path);

    TestSack(const char *repoDirPath, const char *tmpDirPath);
    ~TestSack();

    void loadRepos(...);
    void loadRepo(const char *name, const char *path, bool installed);
    HyRepo globForRepoFiles(const char *repo_name, const char *path);

    void setUpYumSack(const char *yum_repo_name);
    void addComandline();

    std::shared_ptr<DnfSack> getSack() const { return sack; };
    Pool *getPool() const { return dnf_sack_get_pool(sack.get()); }
    const char *getRepoDirPath() const { return repoDirPath; }
    const char *getTmpDirPath() const { return tmpDirPath; }

private:
    static void repoSetString(HyRepo repo, const char *path, _hy_repo_param_e type);

    void setUpSack();

    std::shared_ptr<DnfSack> sack;
    char *repoDirPath;
    char *tmpDirPath;
};


#endif //LIBDNF_TESTSACK_HPP
