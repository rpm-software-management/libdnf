#ifndef LIBDNF_UTILS_XML_HPP
#define LIBDNF_UTILS_XML_HPP


#include <libxml/tree.h>
#include <libxml/xmlstring.h>

#include <cstring>
#include <stdexcept>


/// Extend strcmp() to handle 'const xmlChar *' type.
inline int strcmp(const xmlChar * lhs, const char * rhs) {
    return strcmp((const char *)lhs, rhs);
}


namespace libdnf::utils::xml {


/// Get content of an xml tag: <tag>content</tag>
inline const char * get_content(xmlNode * a_node) {
    for (auto node = a_node->children; node != nullptr; node = node->next) {
        if (xmlNodeIsText(node)) {
            return (const char *)node->content;
        }
    }
    return "";
}


/// Convert 'true' or 'false' to a boolean value.
inline bool get_bool(const char * value) {
    if (strcmp(value, "true") == 0) {
        return true;
    } else if (strcmp(value, "false") == 0) {
        return false;
    } else {
        throw std::runtime_error("bool");
    }
}


/// Get an XML attribute of given name.
/// Return a pair: <bool exists, std::string value>
inline std::pair<bool, std::string> get_attribute(xmlNode * a_node, const char * name) {
    auto value = xmlGetProp(a_node, (xmlChar *)name);
    if (value == nullptr) {
        return {false, ""};
    }
    std::pair<bool, std::string> result = {true, (const char *)value};
    xmlFree(value);
    return result;
}


}  // namespace libdnf::utils::xml


#endif  // LIBDNF_UTILS_XML_HPP
