file(GLOB_RECURSE _COMMON_DEBUG_SRC_LIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cc")
file(STRINGS "${TOP_DIR}/version.txt" VERSION)
add_definitions(-DVERSION=\"${VERSION}\")
set_property(SOURCE ${_COMMON_DEBUG_SRC_LIST} PROPERTY COMPILE_DEFINITIONS
            SUBMODULE_ID=mindspore::SubModuleId::SM_DEBUG)
add_library(_mindspore_common_debug_obj OBJECT ${_COMMON_DEBUG_SRC_LIST})
