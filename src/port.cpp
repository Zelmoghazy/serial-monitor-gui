#include "port.hpp"
#include <string>
#include <vector>

uint32_t serial_platform::init(std::string port, baud_rate_e baud_rate, data_e data_bits, parity_e parity, stop_bits_e stop_bits, bool flow_ctrl)
{
    #ifdef WINDOWS
        osReader = {0};
        osWrite  = {0};
        fWaitingOnRead = FALSE;
        timeouts_ori = {0};

        // The CreateFile function opens a communications port.
        // overlapped I/O, the system may return to the caller 
        // immediately even when an operation is not finished and
        // will signal the caller when the operation completes

        hComm = CreateFile(
            port.c_str(),                                   // Port name
            GENERIC_READ | GENERIC_WRITE,                   // The requested access mode
            NULL,                                           // share mode : must be zero, ports cannot be shared
            NULL,                                           // Security : Default
            OPEN_EXISTING,                                  // For devices other than files, this parameter is usually set to OPEN_EXISTING.
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,   // open a communications resource for overlapped operation
            NULL  
        ); 

        // error openning a port
        if (hComm == INVALID_HANDLE_VALUE) {
            fprintf(stderr,"Error openning port!\n");
            return -1;
        }

        if (PurgeComm(hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR) == 0) {
            return -1;
        }//purge

        DCB dcbOri;
        bool fSuccess;

        fSuccess = GetCommState(hComm, &dcbOri);
        if (!fSuccess) {
            return -1;
        }

        DCB dcb1 = dcbOri;

        switch (baud_rate) {
            case baud_rate_e::BAUDRATE_2400:
                dcb1.BaudRate = CBR_2400;
                break;
            case baud_rate_e::BAUDRATE_9600:
                dcb1.BaudRate = CBR_9600;
                break;
            case baud_rate_e::BAUDRATE_19200:
                dcb1.BaudRate = CBR_19200;
                break;
            case baud_rate_e::BAUDRATE_57600:
                dcb1.BaudRate = CBR_19200;
                break;
            case baud_rate_e::BAUDRATE_115200:
                dcb1.BaudRate = CBR_115200;
                break;
            case baud_rate_e::BAUDRATE_128000:
                dcb1.BaudRate = CBR_128000;
                break;
            case baud_rate_e::BAUDRATE_256000:
                dcb1.BaudRate = CBR_256000;
                break;
            default:
                break;
        }


        switch (parity) {
            case parity_e::P_NONE:
                dcb1.Parity = NOPARITY;
                break;
            case parity_e::P_EVEN:
                dcb1.Parity = EVENPARITY;
                break;
            case parity_e::P_ODD:
                dcb1.Parity = ODDPARITY;
                break;
            default:
                break;          
        }

        switch (data_bits) {
            case data_e::DATA_7B:
                dcb1.ByteSize = (BYTE)7;
                break;
            case data_e::DATA_8B:
                dcb1.ByteSize = (BYTE)8;
                break;
            case data_e::DATA_9B:
                dcb1.ByteSize = (BYTE)9;
                break;
        }

        switch (stop_bits) {
            case stop_bits_e::STOP_ONE:
                dcb1.StopBits = ONESTOPBIT;
                break;
            case stop_bits_e::STOP_ONE_HALF:
                dcb1.StopBits = ONE5STOPBITS;
                break;
            case stop_bits_e::STOP_TWO:
                dcb1.StopBits = TWOSTOPBITS;
                break;
        }
        if(flow_ctrl) {
            dcb1.fOutxCtsFlow = true;                   // the CTS (clear-to-send) signal is monitored for output flow control. 
            dcb1.fOutxDsrFlow = true;                   // the DSR (data-set-ready) signal is monitored for output flow control.
            dcb1.fOutX        = true;                   // XON/XOFF flow control is used during transmission
            dcb1.fDtrControl  = DTR_CONTROL_ENABLE;     // The DTR (data-terminal-ready) flow control
            dcb1.fRtsControl  = RTS_CONTROL_ENABLE;     // The RTS (request-to-send) flow control.
        }else{
            dcb1.fOutxCtsFlow = false;
            dcb1.fOutxDsrFlow = false;
            dcb1.fOutX        = false;
            dcb1.fDtrControl  = DTR_CONTROL_DISABLE;
            dcb1.fRtsControl  = RTS_CONTROL_DISABLE;
        }

        fSuccess = SetCommState(hComm, &dcb1);

        this->delay(60);

        if (!fSuccess) {return -1;}

        osReader = {0};

        // Create the overlapped event. Must be closed before exiting
        // to avoid a handle leak
        osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if(osReader.hEvent == NULL){
            // Error creating overlapped event; abort.
            return -1;
        }

        if (!GetCommTimeouts(hComm, &timeouts_ori)) { return -1; } // Error getting time-outs.

        COMMTIMEOUTS timeouts;
        timeouts.ReadIntervalTimeout            = 20;
        timeouts.ReadTotalTimeoutMultiplier     = 15;
        timeouts.ReadTotalTimeoutConstant       = 100;
        timeouts.WriteTotalTimeoutMultiplier    = 15;
        timeouts.WriteTotalTimeoutConstant      = 100;

        if (!SetCommTimeouts(hComm, &timeouts)) { return -1;} // Error setting time-outs.

        return 0;
    #endif /* WINDOWS */


    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */

}

