cmake_minimum_required(VERSION 3.2.2)

project(MAGEEC)

# Documentation options
option (MAGEEC_WITH_ALL_DOCS    "Generate all documentation")
option (MAGEEC_WITH_USER_DOCS   "Build MAGEEC user documentation")
option (MAGEEC_WITH_DESIGN_DOCS "Build MAGEEC with design documentation")
option (MAGEEC_WITH_DOXYGEN     "Generate MAGEEC API documentation with Doxygen")

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
  set (CMAKE_CXXFLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Werror -Weverything -Wno-c++98-compat")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_CXXFLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Werror -Wall -Wextra")
else ()
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Install header files
install (DIRECTORY include/mageec DESTINATION include)

if (MAGEEC_WITH_ALL_DOCS)
  set (MAGEEC_WITH_USER_DOCS True)
  set (MAGEEC_WITH_DESIGN_DOCS True)
  set (MAGEEC_WITH_DOXYGEN True)
endif()

if (MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS OR MAGEEC_WITH_DOXYGEN)
  add_subdirectory (doc)
endif ()

