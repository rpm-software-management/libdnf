# Find the python3 librepo binding
find_library(PYLIBREPO_LIBRARY NAMES _librepo.so
             PATHS /usr/lib64/python3.6/site-packages/librepo/)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PyLibRepo DEFAULT_MSG PYLIBREPO_LIBRARY)
mark_as_advanced(PYLIBREPO_LIBRARY)
