%module module

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include <std_shared_ptr.i>

%shared_ptr(ModulePackage)
typedef std::shared_ptr< ModulePackage > ModulePackagePtr;

%{
    // make SWIG wrap following headers
    #include "libdnf/module/ModulePackage.hpp"
    #include "libdnf/module/ModulePackageContainer.hpp"
%}

%template() std::vector<std::shared_ptr<ModulePackage>>;
%template() std::map<std::string, std::string>;
%template() std::map<std::string, std::pair<std::string, std::string>>;
%template() std::map<std::string, std::vector<std::string>>;

// make SWIG wrap following headers
%nodefaultctor ModulePackage;

%include "libdnf/module/ModulePackage.hpp"
%include "libdnf/module/ModulePackageContainer.hpp"
