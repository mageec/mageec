cmake_minimum_required(VERSION 3.0.2)

project(MAGEEC)


# MAGEEC options
# Flags are all enabled by default. The user is expected to selectively
# disable them as required.
option (MAGEEC_DEBUG
        "Enable debug for MAGEEC" True)
option (MAGEEC_WITH_USER_DOCS
        "Build MAGEEC user documentation" True)
option (MAGEEC_WITH_DESIGN_DOCS
        "Build MAGEEC with design documentation" True)
option (MAGEEC_WITH_DOXYGEN
        "Generate MAGEEC API documentation with Doxygen" True)
option (MAGEEC_WITH_GCC_FEATURE_EXTRACT_PLUGIN
        "Build with the GCC feature extraction plugin" True)
option (MAGEEC_WITH_GCC_DRIVER
        "Build with the GCC driver" True)
option (MAGEEC_WITH_C5
        "Built the C5 classifier as well as MAGEEC" True)

# Search path to find gcc plugin headers.
option (GCC_PLUGIN_INCLUDE_DIR
        "Directory in which GCC plugin headers can be found")


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
#
# In debug mode, disable optimizations, and use -Werror will warnings
# selectively disabled.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "-fPIC -std=c++11")
  if (NOT MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-O1 ${CMAKE_CXX_FLAGS}")
  endif()

  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 \
-Werror -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
-Wno-global-constructors -Wno-padded -Wno-weak-vtables -Wno-switch-enum")
  endif ()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "-fPIC -std=c++11")
  if (NOT MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "-O1 ${CMAKE_CXX_FLAGS}")
  endif()

  if (MAGEEC_DEBUG)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Werror -Wall -Wextra")
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
add_executable (mageec_driver lib/Driver.cpp)
set_target_properties(mageec_driver PROPERTIES OUTPUT_NAME mageec)
target_link_libraries(mageec_driver mageec_core mageec_ml)

# Install libraries and executables
install(TARGETS mageec_driver mageec_ml mageec_core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install scripts
install (DIRECTORY script/ DESTINATION bin PATTERN "*"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)

# Documentation
if (MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS OR MAGEEC_WITH_DOXYGEN)
  add_subdirectory (doc)
endif ()

# GCC feature extraction plugin
if (EXISTS ${PROJECT_SOURCE_DIR}/plugin/gcc_feature_extract AND MAGEEC_WITH_GCC_FEATURE_EXTRACT_PLUGIN)
  add_subdirectory(plugin/gcc_feature_extract)
endif()

# GCC driver
if (EXISTS ${PROJECT_SOURCE_DIR}/tools/gcc_driver AND MAGEEC_WITH_GCC_DRIVER)
  add_subdirectory(tools/gcc_driver)
endif()

# Standalone C5 classifier
if (MAGEEC_WITH_C5)
  add_subdirectory (lib/ML/C5)
endif ()
