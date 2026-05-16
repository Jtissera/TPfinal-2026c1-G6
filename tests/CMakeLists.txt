target_sources(taller_tests
    PRIVATE
    version_test.cpp
    basic_protocol_test.cpp
    mapSerializer_test.cpp
    # Fuentes del editor necesarias para compilar el test
    ../editor/map/mapData.cpp
    ../editor/map/mapSerializer.cpp
    )
 