%module module

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <exception.i>
%include <std_map.i>
%include <std_pair.i>
%include <std_set.i>
%include <std_shared_ptr.i>
%include <std_string.i>
%include <std_vector.i>


%exception {
    try {
        $action
    }
    catch (const std::exception & e)
    {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
    catch (...)
    {
       SWIG_exception(SWIG_UnknownError, "C++ anonymous exception");
    }
}


%shared_ptr(ModulePackage)
typedef std::shared_ptr< ModulePackage > ModulePackagePtr;

typedef int Id;

%{
    // make SWIG wrap following headers
    #include "libdnf/module/ModulePackage.hpp"
    #include "libdnf/module/ModulePackageContainer.hpp"
    #include "libdnf/module/modulemd/ModuleProfile.hpp"
%}
%template(SetString) std::set<std::string>;
%template(VectorModulePackagePtr) std::vector<ModulePackagePtr>;
%template(VectorVectorVectorModulePackagePtr) std::vector<std::vector<std::vector<ModulePackagePtr>>>;
%template(VectorModuleProfile) std::vector<ModuleProfile>;
%template(MapStringString) std::map<std::string, std::string>;
%template(MapStringPairStringString) std::map<std::string, std::pair<std::string, std::string>>;
%template(VectorString) std::vector<std::string>;
%template(MapStringVectorString) std::map<std::string, std::vector<std::string>>;


// make SWIG wrap following headers
%nodefaultctor ModulePackage;
%nodefaultctor ModuleProfile;

%include "libdnf/module/ModulePackage.hpp"
%include "libdnf/module/ModulePackageContainer.hpp"
%include "libdnf/module/modulemd/ModuleProfile.hpp"
