C++ coding style
================

* C++17
* Indent by 4 spaces
* Up to 120 characters per line
* Comments start with two forward slashes: ``//``
* Docstrings start with three forward slashes: ``///``
* See .clang-format for more details and examples


Character case
--------------

* Types: CamelCase
* Classes: CamelCase
* Functions: snake_case
* Variables: snake_case
* Arguments: snake_case
* Constants: UPPER_CASE


Includes
--------
* Includes grouped and alphabetically ordered within each group::

    #include "current-dir-include.hpp"

    #include <libdnf-cli/.../*.hpp>

    #include <libdnf/.../*.hpp>

    #include <3rd party>

    #include <standard library>

* Includes within the same directory should use relative paths::

     #include "current-dir-include.hpp"

* Other includes should use absolute paths::

    #include <libdnf/.../*.hpp>

Line breaks
-----------
clang-format doesn't enforce number of line breaks between top level definitions, hence the style of these is on a best-effort basis. Please try to adhere to the following conventions:

* Single line break between top-level copyright comment, include guards and includes
* Double line break before and between namespace blocks.
* Single line break inside a namespace block at the start and end
* Double line break between unrelated or loosely related definitions, single or no line break between related definitions
* Single line break between a namespace block and an include guard #endif
* Function/method definitions should be separated by a double line break

Example::

    // SPDX-License-Identifier: GPL-2.0-or-later
    // SPDX-FileCopyrightText: Copyright contributors to the libdnf project

    #ifndef LIBDNF_RPM_PACKAGE_SACK_HPP
    #define LIBDNF_RPM_PACKAGE_SACK_HPP

    #include "package.hpp"
    #include "package_set.hpp"

    #include "libdnf/base/base_weak.hpp"

    #include <map>
    #include <string>


    namespace libdnf {

    class Goal;

    }  // namespace libdnf


    namespace libdnf::repo {

    class Repo;

    }  // namespace libdnf::repo


    namespace libdnf::rpm {

    class PackageSack;
    using PackageSackWeakPtr = WeakPtr<PackageSack, false>;

    class PackageSack {
    // ...
    };


    PackageSack::PackageSack() {
        // ...
    }


    void PackageSack::load_repo(...) {
        // ...
    }

    }  // namespace libdnf ::rpm

    #endif  // LIBDNF_RPM_PACKAGE_SACK_HPP
