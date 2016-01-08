cmake_minimum_required(VERSION 3.0.2)


# Check if we are building standalone, or as part of MAGEEC
set (STANDALONE_BUILD False)
if ( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  set (STANDALONE_BUILD True)
endif ()


# If we're standalone, initialize a new project
if (STANDALONE_BUILD)
  project(MAGEEC_GCC_PLUGIN)
endif ()

# Version numbers for the plugin
set (MAGEEC_GCC_PLUGIN_VERSION_MAJOR 2)
set (MAGEEC_GCC_PLUGIN_VERSION_MINOR 0)
set (MAGEEC_GCC_PLUGIN_VERSION_PATCH 0)

# Debugging enabled by default
set (MAGEEC_GCC_PLUGIN_DEBUG True)

# Library and include search paths
set (GCC_PLUGIN_INCLUDE_DIR False)
set (MAGEEC_INCLUDE_DIR False)
set (MAGEEC_LIBRARY_DIR False)


if (STANDALONE_BUILD)
  # Options which may be provided to a standalone build
  option (MAGEEC_GCC_PLUGIN_DEBUG
          "Enable debug for the MAGEEC GCC plugin")
  option (GCC_PLUGIN_INCLUDE_DIR
          "Directory in which GCC plugin headers can be found")
  option (MAGEEC_INCLUDE_DIR
          "Directory in which the MAGEEC library can be found")
  option (MAGEEC_LIBRARY_DIR
          "Directory in which the MAGEEC headers can be found")


  if (NOT PACKAGE_VERSION)
    set(PACKAGE_VERSION "\
${MAGEEC_GCC_PLUGIN_VERSION_MAJOR}.\
${MAGEEC_GCC_PLUGIN_VERSION_MINOR}.\
${MAGEEC_GCC_PLUGIN_VERSION_PATCH}")
  endif()

  set (PACKAGE_NAME MAGEEC_GCC_PLUGIN)
  set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

  set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


else ()
  # Inherit debug from MAGEEC
  set (MAGEEC_GCC_PLUGIN_DEBUG ${MAGEEC_DEBUG})
endif ()


# Set compiler options depending on the compiler
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "\
-fPIC -fno-rtti -std=gnu++11 -Werror -Weverything \
-Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-padded \
-Wno-weak-vtables -Wno-switch-enum -Wno-reserved-id-macro \
-Wno-old-style-cast -Wno-undef -Wno-sign-conversion \
-Wno-mismatched-tags -Wno-reserved-user-defined-literal \
-Wno-shadow -Wno-shorten-64-to-32 -Wno-cast-align -Wno-conversion \
-Wno-disabled-macro-expansion -Wno-exit-time-destructors")
  if (MAGEEC_GCC_PLUGIN_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "\
-fPIC -fno-rtti -std=gnu++11 -Werror -Wall -Wextra -Wno-literal-suffix")
  if (MAGEEC_GCC_PLUGIN_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()

else ()
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Add plugin version numbers to the compile, these should match the version
# numbers found in the plugin
add_definitions (
  -DMAGEEC_GCC_PLUGIN_VERSION_MAJOR=${MAGEEC_GCC_PLUGIN_VERSION_MAJOR}
  -DMAGEEC_GCC_PLUGIN_VERSION_MINOR=${MAGEEC_GCC_PLUGIN_VERSION_MINOR}
  -DMAGEEC_GCC_PLUGIN_VERSION_PATCH=${MAGEEC_GCC_PLUGIN_VERSION_PATCH}
)
# Define debug
if (MAGEEC_GCC_PLUGIN_DEBUG)
  add_definitions ( -DMAGEEC_GCC_PLUGIN_DEBUG )
endif ()


# Search for GCC plugin headers if a path is not provided by the user
set (PLUGIN_INC_FOUND 0)
if (GCC_PLUGIN_INCLUDE_DIR)
  set (PLUGIN_INC_FOUND 1)
endif ()

if (NOT PLUGIN_INC_FOUND)
  message(STATUS "Searching for GCC plugin headers")
  execute_process(
    COMMAND gcc "-print-file-name=plugin"
    RESULT_VARIABLE PLUGIN_INC_SEARCH_RES
    ERROR_VARIABLE  PLUGIN_INC_SEARCH_ERR
    OUTPUT_VARIABLE PLUGIN_INC_BASE
  )
  if (PLUGIN_INC_SEARCH_RES EQUAL 0)
    set (PLUGIN_INC_FOUND 1)
    string (STRIP ${PLUGIN_INC_BASE} PLUGIN_INC_BASE)
    set (GCC_PLUGIN_INCLUDE_DIR ${PLUGIN_INC_BASE}/include)
  else ()
    message (FATAL_ERROR "Failed to find GCC plugin headers in ${PLUGIN_INC_SEARCH_ERR}")
  endif ()
endif()

message (STATUS "GCC plugin headers in ${GCC_PLUGIN_INCLUDE_DIR}")
message (STATUS "Checking for gcc-plugin.h")
if (NOT EXISTS "${GCC_PLUGIN_INCLUDE_DIR}/gcc-plugin.h")
  message (FATAL_ERROR
      "Could not source file ${GCC_PLUGIN_INCLUDE_DIR}/gcc-plugin.h")
endif ()
message (STATUS "gcc-plugin.h found")

include_directories(${GCC_PLUGIN_INCLUDE_DIR})

# Add other search directories for MAGEEC headers and libraries if they are
# defined
if (MAGEEC_INCLUDE_DIR)
  include_directories(${MAGEEC_INCLUDE_DIR})
endif ()
if (MAGEEC_LIBRARY_DIR)
  link_directories(${MAGEEC_LIBRARY_DIR})
endif ()


### Targets ###


# Plugin library target, link against the mageec core library and the
# machine learners
add_library(mageec_gcc SHARED
  FeatureExtract.cpp
  GCCCallbacks.cpp
  GCCPlugin.cpp
  PluginInfo.cpp
)
target_link_libraries(mageec_gcc mageec_core mageec_ml)

# install the plugin
install(TARGETS mageec_gcc
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
