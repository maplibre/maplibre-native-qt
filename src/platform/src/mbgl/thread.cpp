#include <mbgl/platform/thread.hpp>
#include <mbgl/util/platform.hpp>

#include <string>

namespace mbgl {
namespace platform {

std::string getCurrentThreadName() {
    return "unknown";
}

void setCurrentThreadName(const std::string&) {}

void makeThreadLowPriority() {}

void setCurrentThreadPriority(double) {}

void attachThread() {}

void detachThread() {}

} // namespace platform
} // namespace mbgl
