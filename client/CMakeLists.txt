file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE CLIENT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

list(APPEND CLIENT_SOURCES
    "${CMAKE_SOURCE_DIR}/editor/map/mapSerializer.cpp"
    "${CMAKE_SOURCE_DIR}/editor/map/mapData.cpp"
)

target_sources(taller_client
    PRIVATE
        ${CLIENT_SOURCES}
        ${CLIENT_HEADERS}
)