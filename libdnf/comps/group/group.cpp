#include <libdnf/comps/group/group.hpp>
#include <libdnf/utils/xml.hpp>
#include <libxml/tree.h>

#include <cstring>
#include <iostream>


namespace libdnf::comps {


bool Group::get_installed() const {
    return repos.find("@System") != repos.end();
}


Group & Group::operator+=(const Group & rhs) {
    if (!rhs.id.empty()) {
        this->id = rhs.id;
    }
    if (!rhs.name.empty()) {
        this->name = rhs.name;
    }
    return *this;
}


void load_group_from_xml(Group & grp, xmlNode * a_node) {
    for (auto node = a_node->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE) {
            continue;
        }

        if (strcmp(node->name, "id") == 0) {
            // <id>
            grp.set_id(libdnf::utils::xml::get_content(node));
        } else if (strcmp(node->name, "name") == 0) {
            // <name>, <name lang="...">
            auto lang = libdnf::utils::xml::get_attribute(node, "lang");
            auto value = libdnf::utils::xml::get_content(node);
            if (lang.first) {
                // translated name
                grp.set_translated_name(lang.second, value);
            } else {
                // name
                grp.set_name(value);
            }
            //libdnf::utils::xml::free(lang);
        } else if (strcmp(node->name, "description") == 0) {
            // <description>, <description lang="...">
            auto lang = libdnf::utils::xml::get_attribute(node, "lang");
            auto value = libdnf::utils::xml::get_content(node);
            if (lang.first) {
                // translated name
                grp.set_translated_description(lang.second, value);
            } else {
                // name
                grp.set_description(value);
            }
            //libdnf::utils::xml::free(lang);
        } else if (strcmp(node->name, "default") == 0) {
            // <default>true|false</default>
            grp.set_default(libdnf::utils::xml::get_bool(libdnf::utils::xml::get_content(node)));
        } else if (strcmp(node->name, "uservisible") == 0) {
            // <uservisible>true|false</uservisible>
            grp.set_uservisible(libdnf::utils::xml::get_bool(libdnf::utils::xml::get_content(node)));
        } else if (strcmp(node->name, "packagelist") == 0) {
            // <packagelist>
            // <packagereq type="default">pkg</packagereq>
            // <packagereq type="conditional" requires="if-pkg">pkg</packagereq>
        } else {
            // else throw an error?
            throw std::runtime_error((const char *)node->name);
        }
    }
    //std::cout << grp->get_id() << std::endl;
}


}  // namespace libdnf::comps
