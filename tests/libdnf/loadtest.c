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
    spec.maxage = 6 * 24 * 3600;
    hy_repo_load(repo, &spec, "/var/cache/dnf/fedora-6dbd63560daef6bf");
    hy_repo_free(repo);
    return 0;
}
