/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#include "MergedTransaction.hpp"
#include <algorithm>
#include <vector>

namespace libdnf {

/**
 * Create a new MergedTransaction object with a single transaction
 * \param trans initial transaction
 */
MergedTransaction::MergedTransaction(TransactionPtr trans)
  : transactions{trans}
{
}

/**
 * Merge \trans into this transaction
 * Internally, transactions are kept in a sorted vector, what allows to
 *  easily access merged transaction properties on demand.
 * \param trans transaction to be merged with
 */
void
MergedTransaction::merge(TransactionPtr trans)
{
    bool inserted = false;
    for (auto it = transactions.begin(); it < transactions.end(); ++it) {
        if ((*it)->getId() > trans->getId()) {
            transactions.insert(it, trans);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        transactions.push_back(trans);
    }
}

/**
 * Get IDs of the transactions involved in the merged transaction
 * \return list of transaction IDs sorted in ascending order
 */
std::vector< int64_t >
MergedTransaction::listIds() const
{
    std::vector< int64_t > ids;
    for (auto t : transactions) {
        ids.push_back(t->getId());
    }
    return ids;
}

/**
 * Get UNIX IDs of users who performed the transaction.
 * \return list of user IDs sorted by transaction ID in ascending order
 */
std::vector< uint32_t >
MergedTransaction::listUserIds() const
{
    std::vector< uint32_t > users;
    for (auto t : transactions) {
        users.push_back(t->getUserId());
    }
    return users;
}

/**
 * Get list of commands that started the transaction
 * \return list of commands sorted by transaction ID in ascending order
 */
std::vector< std::string >
MergedTransaction::listCmdlines() const
{
    std::vector< std::string > cmdLines;
    for (auto t : transactions) {
        cmdLines.push_back(t->getCmdline());
    }
    return cmdLines;
}

std::vector< TransactionPersistence >
MergedTransaction::listPersistences() const
{
    std::vector< TransactionPersistence > persistences;
    for (auto t : transactions) {
        persistences.push_back(t->getPersistence());
    }
    return persistences;
}

std::vector< TransactionState >
MergedTransaction::listStates() const
{
    std::vector< TransactionState > result;
    for (auto t : transactions) {
        result.push_back(t->getState());
    }
    return result;
}

std::vector< std::string >
MergedTransaction::listReleasevers() const
{
    std::vector< std::string > result;
    for (auto t : transactions) {
        result.push_back(t->getReleasever());
    }
    return result;
}

std::vector< std::string >
MergedTransaction::listComments() const
{
    std::vector< std::string > result;
    for (auto t : transactions) {
        result.push_back(t->getComment());
    }
    return result;
}


int64_t
MergedTransaction::getDtBegin() const noexcept
{
    return transactions.front()->getDtBegin();
}
int64_t
MergedTransaction::getDtEnd() const noexcept
{
    return transactions.back()->getDtEnd();
}
const std::string &
MergedTransaction::getRpmdbVersionBegin() const noexcept
{
    return transactions.front()->getRpmdbVersionBegin();
}

const std::string &
MergedTransaction::getRpmdbVersionEnd() const noexcept
{
    return transactions.back()->getRpmdbVersionEnd();
}

std::set< RPMItemPtr >
MergedTransaction::getSoftwarePerformedWith() const
{
    std::set< RPMItemPtr > software;
    for (auto t : transactions) {
        auto tranSoft = t->getSoftwarePerformedWith();
        software.insert(tranSoft.begin(), tranSoft.end());
    }
    return software;
}

std::vector< std::pair< int, std::string > >
MergedTransaction::getConsoleOutput()
{
    std::vector< std::pair< int, std::string > > output;
    for (auto t : transactions) {
        auto tranOutput = t->getConsoleOutput();
        output.insert(output.end(), tranOutput.begin(), tranOutput.end());
    }
    return output;
}


static bool transaction_item_sort_function(const std::shared_ptr<TransactionItemBase> lhs, const std::shared_ptr<TransactionItemBase> rhs) {
    if (lhs->isForwardAction() && rhs->isForwardAction()) {
        return false;
    }
    if (lhs->isBackwardAction() && rhs->isBackwardAction()) {
        return false;
    }
    if (lhs->isBackwardAction()) {
        return true;
    }
    return false;
}


/**
 * Get list of transaction items involved in the merged transaction
 * Actions are merged using following rules:
 * (old action) -> (new action) = (merged action)
 *
 * Erase/Obsolete -> Install/Obsoleting = Downgrade/Upgrade
 *
 * Reinstall/Reason change -> (new action) = (new action)
 *
 * Install -> Erase = (nothing)
 *
 * Install -> Upgrade/Downgrade = Install (with Upgrade version)
 *
 * Downgrade/Upgrade/Obsoleting -> Reinstall = (old action)
 *
 * Downgrade/Upgrade/Obsoleting -> Erase/Obsoleted = Erase/Obsolete (with old package)
 *
 * Downgrade/Upgrade/Obsoleting -> Downgraded/Upgrade =
 *      We have differentiate between original transaction, and new one.
 *      When a transaction package pair is not complete, then we are still in original one.
 *
 *      With complete transaction pair we need to get a new Upgrade/Downgrade package and
 *      compare versions with original package from pair.
 *
 * Additionally, if a package is installed both before and after the list of transactions
 * with the same version, no action will be taken.
 */
std::vector< TransactionItemBasePtr >
MergedTransaction::getItems()
{
    ItemPairMap itemPairMap;

    // iterate over transaction
    for (auto t : transactions) {
        auto transItems = t->getItems();
        // sort transaction items by their action type - forward/backward
        // this fixes behavior of the merging algorithm in several edge cases
        std::sort(transItems.begin(), transItems.end(), transaction_item_sort_function);
        // iterate over transaction items
        for (auto transItem : transItems) {
            // get item and its type
            auto mTransItem = std::dynamic_pointer_cast< TransactionItemBase >(transItem);
            mergeItem(itemPairMap, mTransItem);
        }
    }

    std::vector< TransactionItemBasePtr > items;
    for (const auto &row : itemPairMap) {
        for (const auto &itemPair : row.second) {
            items.push_back(itemPair.first);
            if (itemPair.second != nullptr) {
                items.push_back(itemPair.second);
            }
        }
    }
    return items;
}

static std::string
getItemIdentifier(ItemPtr item)
{
    auto itemType = item->getItemType();
    std::string name;
    if (itemType == ItemType::RPM) {
        auto rpm = std::dynamic_pointer_cast< RPMItem >(item);
        name = rpm->getName() + "." + rpm->getArch();
    } else if (itemType == ItemType::GROUP) {
        auto group = std::dynamic_pointer_cast< CompsGroupItem >(item);
        name = group->getGroupId();
    } else if (itemType == ItemType::ENVIRONMENT) {
        auto env = std::dynamic_pointer_cast< CompsEnvironmentItem >(item);
        name = env->getEnvironmentId();
    }
    return name;
}

/**
 * Resolve the difference between RPMs in the first and second transaction item
 *  and create a ItemPair of Upgrade, Downgrade or remove the item from the merged
 *  transaction set in case of both packages are the same.
 * Method is called when original package is being removed and then installed again.
 * \param entries NEVRA entries tracked for this name.arch
 * \param entryIt entry being resolved
 * \param mTransItem new transaction item
 * \return true if the original and new transaction item differ
 */
bool
MergedTransaction::resolveRPMDifference(std::vector< ItemPair > &entries,
                                        ItemPairEntry entryIt,
                                        TransactionItemBasePtr mTransItem)
{
    ItemPair &previousItemPair = *entryIt;
    auto firstItem = previousItemPair.first->getItem();
    auto secondItem = mTransItem->getItem();

    auto firstRPM = std::dynamic_pointer_cast< RPMItem >(firstItem);
    auto secondRPM = std::dynamic_pointer_cast< RPMItem >(secondItem);

    if (firstRPM->getVersion() == secondRPM->getVersion() &&
        firstRPM->getEpoch() == secondRPM->getEpoch() &&
        firstRPM->getRelease() == secondRPM->getRelease()) {
        // Drop the item from merged transaction
        entries.erase(entryIt);
        return false;
    } else if ((*firstRPM) < (*secondRPM)) {
        // Upgrade to secondRPM
        previousItemPair.first->setAction(TransactionItemAction::UPGRADED);
        mTransItem->setAction(TransactionItemAction::UPGRADE);
    } else {
        // Downgrade to secondRPM
        previousItemPair.first->setAction(TransactionItemAction::DOWNGRADED);
        mTransItem->setAction(TransactionItemAction::DOWNGRADE);
    }
    previousItemPair.second = mTransItem;
    return true;
}

void
MergedTransaction::resolveErase(std::vector< ItemPair > &entries,
                                ItemPairEntry entryIt,
                                TransactionItemBasePtr mTransItem)
{
    ItemPair &previousItemPair = *entryIt;
    /*
     * The original item has been removed - it has to be installed now unless the rpmdb
     *  has changed. Resolve the difference between packages and mark it as Upgrade,
     *  Downgrade or remove it from the transaction
     */
    if (mTransItem->getAction() == TransactionItemAction::INSTALL) {
        if (mTransItem->getItem()->getItemType() == ItemType::RPM) {
            // resolve the difference between RPM packages
            if (!resolveRPMDifference(entries, entryIt, mTransItem)) {
                return;
            }
        } else {
            // difference between comps can't be resolved
            mTransItem->setAction(TransactionItemAction::REINSTALL);
        }
    }
    previousItemPair.first = mTransItem;
    previousItemPair.second = nullptr;
}

/**
 * Resolve altered - Upgrade(d)/Downgrade(d) transaction items.
 * If the new item is Erased or Obsoleted, than its action is transferred to the original pair.
 * When its being Downgraded/Upgraded and the pair is incomplete then we are in the same
 * transaction - new package is used to complete the pair. Items are stored in pairs (Upgrade,
 * Upgrade) or (Downgraded, Downgrade). With complete transaction pair we need to get the new
 * Upgrade/Downgrade item and compare its version with the original item from the pair.
 * \param entries NEVRA entries tracked for this name.arch
 * \param entryIt entry being resolved
 * \param mTransItem new transaction item
 */
void
MergedTransaction::resolveAltered(std::vector< ItemPair > &entries,
                                  ItemPairEntry entryIt,
                                  TransactionItemBasePtr mTransItem)
{
    ItemPair &previousItemPair = *entryIt;
    auto newState = mTransItem->getAction();
    auto firstState = previousItemPair.first->getAction();

    if (newState == TransactionItemAction::REMOVE || newState == TransactionItemAction::OBSOLETED) {
        // package is being Erased
        // move Erased action to the previous state
        previousItemPair.first->setAction(newState);
        previousItemPair.second = nullptr;
    } else if (newState == TransactionItemAction::DOWNGRADED ||
               newState == TransactionItemAction::UPGRADED) {
        // check if the transaction pair is complete
        if (previousItemPair.second == nullptr) {
            // pair might be in a wrong order
            if (firstState == TransactionItemAction::DOWNGRADE ||
                firstState == TransactionItemAction::UPGRADE) {
                // fix the order
                previousItemPair.second = previousItemPair.first;
                previousItemPair.first = mTransItem;
            }
        }
        // XXX handle obsoleting state -> state is not supported anymore, so it can't
        // occur anymore - maybe we should set some "Obsoleting" flag or what
        // state of obsoleting package should be transferred to a new package
        /*
         * Otherwise we can just drop the package
         * Original package from new transaction should be the same as a new package
         * from previous transaction - unless the RPMDB has altered.
         */

    } else if (newState == TransactionItemAction::DOWNGRADE ||
               newState == TransactionItemAction::UPGRADE) {
        /*
         * Check whether second item is missing in transaction pair
         * When it does, complete the transaction pair.
         */
        if (previousItemPair.second == nullptr) {
            previousItemPair.second = mTransItem;
        } else {
            if (mTransItem->getItem()->getItemType() == ItemType::RPM) {
                // resolve the difference between RPM packages
                resolveRPMDifference(entries, entryIt, mTransItem);
            } else {
                // difference between comps can't be resolved
                previousItemPair.second->setAction(TransactionItemAction::REINSTALL);
                previousItemPair.first = previousItemPair.second;
                previousItemPair.second = nullptr;
            }
        }
    }
}

/**
 * Merge transaction item into merged transaction set.
 *
 * With at most one entry tracked for this name.arch, dispatch is
 * unambiguous and uses the state machine above exactly like before several
 * coexisting NEVRAs were supported - including pairing a forward action
 * (typically Install) with the sole entry regardless of NEVRA when that
 * entry is currently Removed, which is how an Upgrade/Downgrade gets
 * synthesized out of two independently-recorded transactions. The only
 * addition is a relevance guard on backward actions: one whose NEVRA
 * doesn't match the sole tracked entry doesn't belong to it at all (e.g. a
 * Remove of some other NEVRA of the same name.arch that predates the merge
 * window) and becomes its own new, independent entry instead of being
 * misattributed to an unrelated package. A backward action that does match
 * proceeds through the state machine exactly as before (e.g. still doing
 * nothing while waiting for the paired forward half to arrive).
 *
 * Once a second entry exists, matching has to be NEVRA-exact rather than
 * reusing the state machine above: with several independent NEVRAs in play
 * there's no single "the" entry left to loosely pair anything with, and
 * reusing the mutation-heavy pairing logic here would also misinterpret
 * residue of an already-resolved Upgrade/Downgrade (its outgoing half
 * permanently relabeled Install by an earlier getItems() call, since
 * transaction items are mutable and cached) as a fresh, unrelated
 * coexisting NEVRA. Instead: a NEVRA that's already tracked either cancels
 * out (Install cancels a Remove and vice versa) or is a defensive no-op
 * (the same direction seen again, which shouldn't occur); anything else -
 * a second Install of a different NEVRA (installonly packages are the
 * common real-world reason two NEVRAs of the same name.arch coexist), or a
 * Remove of a NEVRA that predates the merge window - becomes its own new,
 * independent entry.
 * \param itemPairMap merged transaction set
 * \param mTransItem transaction item
 */
void
MergedTransaction::mergeItem(ItemPairMap &itemPairMap, TransactionItemBasePtr mTransItem)
{
    // get item identifier
    std::string name = getItemIdentifier(mTransItem->getItem());
    auto &entries = itemPairMap[name];
    bool isRPM = mTransItem->getItem()->getItemType() == ItemType::RPM;

    if (entries.size() <= 1) {
        if (entries.empty()) {
            entries.push_back(ItemPair(mTransItem, nullptr));
            return;
        }

        auto entryIt = entries.begin();

        if (isRPM && mTransItem->isBackwardAction()) {
            // Use whatever NEVRA this entry currently represents - the
            // completed side of an Upgrade/Downgrade pair if there is one,
            // otherwise the sole tracked item - since a backward action
            // continuing that pair (e.g. an Upgraded/Downgraded item
            // referencing the version it's replacing) names the *current*
            // NEVRA, not the original one still sitting in `first`.
            auto currentItem = (entryIt->second != nullptr) ? entryIt->second : entryIt->first;
            auto entryRPM = std::dynamic_pointer_cast< RPMItem >(currentItem->getItem());
            auto itemRPM = std::dynamic_pointer_cast< RPMItem >(mTransItem->getItem());
            if (entryRPM->getNEVRA() != itemRPM->getNEVRA()) {
                entries.push_back(ItemPair(mTransItem, nullptr));
                return;
            }
        }

        ItemPair &previousItemPair = *entryIt;
        auto firstState = previousItemPair.first->getAction();
        auto newState = mTransItem->getAction();

        switch (firstState) {
            case TransactionItemAction::REMOVE:
            case TransactionItemAction::OBSOLETED:
                resolveErase(entries, entryIt, mTransItem);
                break;
            case TransactionItemAction::INSTALL:
                // the original package has been installed -> it may be either Removed, or altered
                if (newState == TransactionItemAction::REMOVE ||
                    newState == TransactionItemAction::OBSOLETED) {
                    // Install -> Remove = (nothing)
                    entries.erase(entryIt);
                    break;
                } else if (mTransItem->isBackwardAction()) {
                    break;
                } else if (newState == TransactionItemAction::INSTALL && isRPM) {
                    // A second Install for the same name.arch with a
                    // different NEVRA - the transition to several
                    // coexisting entries. Track the new NEVRA independently
                    // instead of discarding the previous one.
                    auto firstRPM = std::dynamic_pointer_cast< RPMItem >(previousItemPair.first->getItem());
                    auto secondRPM = std::dynamic_pointer_cast< RPMItem >(mTransItem->getItem());
                    if (firstRPM->getNEVRA() != secondRPM->getNEVRA()) {
                        entries.push_back(ItemPair(mTransItem, nullptr));
                        break;
                    }
                }
                // altered -> transfer install to the altered package
                mTransItem->setAction(TransactionItemAction::INSTALL);
                // don't break
            case TransactionItemAction::REINSTALL:
            case TransactionItemAction::REASON_CHANGE:
                // The original item has been reinstalled or the reason has been changed
                // keep the new action
                previousItemPair.first = mTransItem;
                previousItemPair.second = nullptr;
                break;
            case TransactionItemAction::DOWNGRADE:
            case TransactionItemAction::DOWNGRADED:
            case TransactionItemAction::UPGRADE:
            case TransactionItemAction::UPGRADED:
            case TransactionItemAction::OBSOLETE:
                resolveAltered(entries, entryIt, mTransItem);
                break;
            case TransactionItemAction::REINSTALLED:
                break;
        }

        if (entries.empty()) {
            itemPairMap.erase(name);
        }
        return;
    }

    // Several entries already coexist for this name.arch - each one is
    // always a standalone Install or Remove (never a pending
    // Upgrade/Downgrade pair), so resolution is a simple, mutation-free
    // NEVRA-exact match instead of the pairing state machine above.
    if (isRPM) {
        auto itemNEVRA = std::dynamic_pointer_cast< RPMItem >(mTransItem->getItem())->getNEVRA();
        auto newState = mTransItem->getAction();
        for (auto entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
            auto entryRPM = std::dynamic_pointer_cast< RPMItem >(entryIt->first->getItem());
            if (entryRPM->getNEVRA() != itemNEVRA) {
                continue;
            }

            if (newState == TransactionItemAction::REASON_CHANGE) {
                // Neither installs nor removes the NEVRA, and unlike every
                // other action it's neither forward nor backward (dnf's own
                // reason inheritance for installonly packages can emit this
                // against a sibling NEVRA when another one is removed) - so
                // it can't be classified by direction like the branch
                // below. Absorb it into the existing entry instead, keeping
                // the entry's Install/Remove status unchanged (matching the
                // single-entry state machine's handling of the same
                // action).
                if (entryIt->first->getAction() == TransactionItemAction::INSTALL) {
                    mTransItem->setAction(TransactionItemAction::INSTALL);
                }
                entryIt->first = mTransItem;
            } else if (entryIt->first->isForwardAction() == mTransItem->isForwardAction()) {
                // Same NEVRA, same direction seen again - defensive no-op
                // (shouldn't occur), just keep tracking the newer item.
                entryIt->first = mTransItem;
            } else {
                // Install and Remove of the identical NEVRA cancel out.
                entries.erase(entryIt);
                if (entries.empty()) {
                    itemPairMap.erase(name);
                }
            }
            return;
        }
    }

    // No match - independent new item.
    entries.push_back(ItemPair(mTransItem, nullptr));
}

} // namespace libdnf
