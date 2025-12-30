set_extra_dirs_lib(DPP dpp)
find_library(DPP_LIBRARY
        NAMES dpp
        HINTS ${HINTS_DPP_LIBDIR} ${HINTS_DPP_LIBDIR_AGNOSTIC}
)

set_extra_dirs_include(DPP dpp)
find_path(DPP_INCLUDEDIR
        NAMES dpp/dpp.h
        HINTS ${HINTS_DPP_INCLUDEDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DPP DEFAULT_MSG DPP_LIBRARY DPP_INCLUDEDIR)

mark_as_advanced(DPP_LIBRARY DPP_INCLUDEDIR)

if(DPP_FOUND)
    set(DPP_LIBRARIES ${DPP_LIBRARY})
    set(DPP_INCLUDE_DIRS ${DPP_INCLUDEDIR})
    set(DPP_COPY_FILES)
    is_bundled(DPP_BUNDLED "${DPP_LIBRARY}")

    if(WIN32)
        list(APPEND DPP_COPY_FILES
                "${EXTRA_DPP_LIBDIR}/dpp.dll"
                "${EXTRA_DPP_LIBDIR}/opus.dll"
                "${EXTRA_DPP_LIBDIR}/zlib1.dll"
        )
        if(${TARGET_BITS} EQUAL 64)
            list(APPEND DPP_COPY_FILES
                    "${EXTRA_DPP_LIBDIR}/libcrypto-1_1-x64.dll"
                    "${EXTRA_DPP_LIBDIR}/libssl-1_1-x64.dll"
            )
        else()
            list(APPEND DPP_COPY_FILES
                    "${EXTRA_DPP_LIBDIR}/libcrypto-1_1.dll"
                    "${EXTRA_DPP_LIBDIR}/libssl-1_1.dll"
            )
        endif()
    endif()
endif()