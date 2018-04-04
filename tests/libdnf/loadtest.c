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
    spec.url = "foo";
    hy_repo_load_cache(repo, &spec, "/var/cache/dnf/fedora-6dbd63560daef6bf");
    printf("%s\n", hy_repo_get_string(repo, HY_REPO_PRIMARY_FN));
    hy_repo_free(repo);
    return 0;
}
