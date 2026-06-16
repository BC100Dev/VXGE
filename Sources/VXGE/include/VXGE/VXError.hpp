#ifndef VXGE_ERROR_HPP
#define VXGE_ERROR_HPP

#include <string>
#include <stdexcept>

namespace VX {

    class VXError {
    public:
        explicit VXError() = default;
        explicit VXError(const std::string& msg) : m_message(msg) {}

        const std::string& what() const { return m_message;}
        bool Empty() const { return m_message.empty(); };

    private:
        std::string m_message;
    };

    class VXRuntimeError : std::runtime_error {
    public:
        explicit VXRuntimeError(const std::string& msg) : std::runtime_error(msg) {}
    };

    void SetLastError(const VXError& error);

    VXError GetLastError();
}

#endif //VXGE_ERROR_HPP
