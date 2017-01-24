cmake_minimum_required(VERSION 3.0.2)


# Check if we are building standalone, or as part of MAGEEC
set (STANDALONE_BUILD False)
if ( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  set (STANDALONE_BUILD True)
endif ()


# If we're standalone, initialize a new project
if (STANDALONE_BUILD)
  project(GCC_DRIVER)
endif ()

# Version numbers for the plugin
set (GCC_DRIVER_VERSION_MAJOR 1)
set (GCC_DRIVER_VERSION_MINOR 0)
set (GCC_DRIVER_VERSION_PATCH 0)

# Library and include search paths
set (MAGEEC_INCLUDE_DIR     False)
set (MAGEEC_LIBRARY_DIR     False)


if (STANDALONE_BUILD)
  # Options common to the main project build
  option (MAGEEC_DEBUG
          "Enable debug for the MAGEEC GCC Driver" True)

  # Options unique to the GCC plugin build
  option (MAGEEC_INCLUDE_DIR
          "Directory in which the MAGEEC library can be found")
  option (MAGEEC_LIBRARY_DIR
          "Directory in which the MAGEEC headers can be found")

  if (NOT PACKAGE_VERSION)
    set(PACKAGE_VERSION "\
${GCC_DRIVER_VERSION_MAJOR}.\
${GCC_DRIVER_VERSION_MINOR}.\
${GCC_DRIVER_VERSION_PATCH}")
  endif()

  set (PACKAGE_NAME GCC_DRIVER)
  set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

  set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)


else ()
  # MAGEEC_DEBUG is inherited from the main project build. The MAGEEC includes
  # and libraries should be in the build tree anyway.
endif ()


# Set compiler options depending on the compiler
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
  message (WARNING "Unrecognized compiler, CXXFLAGS unmodified")
endif ()


# Add plugin version numbers to the compile, these should match the version
# numbers found in the plugin
add_definitions (
  -DGCC_DRIVER_VERSION_MAJOR=${GCC_DRIVER_VERSION_MAJOR}
  -DGCC_DRIVER_VERSION_MINOR=${GCC_DRIVER_VERSION_MINOR}
  -DGCC_DRIVER_VERSION_PATCH=${GCC_DRIVER_VERSION_PATCH}
)
# Define debug
if (MAGEEC_DEBUG)
  add_definitions ( -DMAGEEC_WITH_DEBUG )
endif ()


# If MAGEEC header and library paths were defined (because we are building
# out of tree). Then include those directories.
if (MAGEEC_INCLUDE_DIR)
  include_directories(${MAGEEC_INCLUDE_DIR})
endif ()
if (MAGEEC_LIBRARY_DIR)
  link_directories(${MAGEEC_LIBRARY_DIR})
endif ()


### Targets ###


# Driver target, link against the mageec core and the machine learners
add_executable(gcc_driver Driver.cpp)
set_target_properties(gcc_driver PROPERTIES OUTPUT_NAME mageec-gcc)
target_link_libraries(gcc_driver mageec_core mageec_ml)

# install the driver
install(TARGETS gcc_driver
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# FIXME: Linux specific. Create a symlink from mageec-gcc to
# mageec-gfortran and mageec-g++
install(CODE
        "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
                         ${CMAKE_INSTALL_PREFIX}/bin/mageec-gcc \
                         ${CMAKE_INSTALL_PREFIX}/bin/mageec-gfortran)")
install(CODE
        "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
                         ${CMAKE_INSTALL_PREFIX}/bin/mageec-gcc \
                         ${CMAKE_INSTALL_PREFIX}/bin/mageec-g++)")
       
