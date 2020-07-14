#include <libdnf/comps/comps.hpp>
#include <libdnf/comps/group/group-private.hpp>
#include <libdnf/utils/xml.hpp>

#include <cstring>


namespace libdnf::comps {


void Comps::load_installed() {
    auto core = get_group_sack().new_group();
    core->set_id("core");
    core->set_name("Core");
    core->add_repo("@System");
    // TODO(dmach): load from transaction database
}


void Comps::load_from_file(const std::string & path) {
    // xml parser context
    xmlParserCtxtPtr xml_ctx = xmlNewParserCtxt();

    if (xml_ctx == nullptr) {
        throw std::bad_alloc();
        // "Failed to allocate xml parser context"
    }

    // resulting xml document tree
    // parse the file, activating the DTD validation option
    // xml_doc = xmlCtxtReadFile(xml_ctx, path.c_str(), nullptr, XML_PARSE_DTDVALID);
    xmlDocPtr xml_doc = xmlCtxtReadFile(xml_ctx, path.c_str(), nullptr, 0);

    xmlNode * root_node = xmlDocGetRootElement(xml_doc);

    // TODO(dmach): assert name == comps

    xmlNode * cur_node = NULL;
    for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
        if (cur_node->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (strcmp(cur_node->name, "group") == 0) {
            // create a Group object and populate it with data from xml
            auto group = std::make_unique<Group>();
            load_group_from_xml(*group, cur_node);

            // query sack for available (not installed) groups with given id
            auto q = get_group_sack().new_query();
            q.ifilter_installed(false);
            q.ifilter_id(libdnf::sack::QueryCmp::EQ, group->get_id());

            if (q.empty()) {
                // move the newly created group to the sack
                get_group_sack().add_group(std::move(group));
            } else {
                // update an existing group
                auto existing_group = q.get();
                *existing_group.get() += *group;
            }
        }
    }

    xmlFreeDoc(xml_doc);
    xmlFreeParserCtxt(xml_ctx);
}


}  // namespace libdnf::comps
