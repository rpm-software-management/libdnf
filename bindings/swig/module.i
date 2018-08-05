%module module

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <std_map.i>
%include <std_pair.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>

%shared_ptr(ModulePackage)
typedef std::shared_ptr< ModulePackage > ModulePackagePtr;

%{
    // make SWIG wrap following headers
    #include "libdnf/module/ModulePackage.hpp"
    #include "libdnf/module/ModulePackageContainer.hpp"
%}

%template(VectorModulePackagePtr) std::vector<ModulePackagePtr>;
%template(MapStringString) std::map<std::string, std::string>;
%template(MapStringPairStringString) std::map<std::string, std::pair<std::string, std::string>>;
%template(VectorString) std::vector<std::string>;
%template(MapStringVectorString) std::map<std::string, std::vector<std::string>>;


// make SWIG wrap following headers
%nodefaultctor ModulePackage;

%include "libdnf/module/ModulePackage.hpp"
%include "libdnf/module/ModulePackageContainer.hpp"
