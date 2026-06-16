#include <VXGE/VXError.hpp>

namespace VX {

    static VXError v_lastError;

    void SetLastError(const VXError& error) {
        v_lastError = error;
    }

    VXError GetLastError() {
        return v_lastError;
    }
}
