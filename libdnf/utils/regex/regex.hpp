/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _REGEX_HPP
#define _REGEX_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include <regex.h>

/**
* @brief Wrapper of POSIX C regular expression implementation "regex.h".
* Regex class uses POSIX functions regcomp(), regexec(), regerror(), regfree()
* and structures regex_t, regmatch_t internally.
*/
class Regex {
public:
    class Exception : public std::runtime_error {
    public:
        Exception(const std::string & msg) : runtime_error(msg) {}
        Exception(const char * msg) : runtime_error(msg) {}
    };

    /**
    * @brief Delivers error code from regcomp() and coresponding error message string
    */
    class LibraryException : public Exception {
    public:
        LibraryException(int code, const std::string & msg) : Exception(msg), ecode(code) {}
        LibraryException(int code, const char * msg) : Exception(msg), ecode(code) {}
        int code() const noexcept { return ecode; }
    protected:
        int ecode;
    };

    class InvalidException : public Exception {
    public:
        InvalidException()
            : Exception("Regex object unusable. Its value was moved to another Regex object.") {}
    };

    /**
    * @brief Result of Regex::match() method
    * Contains copy or pointer to matched string and information
    * regarding the location of any matched substrings.
    */
    class Result {
    public:
        Result(const Result & src);
        Result(Result && src);
        static constexpr int NOT_FOUND = -1;

        /**
        * @brief Returns true if contains copy of matched string
        * @return bool
        */
        bool hasSourceCopy() const noexcept;

        /**
        * @brief Returns true if string is matching
        * @return bool
        */
        bool isMatched() const noexcept;

        /**
        * @brief Returns number of matched substrings in Result
        * @return std::size_t
        */
        std::size_t getMatchedCount() const noexcept;

        /**
        * @brief Returns start offset of the matched substring
        * @param index index of matched substring
        * @return byte offset from start of string to start of substring or Result::NOT_FOUND
        */
        int getMatchStartOffset(std::size_t index) const noexcept;

        /**
        * @brief Returns end offset of the matched substring
        * @param index index of matched substring
        * @return byte offset from start of string of the first character after the end of substring
        *         or Result::NOT_FOUND
        */
        int getMatchEndOffset(std::size_t index) const noexcept;

        /**
        * @brief Returns length of the matched substring
        * @param index index of matched substring
        * @return length of matched substring or Result::NOT_FOUND
        */
        int getMatchedLen(std::size_t index) const noexcept;

        /**
        * @brief Returns the matched substring
        * @param index Index of matched substring
        * @return matched substring or ""
        */
        std::string getMatchedString(std::size_t index) const;
        ~Result();
    private:
        friend class Regex;
        Result(const char * source, bool copySource, std::size_t count);
        const char * source;
        bool sourceOwner;
        bool matched;
        std::vector<regmatch_t> matches;
    };

    /**
    * @brief Constructs new Regex object
    * Compiles input regular expression string using POSIX regcomp() function.
    * Input parameters regex and flags corespond to regcomp() parameters.
    *
    * @param regex regular expression
    * @param flags compile flags
    */
    Regex(const char * regex, int flags);
    Regex(const Regex & src) = delete;
    Regex(Regex && src) noexcept;

    Regex & operator=(const Regex & src) = delete;
    Regex & operator=(Regex && src) noexcept;

    /**
    * @brief Returns number of parenthesized subexpressions
    * Coresponds to re_nsub in POSIX regex_t structure.
    *
    * @return number of parenthesized subexpressions
    */
    std::size_t getSubexprCount() const;

    /**
    * @brief Match input string to precompiled regular expression
    * POSIX regexec() function is used.
    *
    * @param str string to match
    * @return true if string is matched
    */
    bool match(const char * str) const;

    /**
    * @brief Match input string to precompiled regular expression
    * POSIX regexec() function is used.
    *
    * @param str string to match
    * @param copyStr if is true then copy of str will be done into the Result,
    *                copy only pointer otherwise
    * @param count Max number of matches we want to get
    * @return object providing information regarding the location of any matches
    */
    Result match(const char * str, bool copyStr, std::size_t count) const;
    ~Regex();
private:
    void free() noexcept;
    bool freed{false};
    regex_t exp;
};

inline Regex::Regex(Regex && src) noexcept
: freed(src.freed), exp(src.exp)
{
    src.freed = true;
}

inline std::size_t Regex::getSubexprCount() const
{
    if (freed)
        throw InvalidException();
    return exp.re_nsub; 
}

inline bool Regex::match(const char * str) const
{
    if (freed)
        throw InvalidException();
    return regexec(&exp, str, 0, nullptr, 0) == 0;
}

inline Regex::~Regex()
{
    free();
}

inline void Regex::free() noexcept
{
    if (!freed)
        regfree(&exp);
}

inline Regex::Result::~Result()
{
    if (sourceOwner)
        delete[] source;
}

inline bool Regex::Result::hasSourceCopy() const noexcept
{
    return sourceOwner;
}

inline bool Regex::Result::isMatched() const noexcept
{
    return matched;
}

inline std::size_t Regex::Result::getMatchedCount() const noexcept
{
    return matches.size();
}

inline int Regex::Result::getMatchStartOffset(std::size_t index) const noexcept
{
    return matched && index < matches.size() ? matches[index].rm_so : NOT_FOUND;
}

inline int Regex::Result::getMatchEndOffset(std::size_t index) const noexcept
{
    return matched && index < matches.size() ? matches[index].rm_eo : NOT_FOUND;
}

inline int Regex::Result::getMatchedLen(std::size_t index) const noexcept
{
    return matched && index < matches.size() && matches[index].rm_so != NOT_FOUND ?
           matches[index].rm_eo - matches[index].rm_so : NOT_FOUND;
}

#endif
