#include "services/HttpServerCallback.h"

namespace HttpServerCallback
{
    std::function<void(const std::string&)> setStateCallback;
}