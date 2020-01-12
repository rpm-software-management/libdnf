#pragma once


#include <string>


namespace libdnf::repo {


/// @replaces dnf:dnf/repo.py:class:Repo
class Repo {
public:
    /// @replaces dnf:dnf/repo.py:attribute:Repo.id
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.getId()
    /// @replaces libdnf:libdnf/dnf-repo.h:function:dnf_repo_get_id(DnfRepo * repo)
    std::string get_repo_id() const;

    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.isEnabled()
    /// @replaces libdnf:libdnf/dnf-repo.h:function:dnf_repo_get_enabled(DnfRepo * repo)
    bool get_enabled() const;

    /// @replaces dnf:dnf/repo.py:method:Repo.disable(self)
    /// @replaces dnf:dnf/repo.py:method:Repo.enable(self)
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.enable()
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.disable()
    /// @replaces libdnf:libdnf/dnf-repo.h:function:dnf_repo_set_enabled(DnfRepo * repo, DnfRepoEnabled enabled)
    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_repo_enable(DnfContext * context, const gchar * repo_id, GError ** error)
    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_repo_disable(DnfContext * context, const gchar * repo_id, GError ** error)
    /// TODO: enable -> load to sack(s) (first time only), enable in sack(s)
    /// TODO: disable -> must remain in sack(s), disable in sack(s)
    /// TODO: consider simple integration with rpm/comps/modules/etc.; every enable/disable changes underlying sack data!
    void set_enabled(bool value);

    /// @replaces dnf:dnf/repo.py:method:Repo.add_metadata_type_to_download(self, metadata_type)
    /// @replaces libdnf:libdnf/dnf-repo.h:function:dnf_repo_add_metadata_type_to_download(DnfRepo * repo, const gchar * metadataType)
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.addMetadataTypeToDownload(const std::string & metadataType)
    void add_metadata_type_to_download(std::string & metadata_type);

    /// @replaces dnf:dnf/repo.py:method:Repo.remove_metadata_type_from_download(self, metadata_type)
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.removeMetadataTypeFromDownload(const std::string & metadataType)
    void remove_metadata_type_to_download(std::string & metadata_type);

    /// @replaces dnf:dnf/repo.py:method:Repo.get_metadata_content(self, metadata_type)
    /// @replaces libdnf:libdnf/dnf-repo.h:function:dnf_repo_get_metadata_content(DnfRepo * repo, const gchar * metadataType, gpointer * content, int * length, GError ** error)
    /// TODO: return a stream
    void get_metadata_content(std::string & metadata_type);

    /// @replaces dnf:dnf/repo.py:method:Repo.get_http_headers(self)
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.getHttpHeaders()
    void get_http_headers();

    /// @replaces dnf:dnf/repo.py:method:Repo.set_http_headers(self, headers)
    /// @replaces libdnf:libdnf/repo/Repo.hpp:method:Repo.setHttpHeaders(const char *[] headers)
    void set_http_headers();

    // lukash: Member variables are missing, right? I'm thinking we should add them, while not API, they are integral part of the design, they show the relations between classes (well, some of them).
    // lukash: And the libsolv member variables (mainly the Pool?) will make it clear what holds the "shared" data that need to be stored through base.
};


}  // namespace libdnf::repo

