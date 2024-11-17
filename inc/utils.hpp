#ifndef UTIL_H_
#define UTIL_H_

// Macro generators for enum values and strings
#define ENUM_GEN(ENUM)     ENUM,
#define STRING_GEN(STRING) #STRING,

// Parity enum
#define LIST_PARITY(PARITY)    \
    PARITY(P_NONE)             \
    PARITY(P_EVEN)             \
    PARITY(P_ODD)              \
    PARITY(P_COUNT)

enum class parity_e {
    LIST_PARITY(ENUM_GEN)
};

// Stop bits enum
#define LIST_STOP_BITS(STOP)    \
    STOP(STOP_ONE)              \
    STOP(STOP_ONE_HALF)         \
    STOP(STOP_TWO)              \
    STOP(STOP_COUNT)              

enum class stop_bits_e {
    LIST_STOP_BITS(ENUM_GEN)
};

// Data bits enum
#define LIST_DATA_BITS(DATA)    \
    DATA(DATA_7B)               \
    DATA(DATA_8B)               \
    DATA(DATA_9B)               \
    DATA(DATA_COUNT)               

enum class data_e {
    LIST_DATA_BITS(ENUM_GEN)
};

// Baud rate enum
#define LIST_BAUD_RATE(BAUD)     \
    BAUD(BAUDRATE_2400)          \
    BAUD(BAUDRATE_9600)          \
    BAUD(BAUDRATE_19200)         \
    BAUD(BAUDRATE_57600)         \
    BAUD(BAUDRATE_115200)        \
    BAUD(BAUDRATE_128000)        \
    BAUD(BAUDRATE_256000)        \
    BAUD(BAUDRATE_COUNT)    

enum class baud_rate_e {
    LIST_BAUD_RATE(ENUM_GEN)
};

// Declare string arrays as extern
extern const char* PARITY_STRINGS[];
extern const char* STOP_BITS_STRINGS[];
extern const char* DATA_BITS_STRINGS[];
extern const char* BAUD_RATE_STRINGS[];

template<typename T>
const char* enumToString(T value) {
    return nullptr;  // Default case
}

// Declare template specializations
template<> const char* enumToString(parity_e value);
template<> const char* enumToString(stop_bits_e value);
template<> const char* enumToString(data_e value);
template<> const char* enumToString(baud_rate_e value);

#endif