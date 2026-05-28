target_sources(taller_tests
    PRIVATE
    version_test.cpp
    basic_protocol_test.cpp
    mapSerializer_test.cpp
    ../editor/map/mapData.cpp
    ../editor/map/mapSerializer.cpp
    lobby_test.cpp
    collision_test.cpp
    inventory_test.cpp
    combatSystem_test.cpp

)
