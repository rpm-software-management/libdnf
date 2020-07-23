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


#include "libdnf/rpm/package.hpp"

#include "reldep_list_impl.hpp"
#include "solv/package_private.hpp"
#include "solv_sack_impl.hpp"
#include <string>


inline static std::string cstring2string(const char * input) {
    return input ? std::string(input) : std::string();
}


namespace libdnf::rpm {

const char * Package::get_name_cstring() const noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_name(pool, id);
}

std::string Package::get_name() const {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_name(pool, id));
}

const char * Package::get_epoch_cstring() {
    Pool * pool = sack->pImpl->pool;
    return solv::get_epoch_cstring(pool, id);
}

unsigned long Package::get_epoch() {
    Pool * pool = sack->pImpl->pool;
    return solv::get_epoch(pool, id);
}

const char * Package::get_version_cstring() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_version(pool, id);
}

std::string Package::get_version() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_version(pool, id));
}

const char * Package::get_release_cstring() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_release(pool, id);
}

std::string Package::get_release() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_release(pool, id));
}

const char * Package::get_arch_cstring() const noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_arch(pool, id);
}

std::string Package::get_arch() const {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_arch(pool, id));
}

const char * Package::get_evr_cstring() const noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_evr(pool, id);
}

std::string Package::get_evr() const {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_evr(pool, id));
}

std::string Package::get_nevra() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_nevra(pool, id));
}

std::string Package::get_full_nevra() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_full_nevra(pool, id));
}

std::string Package::get_group() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_group(pool, id));
}

unsigned long long Package::get_size() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_size(pool, id);
}

unsigned long long Package::get_download_size() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_download_size(pool, id);
}

unsigned long long Package::get_install_size() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_install_size(pool, id);
}

std::string Package::get_license() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_license(pool, id));
}

std::string Package::get_sourcerpm() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_sourcerpm(pool, id));
}

unsigned long long Package::get_build_time() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_build_time(pool, id);
}

std::string Package::get_build_host() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_build_host(pool, id));
}

std::string Package::get_packager() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_packager(pool, id));
}

std::string Package::get_vendor() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_vendor(pool, id));
}

std::string Package::get_url() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_url(pool, id));
}

std::string Package::get_summary() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_summary(pool, id));
}

std::string Package::get_description() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_description(pool, id));
}

std::vector<std::string> Package::get_files() {
    Pool * pool = sack->pImpl->pool;
    return solv::get_files(pool, id);
}

ReldepList Package::get_provides() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_provides(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_requires() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_provides(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_requires_pre() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_requires_pre(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_conflicts() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_conflicts(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_obsoletes() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_obsoletes(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_recommends() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_recommends(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_suggests() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_suggests(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_enhances() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_enhances(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_supplements() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_supplements(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_prereq_ignoreinst() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_prereq_ignoreinst(pool, id, list.pImpl->queue);
    return list;
}

ReldepList Package::get_regular_requires() const {
    Pool * pool = sack->pImpl->pool;
    ReldepList list(sack.get());
    solv::get_regular_requires(pool, id, list.pImpl->queue);
    return list;
}

std::string Package::get_baseurl() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_baseurl(pool, id));
}

std::string Package::get_location() {
    Pool * pool = sack->pImpl->pool;
    return cstring2string(solv::get_location(pool, id));
}

bool Package::is_installed() const {
    Pool * pool = sack->pImpl->pool;
    return solv::is_installed(pool, solv::get_solvable(pool, id));
}

unsigned long long Package::get_hdr_end() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_hdr_end(pool, id);
}

unsigned long long Package::get_install_time() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_install_time(pool, id);
}

unsigned long long Package::get_media_number() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_media_number(pool, id);
}

unsigned long long Package::get_rpmdbid() noexcept {
    Pool * pool = sack->pImpl->pool;
    return solv::get_rpmdbid(pool, id);
}

DnfVariant Package::serialize() {
    DnfVariant s_pkg = {
        { "id", this->get_id() },
        { "name", this->get_name() },
        { "epoch", this->get_epoch() },
        { "version", this->get_version() },
        { "release", this->get_release() },
        { "arch", this->get_arch() },
        { "evr", this->get_evr() },
        { "nevra", this->get_nevra() },
        { "full_nevra", this->get_full_nevra() },
        { "group", this->get_group() },
        { "size", this->get_size() },
        { "download_size", this->get_download_size() },
        { "install_size", this->get_install_size() },
        { "license", this->get_license() },
        { "sourcerpm", this->get_sourcerpm() },
        { "build_time", this->get_build_time() },
        { "build_host", this->get_build_host() },
        { "packager", this->get_packager() },
        { "vendor", this->get_vendor() },
        { "url", this->get_url() },
        { "summary", this->get_summary() },
        { "description", this->get_description() },
        { "files", this->get_files() },
        // dependencies
        { "provides", this->get_provides() },
        { "requires", this->get_requires() },
        { "requires_pre", this->get_requires_pre() },
        { "conflicts", this->get_conflicts() },
        { "obsoletes", this->get_obsoletes() },
        { "prereq_ignoreinst", this->get_prereq_ignoreinst() },
        { "regular_requires", this->get_regular_requires() },
        // weak dependencies
        { "recommends", this->get_recommends() },
        { "suggests", this->get_suggests() },
        { "enhances", this->get_enhances() },
        { "supplements", this->get_supplements() },
        // repodata
        { "baseurl", this->get_baseurl() },
        { "location", this->get_location() },
        { "checksum", this->get_checksum() },
        { "hdr_checksum", this->get_hdr_checksum() },
        //system
        { "is_installed", this->is_installed() },
        { "hdr_end", this->get_hdr_end() },
        { "install time", this->get_install_time() },
        { "media_number", this->get_media_number() },
        { "rpmdbid", this->get_rpmdbid() }
    };
    return s_pkg;
}

Checksum Package::get_checksum() {
    Pool * pool = sack->pImpl->pool;

    Solvable * solvable = solv::get_solvable(pool, id);
    int type;
    solv::SolvPrivate::internalize_libsolv_repo(solvable->repo);
    const char * chksum = solvable_lookup_checksum(solvable, SOLVABLE_CHECKSUM, &type);
    Checksum checksum(chksum, type);

    return checksum;
}

Checksum Package::get_hdr_checksum() {
    Pool * pool = sack->pImpl->pool;

    Solvable * solvable = solv::get_solvable(pool, id);
    int type;
    solv::SolvPrivate::internalize_libsolv_repo(solvable->repo);
    const char * chksum = solvable_lookup_checksum(solvable, SOLVABLE_HDRID, &type);
    Checksum checksum(chksum, type);

    return checksum;
}

}  // namespace libdnf::rpm
