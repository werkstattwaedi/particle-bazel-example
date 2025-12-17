#include "lib/app_core/app_core.h"

#include <climits>

namespace app {

pw::Result<int> Add(int a, int b) {
    // Check for overflow
    if (b > 0 && a > (INT_MAX - b)) {
        return pw::Status::OutOfRange();
    }
    if (b < 0 && a < (INT_MIN - b)) {
        return pw::Status::OutOfRange();
    }
    return a + b;
}

pw::Status Initialize() {
    // Placeholder for initialization
    return pw::OkStatus();
}

}  // namespace app
