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


#ifndef LIBDNF_RPM_TRANSACTION_SET_CALLBACK_BASE_HPP
#define LIBDNF_RPM_TRANSACTION_SET_CALLBACK_BASE_HPP

// TODO(dmach): how to handle compatibility with existing libraries/programs when a new handle appears;
// TODO(dmach): will this be the same interface as dnf:dnf/yum/rpmtrans.py:class:TransactionDisplay ?


namespace libdnf::rpm {


class TransactionSetCallbackBase {
    void handle_rpmcallback_unknown();

    // obsolete:
    // RPMCALLBACK_REPACKAGE_START
    // RPMCALLBACK_REPACKAGE_PROGRESS
    // RPMCALLBACK_REPACKAGE_STOP

    void handle_rpmcallback_inst_open_file();
    void handle_rpmcallback_inst_close_file();

    void handle_rpmcallback_unpack_error();
    void handle_rpmcallback_cpio_error();

    void handle_rpmcallback_inst_start();
    void handle_rpmcallback_inst_progress();
    void handle_rpmcallback_inst_stop();

    void handle_rpmcallback_uninst_start();
    void handle_rpmcallback_uninst_progress();
    void handle_rpmcallback_uninst_stop();

    void handle_rpmcallback_trans_start();
    void handle_rpmcallback_trans_progress();
    void handle_rpmcallback_trans_stop();

    void handle_rpmcallback_script_start();
    void handle_rpmcallback_script_stop();
    void handle_rpmcallback_script_error();

    void handle_rpmcallback_verify_start();
    void handle_rpmcallback_verify_progress();
    void handle_rpmcallback_verify_stop();

protected:
    void callback();
};

/*
callback():

// libdnf
dnf_transaction_ts_progress_cb(const void *arg,
                               const rpmCallbackType what,
                               const rpm_loff_t amount,
                               const rpm_loff_t total,
                               fnpyKey key,
                               void *data)
// rpm
rpmtsCallback(const void * hd, const rpmCallbackType what,
              const rpm_loff_t amount, const rpm_loff_t total,
              const void * pkgKey, rpmCallbackData data)
*/


/*
ELEM_PROGRESS:


commit 448db68ceb5be3c7171b7ec0ea908d905792dc2f
Author: Michal Domonkos <mdomonko@redhat.com>
Date:   Mon Dec 7 17:13:26 2015 +0100

    Add RPMCALLBACK_ELEM_PROGRESS callback type
    
    Currently, there's no callback type that would be issued per each
    transaction element.  RPMCALLBACK_TRANS_PROGRESS is only issued during
    the prepare phase but not when packages are actually installed or
    erased.  Likewise, RPMCALLBACK_INST_ST* and RPMCALLBACK_UNINST_ST* won't
    be issued if an install or erase operation is skipped for some reason (a
    script or package upgrade failure).
    
    Having such a callback would allow the Python API consumers to always
    know upfront which element is about to be processed, before any other
    callbacks are issued.  This is important since not every callback type
    carries enough data about the subject package; while the INST types
    provide the user object passed to a former addInstall call, the UNINST
    types only provide the package name (which may not be unique within the
    transaction set).
    
    This commit adds such a callback.

    RPMCALLBACK_ELEM_PROGRESS<->= (1 << 19),
*/


}  // namespace libdnf::rpm

#endif
