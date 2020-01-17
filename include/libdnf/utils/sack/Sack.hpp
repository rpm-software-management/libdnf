#ifndef LIBDNF_UTILS_SACK_SACK_HPP
#define LIBDNF_UTILS_SACK_SACK_HPP


#include "Query.hpp"
#include "Set.hpp"


namespace libdnf::utils::sack {


template <typename T>
class Sack {
public:
    /// Delete all objects in the data set.
    ~Sack();

    /// Create a new query object for filtering the data set.
    Query<T> new_query();

    // EXCLUDES

    const Set<T> get_excludes() { return excludes; }
    void add_excludes(const Set<T> & value) { excludes.update(value); }
    void remove_excludes(const Set<T> & value) { excludes.difference(value); }
    void set_excludes(const Set<T> & value) { excludes = value; }

    // INCLUDES

    const Set<T> get_includes() const { return includes; }
    void add_includes(const Set<T> & value) { includes.update(value); }
    void remove_includes(const Set<T> & value) { includes.difference(value); }
    void set_includes(const Set<T> & value) { includes = value; }
    bool get_use_includes() { return use_includes; }
    void set_use_includes(bool value) { use_includes = value; }

protected:
    Set<T> & get_data() { return data; }

private:
    Set<T> data;  // Owns the data set. Objects get deleted when the Sack is deleted.
    Set<T> excludes;
    Set<T> includes;
    bool use_includes = false;
};


template <typename T>
Sack<T>::~Sack() {
    for (auto & it : data.get_data()) {
        delete it;
    }
};


template <typename T>
Query<T> Sack<T>::new_query() {
    Query<T> result;
    result.update(data);

    // if includes are used, remove everything else from the query
    if (get_use_includes()) {
        result.intersection(includes);
    }

    // apply excludes
    result.difference(excludes);

    return result;
}


}  // namespace libdnf::utils::sack


#endif
