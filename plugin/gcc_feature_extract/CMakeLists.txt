cmake_minimum_required(VERSION 3.0.2)


# Check if we are building standalone, or as part of MAGEEC
set (STANDALONE_BUILD False)
if ( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  set (STANDALONE_BUILD True)
endif ()


# If we're standalone, initialize a new project
if (STANDALONE_BUILD)
  project(GCC_FEATURE_EXTRACT_PLUGIN)
endif ()

# Version numbers for the plugin
set (GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR 2)
set (GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR 0)
set (GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH 0)

# Library and include search paths
set (GCC_PLUGIN_INCLUDE_DIR False)
set (MAGEEC_INCLUDE_DIR     False)
set (MAGEEC_LIBRARY_DIR     False)


if (STANDALONE_BUILD)
  # Options common to the main project build
  option (MAGEEC_DEBUG
          "Enable debug for the MAGEEC GCC plugin" True)
  option (GCC_PLUGIN_INCLUDE_DIR
          "Directory in which GCC plugin headers can be found")

  # Options unique to the GCC plugin build
  option (MAGEEC_INCLUDE_DIR
          "Directory in which the MAGEEC library can be found")
  option (MAGEEC_LIBRARY_DIR
          "Directory in which the MAGEEC headers can be found")

  if (NOT PACKAGE_VERSION)
    set(PACKAGE_VERSION "\
${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR}.\
${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR}.\
${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH}")
  endif()

  set (PACKAGE_NAME GCC_FEATURE_EXTRACT_PLUGIN)
  set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

  set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


else ()
  # MAGEEC_DEBUG and GCC_PLUGIN_INCLUDE_DIR are inherited from the
  # main project build. The MAGEEC includes and libraries should be in the
  # build tree anyway.
  # A search will be done to find the gcc plugin headers.
endif ()


# Set compiler options depending on the compiler
#
# In debug mode, disable optimizations, and use -Werror will warnings
# selectively disabled.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "-fPIC -fno-rtti -std=gnu++11")
  if (NOT MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-O1 ${CMAKE_CXX_FLAGS}")
  endif()
  
  # Quite a lot of warnings must be disabled in order to get the GCC plugin
  # headers to build.
  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 \
-Werror -Weverything -Wno-c++98-compat-pedantic -Wno-global-constructors \
-Wno-padded -Wno-weak-vtables -Wno-switch-enum -Wno-reserved-id-macro \
-Wno-old-style-cast -Wno-undef -Wno-sign-conversion \
-Wno-mismatched-tags -Wno-reserved-user-defined-literal \
-Wno-shadow -Wno-shorten-64-to-32 -Wno-cast-align -Wno-conversion \
-Wno-disabled-macro-expansion -Wno-exit-time-destructors")
  endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "-fPIC -fno-rtti -std=gnu++11")
  if (NOT MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-O1 ${CMAKE_CXX_FLAGS}")
  endif()

  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 \
-Werror -Wall -Wextra -Wno-literal-suffix")
  endif ()

else ()
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Add plugin version numbers to the compile, these should match the version
# numbers found in the plugin
add_definitions (
  -DGCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR=${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR}
  -DGCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR=${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR}
  -DGCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH=${GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH}
)
# Define debug
if (MAGEEC_DEBUG)
  add_definitions ( -DMAGEEC_WITH_DEBUG )
endif ()


# Search for GCC plugin headers if a path is not provided by the user
set (PLUGIN_INC_FOUND 0)
if (GCC_PLUGIN_INCLUDE_DIR)
  set (PLUGIN_INC_FOUND 1)
endif ()

if (NOT PLUGIN_INC_FOUND)
  message (STATUS "Check for gcc")
  find_program(GCC_EXEC gcc)
  if (NOT GCC_EXEC)
    message (FATAL_ERROR "Cannot find gcc plugin headers without gcc")
  endif ()
  message (STATUS "Found GCC: ${GCC_EXEC}")

  message(STATUS "Check for gcc plugin headers")
  execute_process(
    COMMAND ${GCC_EXEC} "-print-file-name=plugin"
    RESULT_VARIABLE PLUGIN_INC_SEARCH_RES
    ERROR_VARIABLE  PLUGIN_INC_SEARCH_ERR
    OUTPUT_VARIABLE PLUGIN_INC_BASE
  )
  if (PLUGIN_INC_SEARCH_RES EQUAL 0)
    set (PLUGIN_INC_FOUND 1)
    string (STRIP ${PLUGIN_INC_BASE} PLUGIN_INC_BASE)
    set (GCC_PLUGIN_INCLUDE_DIR ${PLUGIN_INC_BASE}/include)
  else ()
    message (FATAL_ERROR "No gcc plugin headers in ${PLUGIN_INC_SEARCH_ERR}")
  endif ()
endif()

# Check the plugin headers exist at the provided path
message (STATUS "gcc plugin headers in ${GCC_PLUGIN_INCLUDE_DIR}")
message (STATUS "Checking for gcc-plugin.h")
if (NOT EXISTS "${GCC_PLUGIN_INCLUDE_DIR}/gcc-plugin.h")
  message (FATAL_ERROR
      "Could not source file ${GCC_PLUGIN_INCLUDE_DIR}/gcc-plugin.h")
endif ()
message (STATUS "gcc-plugin.h found")

include_directories(${GCC_PLUGIN_INCLUDE_DIR})

# If MAGEEC header and library paths were defined (because we are building
# out of tree). Then include those directories.
if (MAGEEC_INCLUDE_DIR)
  include_directories(${MAGEEC_INCLUDE_DIR})
endif ()
if (MAGEEC_LIBRARY_DIR)
  link_directories(${MAGEEC_LIBRARY_DIR})
endif ()


### Targets ###


# Plugin library target, link against the mageec core library and the
# machine learners
add_library(gcc_feature_extract SHARED
  FeatureExtract.cpp
  Plugin.cpp
)
target_link_libraries(gcc_feature_extract mageec_core)

# install the plugin
install(TARGETS gcc_feature_extract
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
