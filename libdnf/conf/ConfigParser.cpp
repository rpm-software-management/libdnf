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

#include "ConfigParser.hpp"
#include "iniparser/iniparser.hpp"

#include <algorithm>
#include <fstream>

namespace libdnf {

void ConfigParser::substitute(std::string & text,
    const std::map<std::string, std::string> & substitutions)
{
    auto start = text.find_first_of("$");
    while (start != text.npos)
    {
        auto variable = start + 1;
        if (variable >= text.length())
            break;
        bool bracket;
        if (text[variable] == '{') {
            bracket = true;
            if (++variable >= text.length())
                break;
        } else
            bracket = false;
        auto it = std::find_if_not(text.begin()+variable, text.end(),
            [](char c){return std::isalnum(c) || c=='_';});
        if (bracket && it == text.end())
            break;
        auto pastVariable = std::distance(text.begin(), it);
        if (bracket && *it != '}') {
            start = text.find_first_of("$", pastVariable);
            continue;
        }
        auto subst = substitutions.find(text.substr(variable, pastVariable - variable));
        if (subst != substitutions.end()) {
            if (bracket)
                ++pastVariable;
            text.replace(start, pastVariable - start, subst->second);
            start = text.find_first_of("$", start + subst->second.length());
        } else {
            start = text.find_first_of("$", pastVariable);
        }
    }
}

void ConfigParser::read(const std::string & filePath)
{
    try {
        IniParser parser(filePath);
        IniParser::ItemType readedType;
        while ((readedType = parser.next()) != IniParser::ItemType::END_OF_INPUT) {
            auto section = parser.getSection();
            if (readedType == IniParser::ItemType::SECTION && data.find(section) == data.end())
                data[section] = {};
            else if (readedType == IniParser::ItemType::KEY_VAL)
                data[section][std::move(parser.getKey())] = std::move(parser.getValue());
        }
    } catch (const IniParser::CantOpenFile & e) {
        throw CantOpenFile(e.what());
    } catch (const IniParser::Exception & e) {
        throw ParsingError(e.what() + std::string(" at line ") + std::to_string(e.getLineNumber()));
    }
}

const std::string &
ConfigParser::getValue(const std::string & section, const std::string & key) const
{
    auto sect = data.find(section);
    if (sect == data.end())
        throw MissingSection("OptionReader::getValue(): Missing section " + section);
    auto keyVal = sect->second.find(key);
    if (keyVal == sect->second.end())
        throw MissingOption("OptionReader::getValue(): Missing option " + key +
            " in section " + section);
    return keyVal->second;
}

std::string
ConfigParser::getSubstitutedValue(const std::string & section, const std::string & key) const
{
    auto ret = getValue(section, key);
    substitute(ret, substitutions);
    return ret;
}

static void writeKeyVals(std::ostream & out, const std::map<std::string, std::string> & keyValMap)
{
    for (const auto & keyVal : keyValMap) {
        out << keyVal.first << "=";
        for (const auto chr : keyVal.second) {
            out << chr;
            if (chr == '\n')
                out << " ";
        }
        out << "\n";
    }
}

void ConfigParser::write(const std::string & filePath, bool append) const
{
    bool first{true};
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filePath, append ? std::ofstream::app : std::ofstream::trunc);
    for (const auto & section : data) {
        if (first)
            first = false;
        else
            ofs << "\n";
        ofs << "[" << section.first << "]" << "\n";
        writeKeyVals(ofs, section.second);
    }
}

void ConfigParser::write(const std::string & filePath, bool append, const std::string & section) const
{
    auto sit = data.find(section);
    if (sit == data.end())
        throw MissingSection("ConfigParser::write(): Missing section " + section);
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filePath, append ? std::ofstream::app : std::ofstream::trunc);
    ofs << "[" << section << "]" << "\n";
    writeKeyVals(ofs, sit->second);
}

}
