#include "Cache.hpp"
//include "sha1.hpp"
#include "sha1.cpp"

int main() {
    libdnf::Cache cache("cache");

    std::string url = "https://mirrors.fedoraproject.org/metalink?repo=updates-released-f31&arch=x86_64";
    SHA1Hash h;
    h.update(url.c_str());
    cache.add("metalink", h.hexdigest(), "metalink.xml");

    std::string repomd_sha1 = "35956520fefbc8618d05ee7646c99e83d40430b2";
    std::string repomd_path = "dl.fedoraproject.org/pub/fedora/linux/updates/31/Everything/x86_64/repodata/repomd.xml";
    cache.add("repomd", repomd_sha1, repomd_path);

    std::string solv_path = "/var/cache/dnf/updates.solv";
    cache.add("solv", repomd_sha1, solv_path);

    std::string solv_filenames_path = "/var/cache/dnf/updates-filenames.solvx";
    cache.add("filenames.solvx", repomd_sha1, solv_filenames_path);

    return 0;
}
