// libsolv
#include <solv/util.h>

// hawkey
#include "testsys.h"

void
dump_packagelist(PackageList plist)
{
    for (int i = 0; i < packagelist_count(plist); ++i) {
	Package pkg = packagelist_get(plist, i);
	char *nvra = package_get_nvra(pkg);
	printf("\t%s\n", nvra);
	solv_free(nvra);
    }
}
