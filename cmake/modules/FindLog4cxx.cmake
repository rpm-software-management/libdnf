# - Try to find the log4cxx logging library
# Will define:
#
# LOG4CXX_INCLUDE_DIRS - include directories needed to use the library
# LOG4CXX_LIBRARIES - Libraries to link agains fpr the driver
# LOG4CXX_RUNTIME_LIBRARIES - if found, the runtime libraries for log4cxx (dlls on windows)
#
# Possible hints:
# LOG4CXX_VISUAL_STUDIO_PROJECT - root directory of the log4cxx visual studio project on windows
#
# This file may be licensed under the terms of the
# GNU Lesser General Public License Version 3 (the ``LGPL''),
# or (at your option) any later version.
#
# Software distributed under the License is distributed
# on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the LGPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the LGPL along with this
# program. If not, go to http://www.gnu.org/licenses/lgpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The development of this software was supported by:
#   CoR-Lab, Research Institute for Cognition and Robotics
#     Bielefeld University

INCLUDE(FindPackageHandleStandardArgs)

# use pkg-config as a hint if available
INCLUDE(FindPkgConfig)
IF(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(PKG liblog4cxx)
ENDIF()

FIND_PATH(LOG4CXX_INCLUDE_DIRS
          HINTS ${LOG4CXX_VISUAL_STUDIO_PROJECT}/src/main/include
                ${PKG_INCLUDE_DIRS}
          NAMES logger.h
          PATH_SUFFIXES log4cxx)

FIND_LIBRARY(LOG4CXX_LIBRARIES
             HINTS ${LOG4CXX_VISUAL_STUDIO_PROJECT}/projects/Debug
                   ${LOG4CXX_VISUAL_STUDIO_PROJECT}/projects/Release
                   ${PKG_LIBRARY_DIRS}
             NAMES log4cxx)

IF(WIN32)
    FIND_FILE(LOG4CXX_RUNTIME_LIBRARIES
              HINTS ${LOG4CXX_VISUAL_STUDIO_PROJECT}/projects/Debug
                    ${LOG4CXX_VISUAL_STUDIO_PROJECT}/projects/Release
                    ${PKG_LIBRARY_DIRS}
              NAMES "log4cxx${CMAKE_SHARED_LIBRARY_SUFFIX}")
    MESSAGE(STATUS "log4cxx runtime libraries (log4cxx${CMAKE_SHARED_LIBRARY_SUFFIX}): ${LOG4CXX_RUNTIME_LIBRARIES}")
ELSE()
    SET(LOG4CXX_RUNTIME_LIBRARIES ${LOG4CXX_LIBRARIES})
ENDIF()

IF(LOG4CXX_RUNTIME_LIBRARIES)
    GET_FILENAME_COMPONENT(LOG4CXX_RUNTIME_LIBRARY_DIRS ${LOG4CXX_RUNTIME_LIBRARIES} PATH)
    MESSAGE(STATUS "log4cxx runtime library dir: ${LOG4CXX_RUNTIME_LIBRARY_DIRS}")
ENDIF()

# post-process inlude path
IF(LOG4CXX_INCLUDE_DIRS)
    STRING(REGEX REPLACE log4cxx$$ "" LOG4CXX_INCLUDE_DIRS ${LOG4CXX_INCLUDE_DIRS})
    SET(LOG4CXX_INCLUDE_DIRS ${LOG4CXX_INCLUDE_DIRS} CACHE PATH "log4cxx include dirs" FORCE)
ENDIF()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Log4cxx DEFAULT_MSG LOG4CXX_INCLUDE_DIRS LOG4CXX_LIBRARIES)

# only visible in advanced view
MARK_AS_ADVANCED(LOG4CXX_INCLUDE_DIRS LOG4CXX_LIBRARIES)
