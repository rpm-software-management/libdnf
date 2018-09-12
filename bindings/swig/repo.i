%module(directors="1") repo

%include <stdint.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_string.i>

%import(module="libdnf.conf") "conf.i"

%begin %{
    #define SWIG_PYTHON_2_UNICODE
%}

%{
    // make SWIG wrap following headers
    #include "libdnf/repo/Repo.hpp"
    typedef struct {
        PyObject_HEAD
        HyRepo repo;
    } _RepoObject;
    using namespace libdnf;
%}

%typemap(in) HyRepo {
    $1 = ((_RepoObject *)$input)->repo;
}

%include <exception.i>
%exception {
    try {
        $action
    } catch (const std::exception & e) {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

// make SWIG look into following headers
%template(VectorPairStringString) std::vector<std::pair<std::string, std::string>>;
%template(VectorPPackageTarget) std::vector<libdnf::PackageTarget *>;

%extend libdnf::Repo {
    Repo(const std::string & id, ConfigRepo * config)
    {
        return new Repo(id, std::unique_ptr<libdnf::ConfigRepo>(config));
    }
    void setCallbacks(RepoCB * callbacks)
    {
        self->setCallbacks(std::unique_ptr<libdnf::RepoCB>(callbacks));
    }
}
%ignore libdnf::Repo::Repo;
%ignore libdnf::Repo::setCallbacks;
%feature("director") libdnf::RepoCB;
%ignore libdnf::PackageTarget::PackageTarget(const PackageTarget & src);
%feature("director") libdnf::PackageTargetCB;
%include "libdnf/repo/Repo.hpp"
