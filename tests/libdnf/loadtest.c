#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <librepo/librepo.h>
#include "libdnf/hy-repo.h"
#include "libdnf/hy-repo-private.hpp"

int main(void)
{
    HyRepo repo = hy_repo_create("test");
    HySpec spec;

    spec.url = "http://download.fedoraproject.org/pub/fedora/linux/releases/27/Everything/x86_64/os/";
    spec.cachedir = "/var/tmp/loadtest";
    spec.maxage = 60;

    hy_repo_load(repo, &spec);
    printf("repomd_fn: %s\n", repo->repomd_fn);
    printf("primary_fn: %s\n", repo->primary_fn);
    hy_repo_free(repo);

    return 0;
}
