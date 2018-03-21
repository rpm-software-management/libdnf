
#ifndef LIBDNF_MODULEPROFILE_HPP
#define LIBDNF_MODULEPROFILE_HPP


#include "Profile.hpp"

#include <memory>
#include <modulemd/modulemd.h>

class ModuleProfile : public Profile
{
public:
    explicit ModuleProfile(ModulemdProfile *profile);
    ~ModuleProfile() override = default;

    std::string getName() const override;
    std::string getDescription() const override;
    std::vector<std::string> getContent() const override;

    bool hasRpm(const std::string &rpm) const override;

private:
    ModulemdProfile *profile;
};


#endif //LIBDNF_MODULEPROFILE_HPP
