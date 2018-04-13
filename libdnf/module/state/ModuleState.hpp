#ifndef LIBDNF_MODULESTATE_HPP
#define LIBDNF_MODULESTATE_HPP


class ModuleState
{
public:
//    void install() = 0;
//    void update() = 0;
//    void remove() = 0;
    virtual void enable();
//    void disable() = 0;

    virtual bool isEnabled();
};


#endif //LIBDNF_MODULESTATE_HPP
