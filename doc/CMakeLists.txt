add_custom_target (all-doc)
add_custom_target (all-info)
add_custom_target (all-html)
add_custom_target (all-pdf)
add_dependencies (all-doc all-info all-html all-pdf)


# Find doxygen and makeinfo
find_package(Doxygen)
find_program(MAKEINFO_EXECUTABLE NAMES makeinfo)


# check required programs are available.
if ((MAGEEC_WITH_USER_DOCS OR MAGEEC_WITH_DESIGN_DOCS) AND
    NOT MAKEINFO_EXECUTABLE)
  message(SEND_ERROR "makeinfo missing, cannot generate user/design documentation")
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

    add_dependencies (all-info user-guide-info)
    add_dependencies (all-html user-guide-html)
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

    add_dependencies (all-info design-info)
    add_dependencies (all-html design-html)
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

  add_dependencies (all-doc mageec-doxygen)
endif ()