void serial_platform::close(void)
{
    #ifdef WINDOWS
        SetCommTimeouts(hComm, &timeouts_ori);
        CloseHandle(osReader.hEvent);
        CloseHandle(osWrite.hEvent);
        CloseHandle(hComm);                     //close comm port
        hComm = INVALID_HANDLE_VALUE;
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
}

bool serial_platform::is_open(void) 
{
    #ifdef WINDOWS
        if(hComm == INVALID_HANDLE_VALUE){
            return false;
        }else{
            return true;
        }
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
}

bool serial_platform::write_n(const char *str, size_t n) 
{
    #ifdef WINDOWS
        if (!is_open()){
            return false;
        }

        if (n < 0){
            n = 0;
        }else if(n > 1024) {
            n = 1024;
        }

        BOOL fRes;
        DWORD dwWritten;

        // Issue write.
        if (!WriteFile(hComm, str, n, &dwWritten, &osWrite)) {
            // WriteFile failed, but it isn't delayed. Report error and abort.
            if (GetLastError() != ERROR_IO_PENDING){
                // WriteFile failed, but isn't delayed. Report error and abort.
                fRes = FALSE;
            }else {
                // Write is pending.
                if (!GetOverlappedResult(hComm, &osWrite, &dwWritten, TRUE)){
                    fRes = FALSE;
                }else{
                    fRes = TRUE;// Write operation completed successfully.
                } 
            }
        }else{
            fRes = TRUE;// WriteFile completed immediately.
        } 
        return fRes; 
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
    
}

char serial_platform::read_char(bool &success, char *rx_ch) 
{
    #ifdef WINDOWS
        success = false;

        if (!is_open()){
            return 0;
        }

        DWORD dwRead;
        DWORD length = 1;
        BYTE* data   = (BYTE*)(rx_ch);

        // the creation of the overlapped read operation
        if (!fWaitingOnRead) {
            // Issue read operation.
            if (!ReadFile(hComm, data, length, &dwRead, &osReader)) {
                if (GetLastError() != ERROR_IO_PENDING) {
                    // Error in communications; report it.
                }else { 
                    fWaitingOnRead = TRUE; /*Waiting*/
                }
            }else {
                if(dwRead==length){
                    success = true;
                } 
            }//success
        }


        // detection of the completion of an overlapped read operation

        DWORD dwRes;
        if (fWaitingOnRead) {
            // wait until the object is signaled. 
            // Once the event is signaled, the operation is complete.
            dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);

            switch (dwRes)
            {
                // Read completed.
                case WAIT_OBJECT_0:
                    if (!GetOverlappedResult(hComm, &osReader, &dwRead, FALSE)) { // reports the result of the operation.
                        // Error in communications; report it. 
                    }else {
                        // Read completed successfully.
                        if(dwRead == length){
                            success = true;
                        }
                        // Reset flag so that another opertion can be issued.
                        fWaitingOnRead = FALSE;
                    }
                    break;

                case WAIT_TIMEOUT:
                    // Operation isn't complete yet. fWaitingOnRead flag isn't
                    // changed since I'll loop back around, and I don't want
                    // to issue another read until the first one finishes.
                    //
                    // This is a good time to do some background work.
                    break;

                default:
                    // Error in the WaitForSingleObject; abort.
                    // This indicates a problem with the OVERLAPPED structure's
                    // event handle.
                    break;
            }
        }
        return *rx_ch;
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
}

void serial_platform::delay(uint32_t ms)
{
    #ifdef WINDOWS
        Sleep(ms);
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
}

std::vector<std::string> serial_platform::get_available_ports() 
{
    #ifdef WINDOWS
        std::vector<std::string> ports;

        char lpTargetPath[5000];    // buffer to store the path of the COMPORTS
        bool gotPort = false;       // in case the port is not found

        for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
        {
            std::string str = "COM" + std::to_string(i); // converting to COM0, COM1, COM2
            DWORD test = QueryDosDevice(str.c_str(), lpTargetPath, 5000);

            // Test the return value and error if any
            if (test != 0) //QueryDosDevice returns zero if it didn't find an object
            {
                ports.push_back(str);
            }
            if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
            }
        }
        return ports;
    }
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */


bool serial_platform::set_flow_ctrl(bool RTS, bool DTR) 
{
    #ifdef WINDOWS
        bool r = false;
        if (is_open()) 
        {
            if (RTS) {
                if (EscapeCommFunction(hComm, SETRTS)){
                    r = true;
                } 
            }else {
                if (EscapeCommFunction(hComm, CLRRTS)){
                    r = true;
                } 
            }

            if (DTR) {
                if (EscapeCommFunction(hComm, SETDTR)){
                    r = true;
                } 
            }else {
                if (EscapeCommFunction(hComm, CLRDTR)){
                    r = true;
                } 
            }
        }
        return r;
    #endif /* WINDOWS */

    #ifdef LINUX
        // TODO: Linux Port
    #endif /* LINUX */

    #ifdef MACOS
        // TODO: MACOS Port
    #endif /* MACOS */
}
/* ------------------------------------------ */