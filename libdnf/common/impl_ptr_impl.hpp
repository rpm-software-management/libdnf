/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LIBDNF_COMMON_IMPL_PTR_IMPL_HPP
#define LIBDNF_COMMON_IMPL_PTR_IMPL_HPP

#include "libdnf/common/impl_ptr.hpp"


namespace libdnf {

template <class T>
ImplPtr<T>::ImplPtr(const ImplPtr & src) : ptr(src.ptr ? new T(*src.ptr) : nullptr) {}

template <class T>
ImplPtr<T> & ImplPtr<T>::operator=(const ImplPtr & src) {
    if (ptr != src.ptr) {
        if (ptr) {
            *ptr = *src.ptr;
        } else {
            ptr = new T(*src.ptr);
        }
    }
    return *this;
}

template <class T>
ImplPtr<T> & ImplPtr<T>::operator=(ImplPtr && src) noexcept {
    if (ptr != src.ptr) {
        delete ptr;
        ptr = src.ptr;
        src.ptr = nullptr;
    }
    return *this;
}

template <class T>
ImplPtr<T>::~ImplPtr() {
    delete ptr;
}

}  // namespace libdnf

#endif
