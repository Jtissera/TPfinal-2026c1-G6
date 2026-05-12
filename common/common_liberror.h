#ifndef LIB_ERROR_H
#define LIB_ERROR_H

#include <exception>
#include <stdexcept>

class LibError: public std::exception {
    char msg_error[256];

public:
    LibError(int error_code, const char* fmt, ...) noexcept;

    virtual const char* what() const noexcept override;

    virtual ~LibError() override;
};

struct ClosedSocket: public std::runtime_error {
    ClosedSocket(): std::runtime_error("El socket fue cerrado") {}
};

#endif
