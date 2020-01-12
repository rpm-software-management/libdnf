// TODO: how to handle compatibility with existing libraries/programs when a new handle appears;
// TODO: will this be the same interface as dnf:dnf/yum/rpmtrans.py:class:TransactionDisplay ?

class TransactionSetCallbackBase {
    void handle_RPMCALLBACK_UNKNOWN();

    // obsolete:
    // RPMCALLBACK_REPACKAGE_START
    // RPMCALLBACK_REPACKAGE_PROGRESS
    // RPMCALLBACK_REPACKAGE_STOP

    void handle_RPMCALLBACK_INST_OPEN_FILE();
    void handle_RPMCALLBACK_INST_CLOSE_FILE();

    void handle_RPMCALLBACK_UNPACK_ERROR();
    void handle_RPMCALLBACK_CPIO_ERROR();

    void handle_RPMCALLBACK_INST_START();
    void handle_RPMCALLBACK_INST_PROGRESS();
    void handle_RPMCALLBACK_INST_STOP();

    void handle_RPMCALLBACK_UNINST_START();
    void handle_RPMCALLBACK_UNINST_PROGRESS();
    void handle_RPMCALLBACK_UNINST_STOP();

    void handle_RPMCALLBACK_TRANS_START();
    void handle_RPMCALLBACK_TRANS_PROGRESS();
    void handle_RPMCALLBACK_TRANS_STOP();

    void handle_RPMCALLBACK_SCRIPT_START();
    void handle_RPMCALLBACK_SCRIPT_STOP();
    void handle_RPMCALLBACK_SCRIPT_ERROR();

    void handle_RPMCALLBACK_VERIFY_START();
    void handle_RPMCALLBACK_VERIFY_PROGRESS();
    void handle_RPMCALLBACK_VERIFY_STOP();

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
