#pragma once


#include <string>


namespace libdnf::module {


/// @replaces libdnf:libdnf/nsvcap.hpp:class:Nsvcap
class Nsvcap {
public:
    bool parse(const char *nsvcap_str, HyModuleForm form);
    void clear();

    // NAME

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getName()
    const std::string & get_name() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setName(const std::string & name)
    void set_name(const std::string & name);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setName(std::string && name)
    void set_name(std::string && name);

    // STREAM

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getStream()
    const std::string & get_stream() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setStream(const std::string & stream)
    void set_stream(const std::string & stream);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setStream(std::string && stream)
    void set_stream(std::string && stream);

    // VERSION

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getVersion()
    const std::string & get_version() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setVersion(const std::string & stream)
    void set_version(const std::string & stream);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setVersion(std::string && stream)
    void set_version(std::string && stream);

    // CONTEXT

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getContext()
    const std::string & get_context() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setContext(const std::string & context)
    void set_context(const std::string & context);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setContext(std::string && context)
    void set_context(std::string && context);

    // ARCH

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getArch()
    const std::string & get_arch() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setArch(const std::string & arch)
    void set_arch(const std::string & arch);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setArch(std::string && arch)
    void set_arch(std::string && arch);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.getProfile()
    const std::string & get_profile() const noexcept;

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setProfile(const std::string & profile)
    void set_profile(const std::string & profile);

    /// @replaces libdnf:libdnf/nsvcap.hpp:method:Nsvcap.setProfile(std::string && profile)
    void set_profile(std::string && profile);

protected:
    std::string name;
    std::string stream;
    // TODO: shouldn't version be uint64_t?
    std::string version;
    std::string context;
    std::string arch;
    std::string profile;
};
