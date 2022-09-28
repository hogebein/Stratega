set(NTBEA_SOURCE_FILES
        main.cpp
        )

list(TRANSFORM NTBEA_SOURCE_FILES PREPEND "${SUBPROJ_NTBEA_SRC_DIR}/")

add_executable (ntbea ${NTBEA_SOURCE_FILES})
target_link_libraries(ntbea PUBLIC Stratega)

set_target_properties(ntbea PROPERTIES
         CXX_STANDARD 17
         LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/lib
         RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/bin
         ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/lib
         EXECUTABLE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/bin

         LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBRARY_OUTPUT_DIRECTORY}
         RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY}
         ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${ARCHIVE_OUTPUT_DIRECTORY}
         EXECUTABLE_OUTPUT_DIRECTORY_RELEASE ${EXECUTABLE_OUTPUT_DIRECTORY}
         
         LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIBRARY_OUTPUT_DIRECTORY}
         RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY}
         ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${ARCHIVE_OUTPUT_DIRECTORY}
         EXECUTABLE_OUTPUT_DIRECTORY_DEBUG ${EXECUTABLE_OUTPUT_DIRECTORY}
        )