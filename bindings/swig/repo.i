%module(directors="1") repo

%include <stdint.i>
%include <std_vector.i>
%include <std_string.i>

%import(module="libdnf.common_types") "common_types.i"
%import(module="libdnf.conf") "conf.i"

%include "catch_error.i"

%begin %{
    #define SWIG_PYTHON_2_UNICODE
%}

%{
    // make SWIG wrap following headers
    #include "libdnf/repo/Crypto.hpp"
    #include "libdnf/repo/Repo.hpp"
%}

// make SWIG look into following headers
%template(VectorPPackageTarget) std::vector<libdnf::PackageTarget *>;

%extend libdnf::Repo {
    Repo(const std::string & id, ConfigRepo * config)
    {
        return new libdnf::Repo(id, std::unique_ptr<libdnf::ConfigRepo>(config));
    }
    void setCallbacks(RepoCB * callbacks)
    {
        self->setCallbacks(std::unique_ptr<libdnf::RepoCB>(callbacks));
    }
    void setHttpHeaders(const std::vector<std::string> & headers)
    {
        const char * cHeaders[headers.size() + 1];
        for (size_t i = 0; i < headers.size(); ++i)
            cHeaders[i] = headers[i].c_str();
        cHeaders[headers.size()] = nullptr;
        self->setHttpHeaders(cHeaders);
    }
    std::vector<std::string> getHttpHeaders() const
    {
        auto cHeaders = self->getHttpHeaders();
        if (!cHeaders)
            return {};
        size_t headersCount = 0;
        while (cHeaders[headersCount])
            ++headersCount;
        std::vector<std::string> headers(headersCount);
        for (size_t i = 0; i < headersCount; ++i) {
            headers[i] = cHeaders[i];
        }
        return headers;
    }
}
%ignore libdnf::Repo::Repo;
%ignore libdnf::Repo::setCallbacks;
%ignore libdnf::Repo::setHttpHeaders;
%ignore libdnf::Repo::getHttpHeaders;

%extend libdnf::PackageTarget {
    PackageTarget(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType,
                  const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                  int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks,
                  const std::vector<std::string> & httpHeaders = {})
    {
        const char * cHeaders[httpHeaders.size() + 1];
        for (size_t i = 0; i < httpHeaders.size(); ++i)
            cHeaders[i] = httpHeaders[i].c_str();
        cHeaders[httpHeaders.size()] = nullptr;

        return new libdnf::PackageTarget(cfg, relativeUrl, dest, chksType,
            chksum, expectedSize, baseUrl, resume,
            byteRangeStart, byteRangeEnd, callbacks,
            httpHeaders.size() > 0 ? cHeaders : nullptr);
    }
}
%ignore libdnf::PackageTarget::PackageTarget(ConfigMain * cfg, const char * relativeUrl,
    const char * dest, int chksType,
    const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
    int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks,
    const char * httpHeaders[] = nullptr);

%ignore std::vector<libdnf::Key>::vector(size_type);
%ignore std::vector<libdnf::Key>::resize;
%template(VectorKey) std::vector<libdnf::Key>;
%include "libdnf/repo/Crypto.hpp"

%feature("director") libdnf::RepoCB;
%ignore libdnf::PackageTarget::PackageTarget(const PackageTarget & src);
%feature("director") libdnf::PackageTargetCB;
%include "libdnf/repo/Repo.hpp"
