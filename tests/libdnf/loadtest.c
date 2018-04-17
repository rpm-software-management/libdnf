#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>
#include "libdnf/hy-repo.h"

int main(void)
{
    HyRepo repo = hy_repo_create("test");
    HyRemote remote;

    hy_repo_set_string(repo, HY_REPO_CACHEDIR, "/var/tmp/loadtest");
    hy_repo_set_maxage(repo, 60);

    remote.url = "http://download.fedoraproject.org/pub/fedora/linux/releases/27/Everything/x86_64/os/";
    remote.gpgcheck = 0;
    remote.max_mirror_tries = 3;
    remote.max_parallel_downloads = 1;

    hy_repo_load(repo, &remote);
    printf("repomd_fn: %s\n", hy_repo_get_string(repo, HY_REPO_MD_FN));
    printf("primary_fn: %s\n", hy_repo_get_string(repo, HY_REPO_PRIMARY_FN));

    hy_repo_free(repo);
    return 0;
}
