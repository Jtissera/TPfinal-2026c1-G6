file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE SERVER_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

list(APPEND SERVER_SOURCES
    "${CMAKE_SOURCE_DIR}/editor/map/mapSerializer.cpp"
    "${CMAKE_SOURCE_DIR}/editor/map/mapData.cpp"
)

target_sources(taller_server_lib
    PRIVATE
        ${SERVER_SOURCES}
        ${SERVER_HEADERS}
)
target_sources(taller_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)