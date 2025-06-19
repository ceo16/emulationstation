#pragma once

#include <string>
#include <vector>
#include <functional>
#include "Window.h"

enum class AuthMethod
{
	WebView,
	Imported,
	ApiKey
};

struct AuthStatus
{
	bool isLoggedIn = false;
	std::string userName;
	std::string errorMessage;
};

class IStoreAuth
{
public:
	virtual ~IStoreAuth() {}

	virtual std::string getStoreName() const = 0;
	virtual std::vector<AuthMethod> getSupportedAuthMethods() const = 0;
	virtual void authenticate(Window* window, AuthMethod method, const std::function<void(const AuthStatus&)>& on_complete) = 0;
	virtual void logout() = 0;
	virtual AuthStatus getAuthStatus() = 0;
	virtual void refreshToken(const std::function<void(bool success)>& on_complete) = 0;
	virtual bool loadCredentials() = 0;
};