#pragma once


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


// TODO: consolidate states with plugin callbacks
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
