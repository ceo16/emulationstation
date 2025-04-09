#ifndef ES_APP_SERVICES_HTTPSERVERCALLBACK_H
#define ES_APP_SERVICES_HTTPSERVERCALLBACK_H

#include <string>
#include <functional>

namespace HttpServerCallback {
    std::function<void(const std::string&)> SetStateCallback;
}

#endif // ES_APP_SERVICES_HTTPSERVERCALLBACK_H