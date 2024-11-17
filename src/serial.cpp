#include "serial.hpp"

serial_handle::serial_handle(std::string port, baud_rate_e baud_rate)
{
    set_port_name(port);
    set_baud_rate(baud_rate);
    set_data_bits(data_e::DATA_8B);
    set_parity(parity_e::P_NONE);
    set_stop_bits(stop_bits_e::STOP_ONE);
    flow_ctrl = false;
}

serial_handle::serial_handle(std::string port, baud_rate_e baud_rate, data_e data_bits, parity_e parity, stop_bits_e stop_bits, bool flow_ctrl)
{
    set_port_name(port);
    set_baud_rate(baud_rate);
    set_data_bits(data_bits);
    set_parity(parity);
    set_stop_bits(stop_bits);
    this->flow_ctrl = flow_ctrl;
}

serial_handle::~serial_handle()
{
    close_connection();
}


void serial_handle::set_port_name(std::string port)
{
    this->port = "\\\\.\\\\" + port;
}

std::string serial_handle::get_port_name()
{
    return this->port;
}

void serial_handle::set_data_bits(data_e nbits)
{
    this->data_bits = nbits;
}

data_e serial_handle::get_data_bits()
{
    return this->data_bits;
}

void serial_handle::set_parity(parity_e p)
{
    this->parity = p;
}

parity_e serial_handle::get_parity()
{
    return this->parity;
}

void serial_handle::set_stop_bits(stop_bits_e nbits)
{
    this->stop_bits = nbits;
}

stop_bits_e serial_handle::get_stop_bits(void)
{
    return this->stop_bits;
}

std::vector<std::string> serial_handle::get_available_ports() {
    return this->platform.get_available_ports();
}

void serial_handle::set_baud_rate(baud_rate_e baud)
{
    this->baud_rate = baud;
}

baud_rate_e serial_handle::get_baud_rate()
{
    return this->baud_rate;
}

uint32_t serial_handle::open_connection(void)
{
    return this->platform.init(port, baud_rate, data_bits, parity, stop_bits, flow_ctrl);
}

bool serial_handle::is_connection_open() 
{
    return platform.is_open();
}

void serial_handle::close_connection(void)
{
    if (is_connection_open())
	{
        platform.close();
	}
}

bool serial_handle::write_n(const char *str, size_t n)
{
    return platform.write_n(str, n);
}

bool serial_handle::write(const char* str)
{
    if (is_connection_open())
    {
        return false;
    }
    size_t n = strlen(str);
    return platform.write_n(str, n);
}

bool serial_handle::write_char(const char ch)
{
    char str[2];
    str[0] = ch;
    str[1] = '\0';
    return serial_handle::write(str);
}

char serial_handle::read_char(bool &success) 
{
    return platform.read_char(success, &rx_ch);
}

bool serial_handle::set_flow_ctrl(bool RTS, bool DTR)
{
    return platform.set_flow_ctrl(RTS,DTR);
}

void serial_handle::delay(uint32_t ms)
{
    platform.delay(ms);
}