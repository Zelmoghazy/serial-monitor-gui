#include "utils.hpp"

const char* PARITY_STRINGS[] = {
    LIST_PARITY(STRING_GEN)
};

const char* STOP_BITS_STRINGS[] = {
    LIST_STOP_BITS(STRING_GEN)
};

const char* DATA_BITS_STRINGS[] = {
    LIST_DATA_BITS(STRING_GEN)
};

const char* BAUD_RATE_STRINGS[] = {
    LIST_BAUD_RATE(STRING_GEN)
};

// Template specializations for each enum type
template<>
const char* enumToString(parity_e value) {
    return PARITY_STRINGS[static_cast<int>(value)];
}

template<>
const char* enumToString(stop_bits_e value) {
    return STOP_BITS_STRINGS[static_cast<int>(value)];
}

template<>
const char* enumToString(data_e value) {
    return DATA_BITS_STRINGS[static_cast<int>(value)];
}

template<>
const char* enumToString(baud_rate_e value) {
    return BAUD_RATE_STRINGS[static_cast<int>(value)];
}