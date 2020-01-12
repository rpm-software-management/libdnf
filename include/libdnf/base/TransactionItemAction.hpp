#pragma once


/*
dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_CLEANUP
dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_SCRIPTLET
dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_VERIFY
dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_CLEANUP
dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_SCRIPTLET
dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_VERIFY
dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_CLEANUP
dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_SCRIPTLET
dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_VERIFY
*/

namespace libdnf {


enum class TransactionItemAction : int {
    /// foo
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_INSTALL
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_INSTALL
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_INSTALL
    INSTALL = 1,       // a new package that was installed on the system

    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_DOWNGRADE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_DOWNGRADE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_DOWNGRADE
    DOWNGRADE = 2,     // an older package version that replaced previously installed version

    DOWNGRADED = 3,    // an original package version that was replaced

    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_OBSOLETE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_OBSOLETE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_OBSOLETE
    OBSOLETE = 4,      //

    OBSOLETED = 5,     //

    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_UPGRADE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_UPGRADE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_UPGRADE
    UPGRADE = 6,       //

    UPGRADED = 7,      //

    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_ERASE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_ERASE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_ERASE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_REMOVE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_REMOVE
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_REMOVE
    REMOVE = 8,        // a package that was removed from the system

    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:ErrorTransactionDisplay.PKG_REINSTALL
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:LoggingTransactionDisplay.PKG_REINSTALL
    /// @replaces dnf:dnf/yum/rpmtrans.py:attribute:TransactionDisplay.PKG_REINSTALL
    REINSTALL = 9,     // a package that was reinstalled with the identical version

    REINSTALLED = 10,  // a package that was reinstalled with the identical version (old repo, for example)

    REASON_CHANGE = 11 // a package was kept on the system but it's reason has changed
};


}  // namespace libdnf
