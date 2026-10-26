#include "serial.h"

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#endif

SerialPort::SerialPort() = default;

SerialPort::~SerialPort() { close(); }

#ifdef _WIN32

static std::string WideToUtf8(const wchar_t* w) {
    if (!w) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
    return s;
}

std::vector<std::string> SerialPort::listPorts() {
    std::vector<std::string> ports;
    HDEVINFO info = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (info == INVALID_HANDLE_VALUE) return ports;
    SP_DEVINFO_DATA data{};
    data.cbSize = sizeof(data);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(info, i, &data); ++i) {
        wchar_t buf[256];
        if (!SetupDiGetDeviceRegistryPropertyW(info, &data, SPDRP_FRIENDLYNAME,
                                               nullptr, (PBYTE)buf, sizeof(buf), nullptr))
            continue;
        std::string name = WideToUtf8(buf);
        auto open = name.find("(COM");
        if (open == std::string::npos) continue;
        auto close = name.find(')', open);
        if (close == std::string::npos) continue;
        ports.push_back(name.substr(open + 1, close - open - 1));
    }
    SetupDiDestroyDeviceInfoList(info);
    return ports;
}

bool SerialPort::open(const std::string& name, int baud) {
    close();
    std::string path = name;
    if (path.rfind("\\\\.\\", 0) != 0)
        path = "\\\\.\\" + path;
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        return false;
    }
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = ODDPARITY;
    dcb.fParity = TRUE;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        return false;
    }
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutConstant = 0;
    to.ReadTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant = 200;
    SetCommTimeouts(h, &to);

    handle_ = h;
    name_ = name;
    return true;
}

void SerialPort::close() {
    if (handle_) {
        CloseHandle((HANDLE)handle_);
        handle_ = nullptr;
    }
    name_.clear();
}

bool SerialPort::isOpen() const { return handle_ != nullptr; }

void SerialPort::write(const std::vector<uint8_t>& bytes) {
    if (!handle_ || bytes.empty()) return;
    DWORD written = 0;
    WriteFile((HANDLE)handle_, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
}

std::string SerialPort::readAvailable() {
    if (!handle_) return {};
    DWORD avail = 0;
    if (!ClearCommError((HANDLE)handle_, nullptr, nullptr)) return {};
    COMSTAT st{};
    ClearCommError((HANDLE)handle_, nullptr, &st);
    avail = st.cbInQue;
    if (avail == 0) return {};
    std::string buf(avail, '\0');
    DWORD got = 0;
    if (!ReadFile((HANDLE)handle_, &buf[0], avail, &got, nullptr)) return {};
    buf.resize(got);
    return buf;
}

#else

std::vector<std::string> SerialPort::listPorts() {
    std::vector<std::string> ports;
    DIR* d = opendir("/dev");
    if (!d) return ports;
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n.rfind("cu.", 0) == 0 || n.rfind("tty.usb", 0) == 0 || n.rfind("ttyUSB", 0) == 0)
            ports.push_back(std::string("/dev/") + n);
    }
    closedir(d);
    return ports;
}

bool SerialPort::open(const std::string& name, int baud) {
    close();
    int fd = ::open(name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return false;
    termios tio{};
    if (tcgetattr(fd, &tio) != 0) {
        ::close(fd);
        return false;
    }
    cfmakeraw(&tio);
    speed_t sp = B38400;
    if (baud == 115200) sp = B115200;
    else if (baud == 57600) sp = B57600;
    else if (baud == 9600) sp = B9600;
    cfsetispeed(&tio, sp);
    cfsetospeed(&tio, sp);
    tio.c_cflag |= (CLOCAL | CREAD | PARENB | PARODD);
    tio.c_cflag &= ~CRTSCTS;
    tcsetattr(fd, TCSANOW, &tio);
    fd_ = fd;
    name_ = name;
    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    name_.clear();
}

bool SerialPort::isOpen() const { return fd_ >= 0; }

void SerialPort::write(const std::vector<uint8_t>& bytes) {
    if (fd_ < 0 || bytes.empty()) return;
    ::write(fd_, bytes.data(), bytes.size());
}

std::string SerialPort::readAvailable() {
    if (fd_ < 0) return {};
    char tmp[4096];
    ssize_t n = ::read(fd_, tmp, sizeof(tmp));
    if (n <= 0) return {};
    return std::string(tmp, tmp + n);
}

#endif
