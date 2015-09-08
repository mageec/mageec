cmake_minimum_required(VERSION 3.2.2)

project(MAGEEC)

# default to having debug enabled
set (MAGEEC_DEBUG True)


# Documentation options
option (MAGEEC_WITH_ALL_DOCS    "Generate all documentation")
option (MAGEEC_WITH_USER_DOCS   "Build MAGEEC user documentation")
option (MAGEEC_WITH_DESIGN_DOCS "Build MAGEEC with design documentation")
option (MAGEEC_WITH_DOXYGEN     "Generate MAGEEC API documentation with Doxygen")

option (MAGEEC_DEBUG "Enable debug for MAGEEC" True)


# This should match the version number found in the mageec framework source
set (MAGEEC_VERSION_MAJOR 2)
set (MAGEEC_VERSION_MINOR 0)
set (MAGEEC_VERSION_PATCH 0)

if (NOT PACKAGE_VERSION)
  set(PACKAGE_VERSION
    "${MAGEEC_VERSION_MAJOR}.${MAGEEC_VERSION_MINOR}.${MAGEEC_VERSION_PATCH}")
endif()

set (PACKAGE_NAME MAGEEC)
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


# Set compiler options depending on the compiler used
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Weverything -Wno-c++98-compat -Wno-global-constructors -Wno-padded -Wno-weak-vtables")
  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -Wextra")
  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()
else ()
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Install header files
install (DIRECTORY include/mageec DESTINATION include)


# Main executable
include_directories(include)

if (MAGEEC_DEBUG)
  add_definitions (-DMAGEEC_DEBUG)
endif ()

add_executable (tmp
  lib/Database.cpp
  lib/DatabaseQuery.cpp
  lib/Framework.cpp
  lib/Main.cpp
  lib/TrainedML.cpp
)
target_link_libraries(tmp sqlite3)


# Documentation
if (MAGEEC_WITH_ALL_DOCS)
  set (MAGEEC_WITH_USER_DOCS True)
  set (MAGEEC_WITH_DESIGN_DOCS True)
  set (MAGEEC_WITH_DOXYGEN True)
endif()

if (MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS OR MAGEEC_WITH_DOXYGEN)
  add_subdirectory (doc)
endif ()
