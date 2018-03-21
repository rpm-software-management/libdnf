
#ifndef LIBDNF_NULLPROFILE_HPP
#define LIBDNF_NULLPROFILE_HPP


#include "Profile.hpp"

class NullProfile : public Profile
{
public:
    NullProfile() : Profile() {};

    std::string getName() const override { throw exception(); };
    std::string getDescription() const override  { throw exception(); };
    std::vector<std::string> getContent() const override { throw exception(); };
    bool hasRpm(const std::string &rpm) const override { throw exception(); };

private:
    std::string exception() const { return "Non-existent profile"; };
};


#endif //LIBDNF_NULLPROFILE_HPP

