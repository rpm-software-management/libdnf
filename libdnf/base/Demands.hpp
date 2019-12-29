#pragma once


#include <string>
#include <vector>


namespace libdnf {


/// Demands are runtime options that are not instance wide
/// and can differ for each method call.
class Demands {
public:
    // allowerasing
    bool get_allow_erasing() const { return allowErasing; }
    void set_allow_erasing(bool value) { allowErasing = value; }

    // best
    bool get_best() const { return best; }
    void set_best(bool value) { best = value; }

    // cacheonly: 0 = off; 1 = repodata; 2 = repodata + packages
    int get_cache_only_level() const { return cacheOnlyLevel; }
    void set_cache_only_level(int value) { cacheOnlyLevel = value; }

protected:
    bool allowErasing = false;
    bool best = false;
    int cacheOnlyLevel = 0;
    std::vector<std::string> fromRepo;
    /*
    available_repos = _BoolDefault(False)
    resolving = _BoolDefault(False)
    root_user = _BoolDefault(False)
    sack_activation = _BoolDefault(False)
    load_system_repo = _BoolDefault(True)
    cacheonly = _BoolDefault(False)
    fresh_metadata = _BoolDefault(True)
    freshest_metadata = _BoolDefault(False)
    changelogs = _BoolDefault(False)

    TODO: debugsolver
    */
};

}  // namespace libdnf
