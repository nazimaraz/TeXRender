find_package(tinyxml2 CONFIG QUIET)
if (NOT tinyxml2_FOUND)
    set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    texrender_add_submodule_dependency(
            tinyxml2
            TINYXML2
            "${CMAKE_CURRENT_LIST_DIR}/../../third_party/tinyxml2"
            CMakeLists.txt
    )
endif ()

if (TARGET tinyxml2::tinyxml2)
    set(TinyXML2_FOUND TRUE)
else ()
    set(TinyXML2_FOUND FALSE)
endif ()
