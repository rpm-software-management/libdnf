%module module

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <exception.i>
%include <std_map.i>
%include <std_pair.i>
%include <std_set.i>
%include <std_string.i>
%include <std_vector.i>


%exception {
    try {
        $action
    } catch (const std::exception & e) {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

typedef int Id;

%{
    // make SWIG wrap following headers
    #include "libdnf/module/ModulePackage.hpp"
    #include "libdnf/module/ModulePackageContainer.hpp"
    #include "libdnf/module/modulemd/ModuleProfile.hpp"
%}
%template(SetString) std::set<std::string>;
%template(VectorModulePackagePtr) std::vector<ModulePackage *>;
%template(VectorVectorVectorModulePackagePtr) std::vector<std::vector<std::vector<ModulePackage *>>>;
%template(VectorModuleProfile) std::vector<ModuleProfile>;
%template(MapStringString) std::map<std::string, std::string>;
%template(MapStringPairStringString) std::map<std::string, std::pair<std::string, std::string>>;

%include <std_vector_ext.i>

// this must follow std_vector_ext.i include, otherwise it returns garbage instead of list of strings
%template(MapStringVectorString) std::map<std::string, std::vector<std::string>>;

// make SWIG wrap following headers
%nodefaultctor ModulePackage;
%nodefaultctor ModuleProfile;

%include "libdnf/module/ModulePackage.hpp"
%include "libdnf/module/ModulePackageContainer.hpp"
%include "libdnf/module/modulemd/ModuleProfile.hpp"
