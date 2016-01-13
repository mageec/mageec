add_custom_target (doc)
add_custom_target (info)
add_custom_target (html)
add_custom_target (pdf)
add_dependencies (doc info html pdf)


# Find doxygen and makeinfo
find_package(Doxygen)
find_program(MAKEINFO_EXECUTABLE NAMES makeinfo)


# check required programs are available.
if ((MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS) AND
    NOT MAKEINFO_EXECUTABLE)
  message(SEND_ERROR "makeinfo missing, cannot generate user or design documentation")
endif ()

if (MAGEEC_WITH_DOXYGEN AND NOT DOXYGEN_FOUND)
  message(SEND_ERROR "doxygen missing, cannot generate api documentation")
endif()


set (doc_src ${CMAKE_CURRENT_SOURCE_DIR})

# make directories for the different types of documentation
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/info)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html)

if (MAKEINFO_EXECUTABLE)
  if (MAGEEC_WITH_USER_DOCS)
    add_custom_target (user-guide-info
      COMMAND ${MAKEINFO_EXECUTABLE} ${doc_src}/mageec.texi
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/info
      COMMENT "Generating MAGEEC user guide manual." VERBATIM)

    add_custom_target (user-guide-html
      COMMAND ${MAKEINFO_EXECUTABLE} ${doc_src}/mageec.texi "--html"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
      COMMENT "Generating MAGEEC user guide html." VERBATIM)

    add_dependencies (info user-guide-info)
    add_dependencies (html user-guide-html)
  endif ()

  if (MAGEEC_WITH_DESIGN_DOCS)
    add_custom_target (design-info
      COMMAND ${MAKEINFO_EXECUTABLE} ${doc_src}/mageec-design.texi
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/info
      COMMENT "Generated MAGEEC design document manual." VERBATIM)

    add_custom_target (design-html
      COMMAND ${MAKEINFO_EXECUTABLE} ${doc_src}/mageec-design.texi "--html"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
      COMMENT "Generated MAGEEC design document html." VERBATIM)

    add_dependencies (info design-info)
    add_dependencies (html design-html)
  endif()
endif()

if (DOXYGEN_FOUND AND MAGEEC_WITH_DOXYGEN)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                 @ONLY)

  add_custom_target (mageec-doxygen
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen
    COMMENT "Generating MAGEEC API documentation using doxygen." VERBATIM)

  add_dependencies (doc mageec-doxygen)
endif ()


# Install documentation by copying the various directories to the install
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/info    DESTINATION doc)
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html    DESTINATION doc)
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen DESTINATION doc)
