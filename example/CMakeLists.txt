set(EXECUTABLE_PROJECT "${PROJECT_NAME}-example")

add_executable(${EXECUTABLE_PROJECT} main.cpp)

target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC

        Threads::Threads
        ${PROJECT_NAME}
        )

# Set the output of TARGET to be 
# - CMAKE_BIN_DIR/lib for libraries
# - CMAKE_BIN_DIR/bin for BINARIES
# an additional argument specifies the subdirectory.
function(set_target_directory_hierarchy TARGET)
    set_target_properties(${TARGET}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
    )
endfunction()

    target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        )

set_target_directory_hierarchy(${EXECUTABLE_PROJECT})

target_include_directories(${EXECUTABLE_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (LIBRT)
    target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC
        ${LIBRT}
        )
endif ()
