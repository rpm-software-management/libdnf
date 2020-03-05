/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef LIBDNF_BASE_STATE_HPP
#define LIBDNF_BASE_STATE_HPP

/*
// * @DNF_STATE_ACTION_UNKNOWN:                   Unknown status
// * @DNF_STATE_ACTION_DOWNLOAD_PACKAGES:         Downloading packages
// * @DNF_STATE_ACTION_DOWNLOAD_METADATA:         Downloading metadata
// * @DNF_STATE_ACTION_LOADING_CACHE:             Loading cache
// * @DNF_STATE_ACTION_TEST_COMMIT:               Testing transaction
// * @DNF_STATE_ACTION_REQUEST:                   Requesting data
// * @DNF_STATE_ACTION_REMOVE:                    Removing packages
// * @DNF_STATE_ACTION_INSTALL:                   Installing packages
// * @DNF_STATE_ACTION_UPDATE:                    Updating packages
// * @DNF_STATE_ACTION_CLEANUP:                   Cleaning packages
// * @DNF_STATE_ACTION_OBSOLETE:                  Obsoleting packages
// * @DNF_STATE_ACTION_REINSTALL:                 Reinstall packages
// * @DNF_STATE_ACTION_DOWNGRADE:                 Downgrading packages
 * @DNF_STATE_ACTION_QUERY:                     Querying for results
*/

namespace libdnf {


// TODO(dmach): consolidate states with plugin callbacks
enum class State {
    INIT,
    LOAD_CONFIG,
    DOWNLOAD_METADATA,
    LOAD_SACK,
    SACK_READY,
    BUILD_TRANSACTION,  // add stuff to goal
    RESOLVE_TRANSACTION,
    DOWNLOAD_PACKAGES,
    TEST_TRANSACTION,
    TRANSACTION,
};


}  // namespace libdnf

#endif
