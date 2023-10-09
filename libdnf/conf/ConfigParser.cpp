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
#include "../utils/iniparser/iniparser.hpp"

#include <algorithm>
#include <fstream>

namespace libdnf {

void ConfigParser::substitute(std::string & text,
    const std::map<std::string, std::string> & substitutions)
{
    text = ConfigParser::substitute_expression(text, substitutions, 0).first;
}

const unsigned int MAXIMUM_EXPRESSION_DEPTH = 32;

std::pair<std::string, size_t> ConfigParser::substitute_expression(const std::string & text,
    const std::map<std::string, std::string> & substitutions,
    unsigned int depth) {
    if (depth > MAXIMUM_EXPRESSION_DEPTH) {
        return std::make_pair(std::string(text), text.length());
    }
    std::string res{text};

    // The total number of characters read in the replacee
    size_t total_scanned = 0;

    size_t pos = 0;
    while (pos < res.length()) {
        if (res[pos] == '}' && depth > 0) {
            return std::make_pair(res.substr(0, pos), total_scanned);
        }

        if (res[pos] == '\\') {
            // Escape the next character (if there is one)
            if (pos + 1 >= res.length()) {
                break;
            }
            res.erase(pos, 1);
            total_scanned += 2;
            pos += 1;
            continue;
        }
        if (res[pos] == '$') {
            // variable expression starts after the $ and includes the braces
            //     ${variable:-word}
            //      ^-- pos_variable_expression
            size_t pos_variable_expression = pos + 1;
            if (pos_variable_expression >= res.length()) {
                break;
            }

            // Does the variable expression use braces? If so, the variable name
            // starts one character after the start of the variable_expression
            bool has_braces;
            size_t pos_variable;
            if (res[pos_variable_expression] == '{') {
                has_braces = true;
                pos_variable = pos_variable_expression + 1;
                if (pos_variable >= res.length()) {
                    break;
                }
            } else {
                has_braces = false;
                pos_variable = pos_variable_expression;
            }

            // Find the end of the variable name
            auto it = std::find_if_not(res.begin() + static_cast<long>(pos_variable), res.end(), [](char c) {
                return std::isalnum(c) != 0 || c == '_';
            });
            auto pos_after_variable = static_cast<size_t>(std::distance(res.begin(), it));

            // Find the substituting string and the end of the variable expression
            const auto & variable_key = res.substr(pos_variable, pos_after_variable - pos_variable);
            const auto variable_mapping = substitutions.find(variable_key);

            const std::string * variable_value = nullptr;

            if (variable_mapping == substitutions.end()) {
                if (variable_key == "releasever_major" || variable_key == "releasever_minor") {
                    const auto releasever_mapping = substitutions.find("releasever");
                    if (releasever_mapping != substitutions.end()) {
                        const auto & releasever_split = ConfigParser::split_releasever(releasever_mapping->second);
                        if (variable_key == "releasever_major") {
                            variable_value = &std::get<0>(releasever_split);
                        } else {
                            variable_value = &std::get<1>(releasever_split);
                        }
                    }
                }
            } else {
                variable_value = &variable_mapping->second;
            }

            const std::string * subst_str = nullptr;

            size_t pos_after_variable_expression;

            if (has_braces) {
                if (pos_after_variable >= res.length()) {
                    break;
                }
                if (res[pos_after_variable] == ':') {
                    if (pos_after_variable + 1 >= res.length()) {
                        break;
                    }
                    char expansion_mode = res[pos_after_variable + 1];
                    size_t pos_word = pos_after_variable + 2;
                    if (pos_word >= res.length()) {
                        break;
                    }

                    // Expand the default/alternate expression
                    auto word_str = res.substr(pos_word);
                    auto word_substitution = substitute_expression(word_str, substitutions, depth + 1);
                    auto expanded_word = word_substitution.first;
                    auto scanned = word_substitution.second;
                    auto pos_after_word = pos_word + scanned;
                    if (pos_after_word >= res.length()) {
                        break;
                    }
                    if (res[pos_after_word] != '}') {
                        // The variable expression doesn't end in a '}',
                        // continue after the word and don't expand it
                        total_scanned += pos_after_word - pos;
                        pos = pos_after_word;
                        continue;
                    }

                    if (expansion_mode == '-') {
                        // ${variable:-word} (default value)
                        // If variable is unset or empty, the expansion of word is
                        // substituted. Otherwise, the value of variable is
                        // substituted.
                        if (variable_value == nullptr || variable_value->empty()) {
                            subst_str = &expanded_word;
                        } else {
                            subst_str = variable_value;
                        }
                    } else if (expansion_mode == '+') {
                        // ${variable:+word} (alternate value)
                        // If variable is unset or empty nothing is substituted.
                        // Otherwise, the expansion of word is substituted.
                        if (variable_value == nullptr || variable_value->empty()) {
                            const std::string empty{};
                            subst_str = &empty;
                        } else {
                            subst_str = &expanded_word;
                        }
                    } else {
                        // Unknown expansion mode, continue after the ':'
                        pos = pos_after_variable + 1;
                        continue;
                    }
                    pos_after_variable_expression = pos_after_word + 1;
                } else if (res[pos_after_variable] == '}') {
                    // ${variable}
                    subst_str = variable_value;
                    // Move past the closing '}'
                    pos_after_variable_expression = pos_after_variable + 1;
                } else {
                    // Variable expression doesn't end in a '}', continue after the variable
                    pos = pos_after_variable;
                    continue;
                }
            } else {
                // No braces, we have a $variable
                subst_str = variable_value;
                pos_after_variable_expression = pos_after_variable;
            }

            // If there is no substitution to make, move past the variable expression and continue.
            if (subst_str == nullptr) {
                total_scanned += pos_after_variable_expression - pos;
                pos = pos_after_variable_expression;
                continue;
            }

            res.replace(pos, pos_after_variable_expression - pos, *subst_str);
            total_scanned += pos_after_variable_expression - pos;
            pos += subst_str->length();
        } else {
            total_scanned += 1;
            pos += 1;
        }
    }

    // We have reached the end of the text
    if (depth > 0) {
        // If we are in a subexpression and we didn't find a closing '}', make no substitutions.
        return std::make_pair(std::string{text}, text.length());
    }

    return std::make_pair(res, text.length());
}

std::tuple<std::string, std::string> ConfigParser::split_releasever(const std::string & releasever)
{
    // Uses the same logic as DNF 5 and as splitReleaseverTo in libzypp
    std::string releasever_major;
    std::string releasever_minor;
    const auto pos = releasever.find('.');
    if (pos == std::string::npos) {
        releasever_major = releasever;
    } else {
        releasever_major = releasever.substr(0, pos);
        releasever_minor = releasever.substr(pos + 1);
    }
    return std::make_tuple(releasever_major, releasever_minor);
}

static void read(ConfigParser & cfgParser, IniParser & parser)
{
    IniParser::ItemType readedType;
    while ((readedType = parser.next()) != IniParser::ItemType::END_OF_INPUT) {
        auto section = parser.getSection();
        if (readedType == IniParser::ItemType::SECTION) {
            cfgParser.addSection(std::move(section), std::move(parser.getRawItem()));
        }
        else if (readedType == IniParser::ItemType::KEY_VAL) {
            cfgParser.setValue(section, std::move(parser.getKey()), std::move(parser.getValue()), std::move(parser.getRawItem()));
        }
        else if (readedType == IniParser::ItemType::COMMENT_LINE || readedType == IniParser::ItemType::EMPTY_LINE) {
            if (section.empty())
                cfgParser.getHeader() += parser.getRawItem();
            else
                cfgParser.addCommentLine(section, std::move(parser.getRawItem()));
        }
    }
}

void ConfigParser::read(const std::string & filePath)
{
    try {
        IniParser parser(filePath);
        ::libdnf::read(*this, parser);
    } catch (const IniParser::CantOpenFile & e) {
        throw CantOpenFile(e.what());
    } catch (const IniParser::Exception & e) {
        throw ParsingError(e.what() + std::string(" at line ") + std::to_string(e.getLineNumber()));
    }
}

void ConfigParser::read(std::unique_ptr<std::istream> && inputStream)
{
    try {
        IniParser parser(std::move(inputStream));
        ::libdnf::read(*this, parser);
    } catch (const IniParser::CantOpenFile & e) {
        throw CantOpenFile(e.what());
    } catch (const IniParser::Exception & e) {
        throw ParsingError(e.what() + std::string(" at line ") + std::to_string(e.getLineNumber()));
    }
}

static std::string createRawItem(const std::string & value, const std::string & oldRawItem)
{
    auto eqlPos = oldRawItem.find('=');
    if (eqlPos == oldRawItem.npos)
        return "";
    auto valuepos = oldRawItem.find_first_not_of(" \t", eqlPos + 1);
    auto keyAndDelimLength = valuepos != oldRawItem.npos ? valuepos : oldRawItem.length();
    return oldRawItem.substr(0, keyAndDelimLength) + value + '\n';
}

void ConfigParser::setValue(const std::string & section, const std::string & key, const std::string & value)
{
    auto rawIter = rawItems.find(section + ']' + key);
    auto raw = createRawItem(value, rawIter != rawItems.end() ? rawIter->second : "");
    setValue(section, key, value, raw);
}

void ConfigParser::setValue(const std::string & section, std::string && key, std::string && value)
{
    auto rawIter = rawItems.find(section + ']' + key);
    auto raw = createRawItem(value, rawIter != rawItems.end() ? rawIter->second : "");
    setValue(section, std::move(key), std::move(value), std::move(raw));
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

static void writeKeyVals(std::ostream & out, const std::string & section, const ConfigParser::Container::mapped_type & keyValMap, const std::map<std::string, std::string> & rawItems)
{
    for (const auto & keyVal : keyValMap) {
        auto first = keyVal.first[0];
        if (first == '#' || first == ';')
            out << keyVal.second;
        else {
            auto rawItem = rawItems.find(section + ']' + keyVal.first);
            if (rawItem != rawItems.end())
                out << rawItem->second;
            else {
                out << keyVal.first << "=";
                for (const auto chr : keyVal.second) {
                    out << chr;
                    if (chr == '\n')
                        out << " ";
                }
                out << "\n";
            }
        }
    }
}

static void writeSection(std::ostream & out, const std::string & section, const ConfigParser::Container::mapped_type & keyValMap, const std::map<std::string, std::string> & rawItems)
{
    auto rawItem = rawItems.find(section);
    if (rawItem != rawItems.end())
        out << rawItem->second;
    else
        out << "[" << section << "]" << "\n";
    writeKeyVals(out, section, keyValMap, rawItems);
}

void ConfigParser::write(const std::string & filePath, bool append) const
{
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filePath, append ? std::ofstream::app : std::ofstream::trunc);
    write(ofs);
}

void ConfigParser::write(const std::string & filePath, bool append, const std::string & section) const
{
    auto sit = data.find(section);
    if (sit == data.end())
        throw MissingSection("ConfigParser::write(): Missing section " + section);
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filePath, append ? std::ofstream::app : std::ofstream::trunc);
    writeSection(ofs, sit->first, sit->second, rawItems);
}

void ConfigParser::write(std::ostream & outputStream) const
{
    outputStream << header;
    for (const auto & section : data) {
        writeSection(outputStream, section.first, section.second, rawItems);
    }
}

void ConfigParser::write(std::ostream & outputStream, const std::string & section) const
{
    auto sit = data.find(section);
    if (sit == data.end())
        throw MissingSection("ConfigParser::write(): Missing section " + section);
    writeSection(outputStream, sit->first, sit->second, rawItems);
}

}
