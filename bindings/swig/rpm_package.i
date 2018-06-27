%module rpm_package

%include <attribute.i>
%include <stdint.i>
%include <std_string.i>
%include <std_shared_ptr.i>

%{
// make SWIG wrap following headers
#include "libdnf/repo/solvable/Package.hpp"
#include "libdnf/repo/RpmPackage.hpp"
using namespace libdnf;
%}

%include <exception.i>
%exception {
try {
$action
}
catch (const std::exception & e)
{
SWIG_exception(SWIG_RuntimeError, (std::string("C++ std::exception: ") + e.what()).c_str());
}
catch (...)
{
SWIG_exception(SWIG_UnknownError, "C++ anonymous exception");
}
}

// make SWIG look into following headers
%include "libdnf/repo/solvable/Package.hpp"
%include "libdnf/repo/RpmPackage.hpp"

%shared_ptr(RpmPackage)

%ignore libdnf::RpmPackage::getChecksum;
%ignore libdnf::RpmPackage::getHeaderChecksum;

%attribute(libdnf::RpmPackage, const std::shared_ptr<DnfRepo>&, repo, getRepo, setRepo);
%attribute(libdnf::RpmPackage, const char *, debug_name, getDebugName);
%attribute(libdnf::RpmPackage, const char *, source_debug_name, getSourceDebugName);
%attribute(libdnf::RpmPackage, const char *, source_name, getSourceName);
%attribute(libdnf::RpmPackage, unsigned long, idx, getRpmdbId);
%attribute(libdnf::RpmPackage, const char *, repoid, getReponame);
%attribute(libdnf::RpmPackage, const char *, relativepath, getLocation);
%attribute(libdnf::RpmPackage, const char *, a, getArch);
%attribute(libdnf::RpmPackage, unsigned long, e, getEpoch);
%attribute(libdnf::RpmPackage, const char *, v, getVersion);
%attribute(libdnf::RpmPackage, const char *, r, getRelease);
%attribute(libdnf::RpmPackage, const char *, ui_from_repo, getReponame);

%pythoncode %{
# Compatible name aliases
RpmPackage.evr_eq = RpmPackage.isEvrEqual
RpmPackage.evr_gt = RpmPackage.isEvrGreater
RpmPackage.evr_lt = RpmPackage.isEvrLesser
RpmPackage.getDiscNum = RpmPackage.getMediaNumber
%}
