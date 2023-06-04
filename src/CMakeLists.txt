set(EXECUTABLE_PROJECT "${PROJECT_NAME}")
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

set(SOURCE_FILES
    main.cpp
    session.cpp

    payloads/subscription.cpp
    payloads/session-welcome.cpp

    payloads/channel-ban-v1.cpp
    payloads/stream-online-v1.cpp
    payloads/stream-offline-v1.cpp

    messages/metadata.cpp
        )

add_executable(${EXECUTABLE_PROJECT} ${SOURCE_FILES})

# Generate source groups for use in IDEs
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC

        Threads::Threads
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

target_include_directories(${EXECUTABLE_PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/autogen/)

set_target_directory_hierarchy(${EXECUTABLE_PROJECT})

# Used to provide a date of build in the About page (for nightly builds). Getting the actual time of
# compilation in CMake is a more involved, as documented in https://stackoverflow.com/q/24292898.
# For CI runs, however, the date of build file generation should be consistent with the date of
# compilation so this approximation is "good enough" for our purpose.
if (DEFINED ENV{CHATTERINO_SKIP_DATE_GEN})
    set(cmake_gen_date "1970-01-01")
else ()
    string(TIMESTAMP cmake_gen_date "%Y-%m-%d")
endif ()

if (MSVC)
    target_compile_options(${EXECUTABLE_PROJECT} PUBLIC /EHsc /bigobj)
endif ()

target_include_directories(${EXECUTABLE_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/autogen/)

# semver dependency https://github.com/Neargye/semver
target_include_directories(${EXECUTABLE_PROJECT} PUBLIC ${CMAKE_SOURCE_DIR}/lib/semver/include)

# miniaudio dependency https://github.com/mackron/miniaudio
target_include_directories(${EXECUTABLE_PROJECT} PUBLIC ${CMAKE_SOURCE_DIR}/lib/miniaudio)

if (UNIX)
    if (CMAKE_DL_LIBS)
        # libdl is a requirement for miniaudio on Linux
        message(STATUS "Linking with CMake DL libs: '${CMAKE_DL_LIBS}'")
        target_link_libraries(${EXECUTABLE_PROJECT} PUBLIC ${CMAKE_DL_LIBS})
    endif ()
endif ()

target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC
        ${Boost_LIBRARIES}
        )

    target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        )

if (LIBRT)
    target_link_libraries(${EXECUTABLE_PROJECT}
        PUBLIC
        ${LIBRT}
        )
endif ()

# Configure compiler warnings
if (MSVC)
    # Change flags for RelWithDebInfo

    # Default: "/debug /INCREMENTAL"
    # Changes:
    #   - Disable incremental linking to reduce padding
    #   - Enable all optimizations - by default when /DEBUG is specified,
    #     these optimizations will be disabled. We need /DEBUG to generate a PDB.
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/20812 for more details.
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:REF,ICF,LBR")

    # Use the function inlining level from 'Release' mode (2).
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Configure warnings

    # Someone adds /W3 before we add /W4.
    # This makes sure, only /W4 is specified.
    string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    # 4505 - "unreferenced local version has been removed"
    #        Although this might give hints on dead code,
    #        there are some cases where it's distracting.
    #
    # 4100 - "unreferenced formal parameter"
    #        There are a lot of functions and methods where
    #        an argument was given a name but never used.
    #        There's a clang-tidy rule that will catch this
    #        for new/updated functions/methods.
    #
    # 4267 - "possible loss of data in return"
    #        These are implicit conversions from size_t to int/qsizetype.
    #        We don't use size_t in a lot of cases, since
    #        Qt doesn't use it - it uses int (or qsizetype in Qt6).
    target_compile_options(${EXECUTABLE_PROJECT} PUBLIC
        /W4
        # 5038 - warnings about initialization order
        /w15038
        # 4855 - implicit capture of 'this' via '[=]' is deprecated
        /w14855
        # Disable the following warnings (see reasoning above)
        /wd4505
        /wd4100
        /wd4267
    )
    # Disable min/max macros from Windows.h
    target_compile_definitions(${EXECUTABLE_PROJECT} PUBLIC NOMINMAX)
else ()
    target_compile_options(${EXECUTABLE_PROJECT} PUBLIC
        -Wall
        # Disable the following warnings
        -Wno-unused-function
        -Wno-switch
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-unused-variable

        # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
        -Wno-strict-aliasing

        -Werror=return-type
        -Werror=reorder
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${EXECUTABLE_PROJECT} PUBLIC
            -Wno-unused-local-typedef
            -Wno-unused-private-field
            -Werror=inconsistent-missing-override
            -Werror=final-dtor-non-final-class
            -Werror=ambiguous-reversed-operator

        )
    else ()
        target_compile_options(${EXECUTABLE_PROJECT} PUBLIC
            -Wno-class-memaccess
        )
    endif()
endif ()

if(CHATTERINO_ENABLE_LTO)
    message(STATUS "Enabling LTO for ${EXECUTABLE_PROJECT}")
    set_property(TARGET ${EXECUTABLE_PROJECT}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
