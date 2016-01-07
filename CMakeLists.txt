cmake_minimum_required(VERSION 3.0.2)

project(MAGEEC)

# Debug enabled by default, all features disabled.
set (MAGEEC_DEBUG            True)
set (MAGEEC_WITH_ALL_DOCS    False)
set (MAGEEC_WITH_DESIGN_DOCS False)
set (MAGEEC_WITH_DOXYGEN     False)
set (MAGEEC_WITH_GCC_PLUGIN  False)
set (MAGEEC_WITH_C5          False)


# MAGEEC options
option (MAGEEC_DEBUG
        "Enable debug for MAGEEC")
option (MAGEEC_WITH_ALL_DOCS
        "Generate all documentation")
option (MAGEEC_WITH_USER_DOCS
        "Build MAGEEC user documentation")
option (MAGEEC_WITH_DESIGN_DOCS
        "Build MAGEEC with design documentation")
option (MAGEEC_WITH_DOXYGEN
        "Generate MAGEEC API documentation with Doxygen")
option (MAGEEC_WITH_GCC_PLUGIN
        "Build the GCC plugin as well as MAGEEC")
option (MAGEEC_WITH_C5
        "Built the C5 classifier as well as MAGEEC")


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

set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


# Set compiler options depending on the compiler used
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # Clang compiler options
  set (CMAKE_CXX_FLAGS "\
-fPIC -std=c++11 -Werror -Weverything -Wno-c++98-compat \
-Wno-c++98-compat-pedantic -Wno-global-constructors -Wno-padded \
-Wno-weak-vtables -Wno-switch-enum")
  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # GCC compiler options
  set (CMAKE_CXX_FLAGS "-fPIC -std=c++11 -Werror -Wall -Wextra")
  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-g3 ${CMAKE_CXX_FLAGS}")
  endif ()

else ()
  # Unknown compiler
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Pass the mageec version number into the compile
add_definitions(
  -DMAGEEC_VERSION_MAJOR=${MAGEEC_VERSION_MAJOR}
  -DMAGEEC_VERSION_MINOR=${MAGEEC_VERSION_MINOR}
  -DMAGEEC_VERSION_PATCH=${MAGEEC_VERSION_PATCH}
)
# Add debug flag
if (MAGEEC_DEBUG)
  add_definitions (-DMAGEEC_WITH_DEBUG)
endif ()

# Install header files
install (DIRECTORY include/mageec DESTINATION include)

include_directories(include)


### Targets ###


# MAGEEC library
add_library (mageec_core
  lib/DatabaseQuery.cpp
  lib/Database.cpp
  lib/Framework.cpp
  lib/TrainedML.cpp
  lib/Util.cpp
)
set_target_properties(mageec_core PROPERTIES OUTPUT_NAME mageec)
target_link_libraries(mageec_core sqlite3)

# Machine learners incorporated into MAGEEC
set (ML_SOURCES "lib/ML/C5.cpp" "lib/ML/FileML.cpp")

add_library (mageec_ml ${ML_SOURCES})
target_link_libraries(mageec_ml mageec_core)

# Standalone tool executable
add_executable (mageec_driver
  lib/Driver.cpp
)
set_target_properties(mageec_driver PROPERTIES OUTPUT_NAME mageec)
target_link_libraries(mageec_driver mageec_core mageec_ml)

# Test file
add_executable (mageec_test
  lib/Main.cpp
)
target_link_libraries(mageec_test mageec_core)

# Install libraries and executables
install(TARGETS mageec_test mageec_driver mageec_ml mageec_core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Documentation
if (MAGEEC_WITH_ALL_DOCS)
  set (MAGEEC_WITH_USER_DOCS True)
  set (MAGEEC_WITH_DESIGN_DOCS True)
  set (MAGEEC_WITH_DOXYGEN True)
endif()

if (MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS OR MAGEEC_WITH_DOXYGEN)
  add_subdirectory (doc)
endif ()

# GCC plugin
if (EXISTS ${PROJECT_SOURCE_DIR}/gcc_plugin AND MAGEEC_WITH_GCC_PLUGIN)
  add_subdirectory(gcc_plugin)
endif()

# Standalone C5 classifier
if (MAGEEC_WITH_C5)
  add_subdirectory (lib/ML/C5)
endif ()
