target_sources(taller_common
    PRIVATE
    # cpp files
    version.cpp

    common_protocol.cpp
    common_liberror.cpp
    common_resolver.cpp
    common_resolverror.cpp
    common_socket.cpp

    PUBLIC
    # h files
    version.h

    common_command.h
    common_message.h
    common_protocol.h
    common_liberror.h
    common_resolver.h
    common_resolverror.h
    common_socket.h
)