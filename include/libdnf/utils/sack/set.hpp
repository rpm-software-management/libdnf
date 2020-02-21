#ifndef LIBDNF_UTILS_SACK_SET_HPP
#define LIBDNF_UTILS_SACK_SET_HPP

#include <algorithm>
#include <set>

namespace libdnf::utils::sack {


/// Set represents set of objects (e.g. packages, or groups)
/// and implements set operations such as unions or differences.
template <typename T>
class Set {
public:
    // GENERIC OPERATIONS
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); };
    void clear() { data.clear(); }

    // ITEM OPERATIONS

    void add(const T & obj) { data.insert(obj); }
    void remove(T & obj) { data.erase(obj); }
    bool contains(T & obj) const { return data.find(obj) != data.end(); }

    // SET OPERATIONS

    // update == union
    void update(const Set<T> & other);
    void difference(const Set<T> & other);
    void intersection(const Set<T> & other);
    void symmetric_difference(const Set<T> & other);

    // bool subset(const Set<T> & other);
    // bool superset(const Set<T> & other);

    /// Return reference to underlying std::set
    const std::set<T> & get_data() const noexcept { return data; }
    std::set<T> & get_data() noexcept { return data; }

private:
    std::set<T> data;
};


template <typename T>
inline void Set<T>::update(const Set<T> & other) {
    /*    std::set<T> result;
    std::set_union(data.begin(), data.end(), other.data.begin(), other.data.end(), std::inserter(result, result.begin()));
    data = result;*/
    data.insert(other.data.begin(), other.data.end());
}


template <typename T>
inline void Set<T>::difference(const Set<T> & other) {
    std::set<T> result;
    std::set_difference(
        data.begin(), data.end(), other.data.begin(), other.data.end(), std::inserter(result, result.begin()));
    data = result;
}


template <typename T>
inline void Set<T>::intersection(const Set<T> & other) {
    std::set<T> result;
    std::set_intersection(
        data.begin(), data.end(), other.data.begin(), other.data.end(), std::inserter(result, result.begin()));
    data = result;
}


template <typename T>
inline void Set<T>::symmetric_difference(const Set<T> & other) {
    std::set<T> result;
    std::set_symmetric_difference(
        data.begin(), data.end(), other.data.begin(), other.data.end(), std::inserter(result, result.begin()));
    data = result;
}


}  // namespace libdnf::utils::sack


#endif
