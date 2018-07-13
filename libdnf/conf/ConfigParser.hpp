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

#ifndef LIBDNF_CONFIG_PARSER_HPP
#define LIBDNF_CONFIG_PARSER_HPP

#include <map>
#include <stdexcept>
#include <utility>

namespace libdnf {

/**
* @class ConfigParser
* 
* @brief Class for parsing dnf/yum .ini configuration files.
*
* IniParser is used for parsing file. The class adds support for substitutions.
* The parsed items are stored into the std::map.
* User can get both substituded and original parsed value.
*/
struct ConfigParser {
public:
    struct Exception : public std::runtime_error {
        Exception(const std::string & what) : runtime_error(what) {}
    };
    struct CantOpenFile : public Exception {
        CantOpenFile(const std::string & what) : Exception(what) {}
    };
    struct ParsingError : public Exception {
        ParsingError(const std::string & what) : Exception(what) {}
    };
    struct MissingSection : public Exception {
        MissingSection(const std::string & what) : Exception(what) {}
    };
    struct MissingOption : public Exception {
        MissingOption(const std::string & what) : Exception(what) {}
    };

    /**
    * @brief Substitute values in text according to the substitutions map
    *
    * @param text The text for substitution
    * @param substitutions Substitution map
    */
    static void substitute(std::string & text,
        const std::map<std::string, std::string> & substitutions);
    void setSubstitutions(const std::map<std::string, std::string> & substitutions);
    void setSubstitutions(std::map<std::string, std::string> && substitutions);
    const std::map<std::string, std::string> & getSubstitutions();
    /**
    * @brief Reads/parse one INI file
    *
    * Can be called repeately for reading/merge more INI files. 
    *
    * @param filePath Name (with path) of file to read
    */
    void read(const std::string & filePath);
    /**
    * @brief Writes all data (all sections) to INI file
    *
    * @param filePath Name (with path) of file to write
    * @param append If true, existent file will be appended, otherwise overwritten
    */
    void write(const std::string & filePath, bool append) const;
    /**
    * @brief Writes one section data to INI file
    *
    * @param filePath Name (with path) of file to write
    * @param append If true, existent file will be appended, otherwise overwritten
    * @param section Section to write
    */
    void write(const std::string & filePath, bool append, const std::string & section) const;
    bool hasSection(const std::string & section) const;
    const std::string & getValue(const std::string & section, const std::string & key) const;
    std::string getSubstitutedValue(const std::string & section, const std::string & key) const;
    const std::map<std::string, std::map<std::string, std::string>> & getData() const noexcept;

private:
    std::map<std::string, std::string> substitutions;
    std::map<std::string, std::map<std::string, std::string>> data;
};

inline void ConfigParser::setSubstitutions(const std::map<std::string, std::string> & substitutions)
{
    this->substitutions = substitutions;
}

inline void ConfigParser::setSubstitutions(std::map<std::string, std::string> && substitutions)
{
    this->substitutions = std::move(substitutions);
}

inline const std::map<std::string, std::string> & ConfigParser::getSubstitutions()
{
    return substitutions;
}

inline bool ConfigParser::hasSection(const std::string & section) const
{
    return data.find(section) != data.end();
}

inline const std::map<std::string,
    std::map<std::string, std::string>> & ConfigParser::getData() const noexcept
{
    return data;
}

}

#endif
