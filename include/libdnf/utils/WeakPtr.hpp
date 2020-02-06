#ifndef LIBDNF_UTILS_WEAKPTR_HPP
#define LIBDNF_UTILS_WEAKPTR_HPP


#include <stdexcept>
#include <unordered_set>

namespace libdnf::utils {

template <typename TPtr, bool ptr_owner>
struct WeakPtr;


template <typename TPtr, bool weak_ptr_is_owner>
struct WeakPtrGuard {
public:
    using TWeakPtr = WeakPtr<TPtr, weak_ptr_is_owner>;

    WeakPtrGuard() = default;
    WeakPtrGuard(const WeakPtrGuard &) = delete;
    WeakPtrGuard(WeakPtrGuard && src) noexcept : registered_weak_ptrs(std::move(src.registered_weak_ptrs)) {
        for (auto it : registered_weak_ptrs)
            it->guard = this;
    }
    ~WeakPtrGuard() { clear(); }

    WeakPtrGuard & operator=(const WeakPtrGuard & src) = delete;
    WeakPtrGuard & operator=(WeakPtrGuard && src) noexcept {
        registered_weak_ptrs = std::move(src.registered_weak_ptrs);
        for (auto it : registered_weak_ptrs)
            it->guard = this;
    }

    /// Returns true if the guard is empty, false otherwise.
    bool empty() const noexcept { return registered_weak_ptrs.empty(); }

    /// Returns the number of registered weak pointers.
    size_t size() const noexcept { return registered_weak_ptrs.size(); }

    /// Deregisters and invalidates all registered weak pointers. After this call, size() returns zero.
    void clear() noexcept {
        for (auto it : registered_weak_ptrs) {
            it->invalidate_guard();
        }
    }

private:
    friend TWeakPtr;
    void register_ptr(TWeakPtr * weak_ptr) { registered_weak_ptrs.insert(weak_ptr); }
    void unregister_ptr(TWeakPtr * weak_ptr) noexcept { registered_weak_ptrs.erase(weak_ptr); }
    std::unordered_set<TWeakPtr *> registered_weak_ptrs;
};


template <typename TPtr, bool ptr_owner>
struct WeakPtr {
public:
    using TWeakPtrGuard = WeakPtrGuard<TPtr, ptr_owner>;

    WeakPtr() = delete;

    WeakPtr(TPtr * ptr, TWeakPtrGuard * guard) : ptr(ptr), guard(guard) {
        if (!ptr) {
            throw std::logic_error("WeakPtr: Creating null WeakPtr is not allowed");
        }
        if (!guard) {
            throw std::logic_error("WeakPtr: Creating WeakPtr without quard is not allowed");
        }
        guard->register_ptr(this);
    }

    // TODO: Want we to allow copying invalid WeakPtr?
    WeakPtr(const WeakPtr & src) : guard(src.guard) {
        if constexpr (ptr_owner) {
            ptr = src.ptr ? new TPtr(*src.ptr) : nullptr;
        } else {
            ptr = src.ptr;
        }
        if (is_valid()) {
            guard->register_ptr(this);
        }
    }

    // TODO: Want we to allow moving invalid WeakPtr?
    template <typename T = TPtr, typename std::enable_if<sizeof(T) && ptr_owner, int>::type = 0>
    WeakPtr(WeakPtr && src) : guard(src.guard) {
        if (src.is_valid()) {
            src.guard->unregister_ptr(src);
        }
        src.guard = nullptr;
        ptr = src.ptr;
        src.ptr = nullptr;
        if (is_valid()) {
            guard->register_ptr(this);
        }
    }

    ~WeakPtr() {
        if (is_valid()) {
            guard->unregister_ptr(this);
        }
        if constexpr (ptr_owner) {
            delete ptr;
        }
    }

    // TODO: Want we to allow copying invalid WeakPtr?
    template <typename T = TPtr, typename std::enable_if<sizeof(T) && ptr_owner, int>::type = 0>
    WeakPtr & operator=(const WeakPtr & src) {
        if (guard == src.guard) {
            delete ptr;
            ptr = src.ptr ? new TPtr(*src.ptr) : nullptr;
        } else {
            if (is_valid()) {
                guard->unregister_ptr(this);
            }
            guard = src.guard;
            delete ptr;
            ptr = src.ptr ? new TPtr(*src.ptr) : nullptr;
            if (is_valid()) {
                guard->register_ptr(this);
            }
        }
        return *this;
    }

    // TODO: Want we to allow copying invalid WeakPtr?
    template <typename T = TPtr, typename std::enable_if<sizeof(T) && !ptr_owner, int>::type = 0>
    WeakPtr & operator=(const WeakPtr & src) {
        if (guard == src.guard) {
            ptr = src.ptr;
        } else {
            if (is_valid()) {
                guard->unregister_ptr(this);
            }
            guard = src.guard;
            ptr = src.ptr;
            if (is_valid()) {
                guard->register_ptr(this);
            }
        }
        return *this;
    }

    // TODO: Want we to allow moving invalid WeakPtr?
    template <typename T = TPtr, typename std::enable_if<sizeof(T) && ptr_owner, int>::type = 0>
    WeakPtr & operator=(WeakPtr && src) {
        if (guard == src.guard) {
            if (src.is_valid()) {
                src.guard->unregister_ptr(src);
            }
            src.guard = nullptr;
            ptr = src.ptr;
            src.ptr = nullptr;
        } else {
            if (is_valid()) {
                guard->unregister_ptr(this);
            }
            if (src.is_valid()) {
                src.guard->unregister_ptr(src);
            }
            guard = src.guard;
            src.guard = nullptr;
            ptr = src.ptr;
            src.ptr = nullptr;
            if (is_valid()) {
                guard->register_ptr(this);
            }
        }
        return *this;
    }

    const TPtr * operator->() const {
        check();
        return ptr;
    }

    TPtr * operator->() {
        check();
        return ptr;
    }

    const TPtr * get() const {
        check();
        return ptr;
    }

    TPtr * get() {
        check();
        return ptr;
    }


    bool is_valid() const noexcept { return guard; }
    bool has_same_guard(const WeakPtr & other) const noexcept { return guard == other.guard; }

    bool operator==(const WeakPtr & other) const { return ptr == other.ptr; }
    bool operator!=(const WeakPtr & other) const { return ptr != other.ptr; }
    bool operator<(const WeakPtr & other) const { return ptr < other.ptr; }
    bool operator>(const WeakPtr & other) const { return ptr > other.ptr; }
    bool operator<=(const WeakPtr & other) const { return ptr <= other.ptr; }
    bool operator>=(const WeakPtr & other) const { return ptr >= other.ptr; }

private:
    friend TWeakPtrGuard;
    void invalidate_guard() noexcept { guard = nullptr; }
    void check() const {
        if (!is_valid()) {
            throw std::logic_error("WeakPtr: Invalid pointer");
        }
    }

    TPtr * ptr;
    TWeakPtrGuard * guard;
};

}  // namespace libdnf::utils

#endif
