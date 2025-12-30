cmake_minimum_required(VERSION 3.22)

### Common

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(TARGET_BITS "32")
else()
    set(TARGET_BITS "64")
endif()
if(WIN32)
    set(TARGET_OS "windows")
elseif(UNIX)
    set(TARGET_OS "linux")
endif()

if(NOT(CMAKE_BUILD_TYPE))
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIB_DIR_AGNOSTIC "${TARGET_OS}/lib${TARGET_BITS}")
set(LIB_DIR "${TARGET_OS}/lib${TARGET_BITS}/${CMAKE_BUILD_TYPE}")

function(set_extra_dirs_lib VARIABLE NAME)
    set(DIR_AGNOSTIC "libs/${NAME}/${LIB_DIR_AGNOSTIC}")
    set("HINTS_${VARIABLE}_LIBDIR_AGNOSTIC" "${DIR_AGNOSTIC}" PARENT_SCOPE)
    set("EXTRA_${VARIABLE}_LIBDIR_AGNOSTIC" "${DIR_AGNOSTIC}" PARENT_SCOPE)

    set(DIR "libs/${NAME}/${LIB_DIR}")
    set("HINTS_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
    set("EXTRA_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
endfunction()

function(set_extra_dirs_include VARIABLE NAME)
    set("HINTS_${VARIABLE}_INCLUDEDIR" "libs/${NAME}/include" PARENT_SCOPE)
endfunction()

function(is_bundled VARIABLE PATH)
    if(PATH)
        string(FIND "${PATH}" "${PROJECT_SOURCE_DIR}" LOCAL_PATH_POS)
        if(LOCAL_PATH_POS EQUAL 0 AND TARGET_BITS AND TARGET_OS)
            set("${VARIABLE}" ON PARENT_SCOPE)
        else()
            set("${VARIABLE}" OFF PARENT_SCOPE)
        endif()
    else()
        set("${VARIABLE}" OFF PARENT_SCOPE)
    endif()
endfunction()

function(show_dependency_status OUTPUT_NAME NAME REQUIRED)
    if(${NAME}_FOUND)
        if(${NAME}_BUNDLED)
            message(STATUS " * ${OUTPUT_NAME} not found (using bundled version)")
        else()
            message(STATUS " * ${OUTPUT_NAME} found")
        endif()
    else()
        message(STATUS " * ${OUTPUT_NAME} not found")
        if(REQUIRED)
            message(SEND_ERROR "You must install ${OUTPUT_NAME} to compile MRPG discord bot")
        endif()
    endif()
endfunction()

function(add_dependency VAR FRIENDLY_NAME REQUIRED)
    find_package(${VAR})
    show_dependency_status(${FRIENDLY_NAME} ${VAR} ${REQUIRED})
    STRING(TOUPPER ${VAR} VAR_UPPER)

    if(${VAR_UPPER}_FOUND)
        if(TARGET ${PROJECT_NAME})
            target_link_libraries(${PROJECT_NAME} ${${VAR_UPPER}_LIBRARIES})
            target_include_directories(${PROJECT_NAME} PRIVATE ${${VAR_UPPER}_INCLUDE_DIRS})
        endif()

        if(DEFINED ${VAR_UPPER}_COPY_FILES)
            set(COPY_FILES ${COPY_FILES} ${${VAR_UPPER}_COPY_FILES} PARENT_SCOPE)
        endif()
    endif()
endfunction()

### Project

project(botoumed VERSION 1.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.hpp)
add_executable(${PROJECT_NAME} ${SOURCES})

add_dependency(DPP "DPP" REQUIRED)

file(COPY ${COPY_FILES} DESTINATION .)

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)

target_include_directories(${PROJECT_NAME} PRIVATE src)