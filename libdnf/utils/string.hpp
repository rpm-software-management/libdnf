#include <string>
#include <vector>


namespace libdnf::utils::string {
    /// @replaces libdnf:utils.hpp:function:fromCstring(const char * cstring);
    std::string from_cstring(const char * cstring);

    // SPLIT

    /// @replaces libdnf:utils.hpp:function:split(const std::string & source, const char * delimiter, int maxSplit)
    std::vector<std::string> split(const std::string & str, const char * separator, int maxsplit);

    /// @replaces libdnf:utils.hpp:function:rsplit(const std::string & source, const char * delimiter, int maxSplit)
    std::vector<std::string> rsplit(const std::string & str, const char * separator, int maxsplit);

    // STARTS/ENDS WITH

    /// @replaces libdnf:utils.hpp:function:startsWith(const std::string & source, const std::string & toMatch)
    bool startswith(const std::string & str, const std::string & prefix);

    /// @replaces libdnf:utils.hpp:function:endsWith(const std::string & source, const std::string & toMatch)
    bool endswith(const std::string & str, const std::string & suffix);

    // STRIP

    /// @replaces libdnf:utils.hpp:function:trim(const std::string & source)
    std::string strip(const std::string & str);
    std::string strip(const std::string & str, const std::string & chars);

    std::string lstrip(const std::string & str);
    std::string lstrip(const std::string & str, const std::string & chars);

    std::string rstrip(const std::string & str);
    std::string rstrip(const std::string & str, const std::string & chars);

    // PREFIX/SUFFIX REMOVAL

    std::string remove_suffix(const std::string & str, const std::string & suffix);
    std::string remove_prefix(const std::string & str, const std::string & prefix);
}
