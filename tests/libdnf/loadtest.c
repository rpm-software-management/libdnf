#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>
#include "libdnf/hy-repo.h"
#include "libdnf/hy-repo-private.hpp"

int main(void)
{
    HyRepo repo = hy_repo_create("test");
    HyRemote remote;

    remote.url = "http://download.fedoraproject.org/pub/fedora/linux/releases/27/Everything/x86_64/os/";
    remote.cachedir = "/var/tmp/loadtest";
    remote.maxage = 60;

    hy_repo_load(repo, &remote);
    printf("repomd_fn: %s\n", repo->repomd_fn);
    printf("primary_fn: %s\n", repo->primary_fn);
    hy_repo_free(repo);

    return 0;
}
