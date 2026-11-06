/*
 * Copyright (C) 2026 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef LIBDNF_MODULEMD_COMPAT_HPP
#define LIBDNF_MODULEMD_COMPAT_HPP

#ifdef WITH_MODULEMD

#include <modulemd-2.0/modulemd.h>

#else

/*
 * libdnf is built without libmodulemd (-DWITH_MODULEMD=OFF). ModuleMetadata
 * then never parses module metadata, so no libmodulemd object ever exists and
 * none of the functions below can be reached. They only let the module code
 * compile unchanged, and abort if they are reached anyway.
 */

#include <glib-object.h>

typedef struct _ModulemdDependencies ModulemdDependencies;
typedef struct _ModulemdModuleIndex ModulemdModuleIndex;
typedef struct _ModulemdModuleIndexMerger ModulemdModuleIndexMerger;
typedef struct _ModulemdModuleStream ModulemdModuleStream;
typedef struct _ModulemdModuleStreamV2 ModulemdModuleStreamV2;
typedef struct _ModulemdObsoletes ModulemdObsoletes;
typedef struct _ModulemdProfile ModulemdProfile;

[[noreturn]] static inline void
modulemd_compat_unavailable(const char * func)
{
    g_error("%s: libdnf was built without libmodulemd", func);
}

#define MODULEMD_COMPAT_STUB(ret, name, ...) \
    [[noreturn]] static inline ret name(__VA_ARGS__) { modulemd_compat_unavailable(G_STRFUNC); }

MODULEMD_COMPAT_STUB(gchar **, modulemd_dependencies_get_runtime_modules_as_strv, ModulemdDependencies *)
MODULEMD_COMPAT_STUB(gchar **, modulemd_dependencies_get_runtime_streams_as_strv, ModulemdDependencies *,
                     const gchar *)
MODULEMD_COMPAT_STUB(ModulemdModuleIndex *, modulemd_module_index_new, void)
MODULEMD_COMPAT_STUB(gboolean, modulemd_module_index_add_module_stream, ModulemdModuleIndex *,
                     ModulemdModuleStream *, GError **)
MODULEMD_COMPAT_STUB(gchar *, modulemd_module_index_dump_to_string, ModulemdModuleIndex *, GError **)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_get_module_name, ModulemdModuleStream *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_get_stream_name, ModulemdModuleStream *)
MODULEMD_COMPAT_STUB(guint64, modulemd_module_stream_get_version, ModulemdModuleStream *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_get_context, ModulemdModuleStream *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_get_arch, ModulemdModuleStream *)
MODULEMD_COMPAT_STUB(GPtrArray *, modulemd_module_stream_v2_get_dependencies, ModulemdModuleStreamV2 *)
MODULEMD_COMPAT_STUB(gboolean, modulemd_module_stream_v2_is_static_context, ModulemdModuleStreamV2 *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_v2_get_summary, ModulemdModuleStreamV2 *,
                     const gchar *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_module_stream_v2_get_description, ModulemdModuleStreamV2 *,
                     const gchar *)
MODULEMD_COMPAT_STUB(gchar **, modulemd_module_stream_v2_get_rpm_artifacts_as_strv, ModulemdModuleStreamV2 *)
MODULEMD_COMPAT_STUB(gchar **, modulemd_module_stream_v2_get_demodularized_rpms, ModulemdModuleStreamV2 *)
MODULEMD_COMPAT_STUB(gchar **, modulemd_module_stream_v2_get_profile_names_as_strv, ModulemdModuleStreamV2 *)
MODULEMD_COMPAT_STUB(ModulemdProfile *, modulemd_module_stream_v2_get_profile, ModulemdModuleStreamV2 *,
                     const gchar *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_obsoletes_get_obsoleted_by_module_name, ModulemdObsoletes *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_obsoletes_get_obsoleted_by_module_stream, ModulemdObsoletes *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_profile_get_name, ModulemdProfile *)
MODULEMD_COMPAT_STUB(const gchar *, modulemd_profile_get_description, ModulemdProfile *, const gchar *)
MODULEMD_COMPAT_STUB(gchar **, modulemd_profile_get_rpms_as_strv, ModulemdProfile *)
MODULEMD_COMPAT_STUB(gboolean, modulemd_profile_is_default, ModulemdProfile *)

#undef MODULEMD_COMPAT_STUB

#endif // WITH_MODULEMD

#endif // LIBDNF_MODULEMD_COMPAT_HPP
