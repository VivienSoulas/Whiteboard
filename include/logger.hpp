#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>

#ifndef DEBUG
# define DEBUG 0
#endif

// DEBUG_LOG prints only if DEBUG is 1. Usage: DEBUG_LOG("Something: " << val);
#define DEBUG_LOG(x) \
    do { \
        if (DEBUG) { \
            std::cout << "[DEBUG] " << x << std::endl; \
        } \
    } while (0)

#endif
