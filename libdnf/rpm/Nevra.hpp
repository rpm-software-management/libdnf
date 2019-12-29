#pragma once


namespace libdnf::rpm {


/// @replaces libdnf:libdnf/nevra.hpp:class:Nevra
class Nevra {
public:
    Nevra();

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.parse(const char * nevraStr, HyForm form)
    Nevra(std::string str);

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.compare(const libdnf::Nevra & nevra2)
    bool operator==(const Nevra & other) const;

    std::string to_string() const noexcept;

    // NAME

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getName()
    const std::string & get_name() const noexcept;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setName(const std::string & name)
    void set_name(const std::string & name);

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setName(std::string && name)
    void set_name(std::string && name);

    // EPOCH

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getEpoch()
    int64_t get_epoch() const noexcept;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setEpoch(int epoch)
    void set_epoch(int64_t epoch);

    // VERSION

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getVersion()
    const std::string & get_version() const noexcept;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setVersion(const std::string & version)
    void set_version(const std::string & version);

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setVersion(std::string && version)
    void set_version(std::string && version);

    // RELEASE

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getRelease()
    const std::string & get_release() const noexcept;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setRelease(const std::string & release)
    void set_release(const std::string & release);

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setRelease(std::string && release)
    void set_release(std::string && release);

    // ARCH

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getArch()
    const std::string & get_arch() const noexcept;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setArch(const std::string & arch)
    void set_arch(const std::string & arch);

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.setArch(std::string && arch)
    void set_arch(std::string && arch);

protected:
    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.getEvr()
    std::string get_evr() const;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.hasJustName()
    bool has_just_name() const;

    /// @replaces libdnf:libdnf/nevra.hpp:method:Nevra.compareEvr(const libdnf::Nevra & nevra2, DnfSack * sack)
    int compare_evr(const Nevra & nevra2, DnfSack *sack) const;
};


}  // namespace libdnf::rpm
