#pragma once


#include <string>
#include <vector>


namespace libdnf {


/// Demands are runtime options that are not instance wide
/// and can differ for each method call.
class Demands {
public:
    // allowerasing
    bool getAllowErasing() const { return allowErasing; }
    void setAllowErasing(bool value) { allowErasing = value; }

    // best
    bool getBest() const { return best; }
    void setBest(bool value) { best = value; }

    // cacheonly: 0 = off; 1 = repodata; 2 = repodata + packages
    int getCacheOnlyLevel() const { return cacheOnlyLevel; }
    void setCacheOnlyLevel(int value) { cacheOnlyLevel = value; }

private:
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
