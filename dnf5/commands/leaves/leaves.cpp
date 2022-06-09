/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "leaves.hpp"

#include <libdnf/rpm/package.hpp>
#include <libdnf/rpm/package_query.hpp>
#include <libdnf/rpm/package_set.hpp>

#include <iostream>

namespace dnf5 {

using namespace libdnf::cli;

void LeavesCommand::set_argument_parser() {
    get_argument_parser_command()->set_short_description(
        "List installed packages not required by other installed packages");
}

void LeavesCommand::configure() {
    auto & context = get_context();
    context.set_load_system_repo(true);
    context.set_load_available_repos(Context::LoadAvailableRepos::NONE);
}

namespace {

void add_edges(
    dnf5::Context & context,
    std::set<unsigned int> & edges,
    const libdnf::rpm::PackageQuery & full_query,
    const std::map<libdnf::rpm::PackageId, unsigned int> & pkg_id2idx,
    const libdnf::rpm::ReldepList & deps) {
    // resolve dependencies and add an edge if there is exactly one package satisfying it
    for (int i = 0; i < deps.size(); ++i) {
        libdnf::rpm::ReldepList req_in_list(context.base);
        req_in_list.add(deps.get_id(i));
        libdnf::rpm::PackageQuery query(full_query);
        query.filter_provides(req_in_list);
        if (query.size() == 1) {
            auto pkg = *query.begin();
            edges.insert(pkg_id2idx.at(pkg.get_id()));
        }
    }
}

std::vector<std::vector<unsigned int>> build_graph(
    dnf5::Context & context,
    const libdnf::rpm::PackageQuery & full_query,
    const std::vector<libdnf::rpm::Package> & pkgs,
    bool use_recommends) {
    // create pkg_id2idx to map Packages to their index in pkgs
    std::map<libdnf::rpm::PackageId, unsigned int> pkg_id2idx;
    for (size_t i = 0; i < pkgs.size(); ++i) {
        pkg_id2idx.emplace(pkgs[i].get_id(), i);
    }

    std::vector<std::vector<unsigned int>> graph;
    graph.reserve(pkgs.size());

    for (unsigned int i = 0; i < pkgs.size(); ++i) {
        std::set<unsigned int> edges;
        const auto & package = pkgs[i];
        add_edges(context, edges, full_query, pkg_id2idx, package.get_requires());
        if (use_recommends) {
            add_edges(context, edges, full_query, pkg_id2idx, package.get_recommends());
        }
        edges.erase(i);  // remove self-edges
        graph.emplace_back(edges.begin(), edges.end());
    }

    return graph;
}

std::vector<std::vector<unsigned int>> reverse_graph(const std::vector<std::vector<unsigned int>> & graph) {
    std::vector<std::vector<unsigned int>> rgraph(graph.size());

    // pre-allocate (reserve) memory to prevent reallocations
    std::vector<unsigned int> lens(graph.size());
    for (const auto & edges : graph) {
        for (auto edge : edges) {
            ++lens[edge];
        }
    }
    for (unsigned int i = 0; i < graph.size(); ++i) {
        rgraph[i].reserve(lens[i]);
    }

    // reverse graph
    for (unsigned int i = 0; i < graph.size(); ++i) {
        for (auto edge : graph[i]) {
            rgraph[edge].push_back(i);
        }
    }

    return rgraph;
}

std::vector<std::vector<unsigned int>> kosaraju(const std::vector<std::vector<unsigned int>> & graph) {
    const auto N = static_cast<unsigned int>(graph.size());
    std::vector<unsigned int> rstack(N);
    std::vector<unsigned int> stack(N);
    std::vector<bool> tag(N, false);
    unsigned int r = N;
    unsigned int top = 0;

    // do depth-first searches in the graph and push nodes to rstack
    // "on the way up" until all nodes have been pushed.
    // tag nodes as they're processed so we don't visit them more than once
    for (unsigned int i = 0; i < N; ++i) {
        if (tag[i]) {
            continue;
        }

        unsigned int u = i;
        unsigned int j = 0;
        tag[u] = true;
        while (true) {
            const auto & edges = graph[u];
            if (j < edges.size()) {
                const auto v = edges[j++];
                if (!tag[v]) {
                    rstack[top] = j;
                    stack[top++] = u;
                    u = v;
                    j = 0;
                    tag[u] = true;
                }
            } else {
                rstack[--r] = u;
                if (top == 0) {
                    break;
                }
                u = stack[--top];
                j = rstack[top];
            }
        }
    }

    if (r != 0) {
        throw std::logic_error("leaves: kosaraju(): r != 0");
    }

    // now searches beginning at nodes popped from rstack in the graph with all
    // edges reversed will give us the strongly connected components.
    // this time all nodes are tagged, so let's remove the tags as we visit each
    // node.
    // the incoming edges to each component is the union of incoming edges to
    // each node in the component minus the incoming edges from component nodes
    // themselves.
    // if there are no such incoming edges the component is a leaf and we
    // add it to the array of leaves.
    auto rgraph = reverse_graph(graph);
    std::set<unsigned int> sccredges;
    std::vector<std::vector<unsigned int>> leaves;
    for (; r < N; ++r) {
        unsigned int u = rstack[r];
        if (!tag[u]) {
            continue;
        }

        stack[top++] = u;
        tag[u] = false;
        unsigned int s = N;
        while (top) {
            u = stack[--s] = stack[--top];
            const auto & redges = rgraph[u];
            for (unsigned int j = 0; j < redges.size(); ++j) {
                const unsigned int v = redges[j];
                sccredges.insert(v);
                if (!tag[v]) {
                    continue;
                }

                stack[top++] = v;
                tag[v] = false;
            }
        }

        for (unsigned int i = s; i < N; ++i) {
            sccredges.erase(stack[i]);
        }

        if (sccredges.empty()) {
            std::vector scc(stack.begin() + s, stack.end());
            std::sort(scc.begin(), scc.end());
            leaves.emplace_back(std::move(scc));
        } else {
            sccredges.clear();
        }
    }

    return leaves;
}

}  // namespace

void LeavesCommand::run() {
    auto & ctx = get_context();

    // get a sorted array of all installed packages
    const libdnf::rpm::PackageQuery query(ctx.base);
    std::vector<libdnf::rpm::Package> pkgs(query.begin(), query.end());
    std::sort(pkgs.begin(), pkgs.end());

    // build the directed graph of dependencies
    bool use_recommends = ctx.base.get_config().install_weak_deps().get_value();
    auto graph = build_graph(ctx, query, pkgs, use_recommends);

    // run Kosaraju's algorithm to find strongly connected components
    // without any incoming edges
    auto leaves = kosaraju(graph);
    std::sort(
        leaves.begin(), leaves.end(), [](const std::vector<unsigned int> & a, const std::vector<unsigned int> & b) {
            return a[0] < b[0];
        });

    // print the packages grouped by their components
    for (const auto & scc : leaves) {
        char mark = '-';

        for (unsigned int j = 0; j < scc.size(); ++j) {
            const auto & package = pkgs[scc[j]];
            std::cout << mark << ' ' << package.get_full_nevra() << '\n';
            mark = ' ';
        }
    }
}

}  // namespace dnf5
