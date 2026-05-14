target_sources(taller_editor
    PRIVATE
        main.cpp
        mainWindow.cpp
        mainWindow.h
        map/tile.h
        map/mapData.h
        map/mapData.cpp
        map/mapSerializer.h
        map/mapSerializer.cpp
        widgets/mapCanvas.h
        widgets/mapCanvas.cpp
        widgets/tilePalette.h
        widgets/tilePalette.cpp
)

# Necesario para que Qt encuentre los headers del proyecto raiz
target_include_directories(taller_editor
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)
