
#ifndef LIBDNF_PROFILE_HPP
#define LIBDNF_PROFILE_HPP


#include <string>
#include <vector>

class Profile
{
public:
    virtual ~Profile() = default;

    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<std::string> getContent() const = 0;

protected:
    Profile() = default;
};


#endif //LIBDNF_PROFILE_HPP
