set(DOXYGEN_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(DOC_OUTPUT_FILE instruments.pdf)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/packages.toml.in
    ${CMAKE_BINARY_DIR}/packages.toml
)
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/contents.toml.in
    ${CMAKE_BINARY_DIR}/contents.toml
)
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in
    ${CMAKE_BINARY_DIR}/Doxyfile
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/build/output/index.adoc
    COMMAND doxygen ${CMAKE_BINARY_DIR}/Doxyfile &&
            asciidoxy --spec-file ${CMAKE_BINARY_DIR}/packages.toml -b adoc ${CMAKE_CURRENT_LIST_DIR}/index.adoc
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${DOC_OUTPUT_FILE}
    DEPENDS
        ${CMAKE_CURRENT_LIST_FILE}
        ${CMAKE_BINARY_DIR}/build/output/index.adoc
    COMMAND asciidoctor-pdf
        -a pdf-themesdir=${CMAKE_CURRENT_LIST_DIR}/theme/
        -a pdf-theme=tutorial.yml
        -a pdf-fontsdir=${CMAKE_CURRENT_LIST_DIR}/theme/fonts
        -a iconsdir=${CMAKE_CURRENT_LIST_DIR}/theme/icons
        -r asciidoctor-mathematical
        -d book
        -D ${PROJECT_BINARY_DIR}
        -o ${DOC_OUTPUT_FILE}
        --failure-level=ERROR
        ${CMAKE_BINARY_DIR}/build/output/index.adoc
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(docs DEPENDS ${CMAKE_BINARY_DIR}/${DOC_OUTPUT_FILE})
