#ifndef PORT_H_
#define PORT_H_

#include <cstdint>
#include <vector>
#include <string>

#include "utils.hpp"

#define READ_TIMEOUT 500

// Compile time OS check to enable code specific to each platform 

/* --------------------------------------------------------------------------------- */
// Windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // define something for Windows (32-bit and 64-bit, this part is common)
    #define WINDOWS

    // #include <minwindef.h>
    #include <windows.h>
    // #include <winnt.h>

    #ifdef _WIN64
        // define something for Windows (64-bit only)
    #else
        // define something for Windows (32-bit only)
#endif

// Apple
#elif __APPLE__
    #include <TargetConditionals.h>

    #if TARGET_IPHONE_SIMULATOR
    // iOS, tvOS, or watchOS Simulator
    #elif TARGET_OS_MACCATALYST
    // Mac's Catalyst (ports iOS API into Mac, like UIKit).
    #elif TARGET_OS_IPHONE
    // iOS, tvOS, or watchOS device
    #elif TARGET_OS_MAC
        #define MACOS
    // Other kinds of Apple platforms
#else
    #error "Unknown Apple platform"
#endif

// Android
#elif __ANDROID__
    // Below __linux__ check should be enough to handle Android,
    // but something may be unique to Android.

// LINUX    
#elif __linux__
    #define LINUX

// all unices not caught above
#elif __unix__  
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown compiler"
#endif

/* ------------------------------------------------------------------ */

struct serial_platform
{
    #ifdef WINDOWS
        HANDLE          hComm;              // Communication handle
        OVERLAPPED      osReader;
        OVERLAPPED      osWrite;
        BOOL            fWaitingOnRead;
        COMMTIMEOUTS    timeouts_ori;
    #endif
    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */

    uint32_t init(std::string port, baud_rate_e baud_rate, data_e data_bits, parity_e parity, stop_bits_e stop_bits, bool flow_ctrl);
    void close(void);
    bool is_open(void);
    bool write_n(const char *str, size_t n);
    char read_char(bool &success, char *ch);
    bool set_flow_ctrl(bool RTS, bool DTR);
    void delay(uint32_t ms);
    std::vector<std::string> get_available_ports();

};

#endif

