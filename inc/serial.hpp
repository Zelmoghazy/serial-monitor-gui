#ifndef SERIAL_H_
#define SERIAL_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "port.hpp"

struct serial_handle
{
    std::string         port;
    baud_rate_e         baud_rate;
    data_e              data_bits;
    parity_e            parity;
    stop_bits_e         stop_bits;
    char                rx_ch;
    bool                flow_ctrl;
    serial_platform     platform;

    serial_handle(std::string port, baud_rate_e baud_rate);
    serial_handle(std::string port, baud_rate_e baud_rate, data_e data_bits, parity_e parity, stop_bits_e stop_bits, bool flow_ctrl);
    ~serial_handle();

    uint32_t open_connection(void);
    void close_connection(void);
    char read_char(bool &success);
    bool write_char(const char ch);
    bool write(const char* str);
    bool write_n(const char *str, size_t size);
    bool set_flow_ctrl(bool RTS=false, bool DTR=false);
    bool is_connection_open(void);
    void set_port_name(std::string port);
    std::string get_port_name();
    void set_baud_rate(baud_rate_e baud);
    baud_rate_e get_baud_rate(void);
    void set_data_bits(data_e nbits);
	data_e get_data_bits();
	void set_parity(parity_e p);
	parity_e get_parity();
	void set_stop_bits(stop_bits_e nbits);
	stop_bits_e get_stop_bits(void);
    std::vector<std::string> get_available_ports();

    void delay(uint32_t ms);
};

#endif