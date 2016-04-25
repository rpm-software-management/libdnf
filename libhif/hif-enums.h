#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * HifComparisonKind:
 *
 * The comparison type.
 */
typedef enum
{
    HIF_COMPARISON_ICASE     = 1 << 0,
    HIF_COMPARISON_NOT       = 1 << 1,
    HIF_COMPARISON_FLAG_MASK = HIF_COMPARISON_ICASE | HIF_COMPARISON_NOT, /*< private >*/

    HIF_COMPARISON_EQ        = 1 << 8,
    HIF_COMPARISON_LT        = 1 << 9,
    HIF_COMPARISON_GT        = 1 << 10,

    HIF_COMPARISON_SUBSTR    = 1 << 11,
    HIF_COMPARISON_GLOB      = 1 << 12,

    HIF_COMPARISON_NEQ       = HIF_COMPARISON_EQ | HIF_COMPARISON_NOT,

    HIF_COMPARISON_NAME_ONLY = 1 << 16,
} HifComparisonKind;

G_END_DECLS
