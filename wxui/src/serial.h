#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// AI: open() uses 38400 baud, 8 data bits, odd parity, 1 stop bit, no flow control.
// No parity, or even parity, never finishes the version handshake.
class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    static std::vector<std::string> listPorts();

    bool open(const std::string& name, int baud = 38400);
    void close();
    bool isOpen() const;
    std::string name() const { return name_; }

    void write(const std::vector<uint8_t>& bytes);

    // Drain pending RX (call from UI thread).
    std::string readAvailable();

private:
    std::string name_;
#ifdef _WIN32
    void* handle_ = nullptr; // HANDLE
#else
    int fd_ = -1;
#endif
};
