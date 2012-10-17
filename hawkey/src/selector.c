// libsolv
#include <solv/util.h>

// hawkey
#include "errno.h"
#include "query_internal.h"
#include "selector_internal.h"

static void
replace_filter(struct _Filter **fp, int keyname, int cmp_type, const char *match)
{
    if (*fp == NULL)
	*fp = filter_create(1);
    else
	filter_reinit(*fp, 1);

    struct _Filter *f = *fp;

    f->keyname = keyname;
    f->filter_type = cmp_type;
    f->matches[0].str = solv_strdup(match);
}

static int
valid_setting(int keyname, int cmp_type)
{
    switch (keyname) {
    case HY_PKG_ARCH:
    case HY_PKG_NAME:
	return (cmp_type == HY_EQ || cmp_type == HY_GLOB);
    default:
	return 0;
    }
}

HySelector
hy_selector_create(HySack sack)
{
    HySelector sltr = solv_calloc(1, sizeof(*sltr));
    sltr->sack = sack;
    return sltr;
}

void
hy_selector_free(HySelector sltr)
{
    filter_free(sltr->f_arch);
    filter_free(sltr->f_name);
    solv_free(sltr);
}

int
hy_selector_set(HySelector sltr, int keyname, int cmp_type, const char *match)
{
    if (!valid_setting(keyname, cmp_type))
	return HY_E_SELECTOR;

    switch (keyname) {
    case HY_PKG_ARCH:
	replace_filter(&sltr->f_arch, keyname, cmp_type, match);
	break;
    case HY_PKG_NAME:
	replace_filter(&sltr->f_name, keyname, cmp_type, match);
	break;
    default:
	return HY_E_SELECTOR;
    }
    return 0;
}
