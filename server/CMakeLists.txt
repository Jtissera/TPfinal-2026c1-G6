target_sources(taller_server
    PRIVATE

    # cpp files
    main.cpp

    server_server.cpp
    server_sender.cpp
    server_receiver.cpp
    server_monitorQueues.cpp
    server_gameLoop.cpp
    server_clientHandler.cpp
    server_acceptor.cpp

    PUBLIC

    # h files
    server_server.h
    server_sender.h
    server_receiver.h
    server_thread.h
    server_queue.h
    server_monitorQueues.h
    server_gameLoop.h
    server_clientHandler.h
    server_acceptor.h
)