/*
dnf:dnf/repo.py:method:Repo.cache_pkgdir(self)
dnf:dnf/repo.py:method:Repo.dump(self)
dnf:dnf/repo.py:method:Repo.get_http_headers(self)
dnf:dnf/repo.py:method:Repo.get_metadata_path(self, metadata_type)
dnf:dnf/repo.py:method:Repo.load(self)
dnf:dnf/repo.py:method:Repo.remote_location(self, location, schemes=('http', 'ftp', 'file', 'https'))
dnf:dnf/repo.py:method:Repo.set_progress_bar(self, progress)
dnf:dnf/repo.py:method:Repo.write_raw_configfile(filename, section_id, substitutions, modify)
dnf:dnf/repo.py:attribute:Repo.DEFAULT_SYNC
dnf:dnf/repo.py:attribute:Repo.load_metadata_other
dnf:dnf/repo.py:attribute:Repo.pkgdir
dnf:dnf/repo.py:attribute:Repo.repofile

libdnf:libdnf/repo/Repo-private.hpp:class:Key
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getId()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getUserId()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getFingerprint()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getTimestamp()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getId()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getUserId()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getFingerprint()
libdnf:libdnf/repo/Repo-private.hpp:method:Key.getTimestamp()
libdnf:libdnf/repo/Repo-private.hpp:class:Impl
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.load()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.loadCache(bool throwExcept)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.downloadMetadata(const std::string & destdir)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.fetch(const std::string & destdir, int && h)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getCachedir()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getPersistdir()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getAge()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.expire()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isExpired()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getExpiresIn()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.downloadUrl(const char * url, int fd)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.setHttpHeaders(const char *[] headers)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getHttpHeaders()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getMetadataPath(const std::string & metadataType)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitBase()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitLocal()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitRemote(const char * destdir, bool mirrorSetup, bool countme)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.attachLibsolvRepo(libdnf::LibsolvRepo * libsolvRepo)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.detachLibsolvRepo()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getCachedHandle()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandlePerform(LrHandle * handle, const std::string & destDirectory, bool setGPGHomeDir)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.embedCountmeFlag(std::string & url)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isMetalinkInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isRepomdInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.resetMetadataExpired()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.retrieve(const std::string & url)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.importRepoKeys()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.progressCB(void * data, double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.fastestMirrorCB(void * data, LrFastestMirrorStages stage, void * ptr)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.mirrorFailureCB(void * data, const char * msg, const char * url, const char * metadata)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.endsWith(const std::string & str, const std::string & ending)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getHash()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.load()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.loadCache(bool throwExcept)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.downloadMetadata(const std::string & destdir)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.fetch(const std::string & destdir, int && h)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getCachedir()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getPersistdir()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getAge()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.expire()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isExpired()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getExpiresIn()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.downloadUrl(const char * url, int fd)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.setHttpHeaders(const char *[] headers)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getHttpHeaders()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getMetadataPath(const std::string & metadataType)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitBase()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitLocal()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandleInitRemote(const char * destdir, bool mirrorSetup, bool countme)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.attachLibsolvRepo(libdnf::LibsolvRepo * libsolvRepo)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.detachLibsolvRepo()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getCachedHandle()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.lrHandlePerform(LrHandle * handle, const std::string & destDirectory, bool setGPGHomeDir)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.embedCountmeFlag(std::string & url)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isMetalinkInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.isRepomdInSync()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.resetMetadataExpired()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.retrieve(const std::string & url)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.importRepoKeys()
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.progressCB(void * data, double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.fastestMirrorCB(void * data, LrFastestMirrorStages stage, void * ptr)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.mirrorFailureCB(void * data, const char * msg, const char * url, const char * metadata)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.endsWith(const std::string & str, const std::string & ending)
libdnf:libdnf/repo/Repo-private.hpp:method:Impl.getHash()
libdnf:libdnf/repo/Repo.hpp:class:LrException
libdnf:libdnf/repo/Repo.hpp:method:LrException.getCode()
libdnf:libdnf/repo/Repo.hpp:method:LrException.getCode()
libdnf:libdnf/repo/Repo.hpp:class:RepoCB
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.start(const char * what)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.end()
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.progress(double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.fastestMirror(libdnf::RepoCB::FastestMirrorStage stage, const char * msg)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.handleMirrorFailure(const char * msg, const char * url, const char * metadata)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.repokeyImport(const std::string & id, const std::string & userId, const std::string & fingerprint, const std::string & url, long timestamp)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.start(const char * what)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.end()
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.progress(double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.fastestMirror(libdnf::RepoCB::FastestMirrorStage stage, const char * msg)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.handleMirrorFailure(const char * msg, const char * url, const char * metadata)
libdnf:libdnf/repo/Repo.hpp:method:RepoCB.repokeyImport(const std::string & id, const std::string & userId, const std::string & fingerprint, const std::string & url, long timestamp)
libdnf:libdnf/repo/Repo.hpp:class:Impl
libdnf:libdnf/repo/Repo.hpp:class:PackageTargetCB
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.end(libdnf::PackageTargetCB::TransferStatus status, const char * msg)
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.progress(double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.mirrorFailure(const char * msg, const char * url)
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.end(libdnf::PackageTargetCB::TransferStatus status, const char * msg)
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.progress(double totalToDownload, double downloaded)
libdnf:libdnf/repo/Repo.hpp:method:PackageTargetCB.mirrorFailure(const char * msg, const char * url)
libdnf:libdnf/repo/Repo.hpp:class:Impl
libdnf:libdnf/repo/Repo.hpp:function:repoGetImpl(libdnf::Repo * repo)

*/
