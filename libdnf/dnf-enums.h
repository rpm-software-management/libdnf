#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * DnfComparisonKind:
 *
 * The comparison type.
 */
typedef enum
{
    DNF_COMPARISON_ICASE     = 1 << 0,
    DNF_COMPARISON_NOT       = 1 << 1,
    DNF_COMPARISON_FLAG_MASK = DNF_COMPARISON_ICASE | DNF_COMPARISON_NOT, /*< private >*/

    DNF_COMPARISON_EQ        = 1 << 8,
    DNF_COMPARISON_LT        = 1 << 9,
    DNF_COMPARISON_GT        = 1 << 10,

    DNF_COMPARISON_SUBSTR    = 1 << 11,
    DNF_COMPARISON_GLOB      = 1 << 12,

    DNF_COMPARISON_NEQ       = DNF_COMPARISON_EQ | DNF_COMPARISON_NOT,

    DNF_COMPARISON_NAME_ONLY = 1 << 16,
} DnfComparisonKind;

G_END_DECLS
