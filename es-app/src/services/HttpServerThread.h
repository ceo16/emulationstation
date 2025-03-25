#pragma once

#include "Window.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace httplib
{
	class Server;
}

class HttpServerThread
{
public:
	HttpServerThread(Window * window);
	virtual ~HttpServerThread();

        void queueTask(std::function<void()> task);
        void addEpicCallbackEndpoint(int port);

	static std::string getMimeType(const std::string &path);

private:
	Window*			mWindow;
	bool			mRunning;
	bool			mFirstRun;
	std::thread*	mThread;

	httplib::Server* mHttpServer;

	void run();

        std::queue<std::function<void()>> mTaskQueue;
        std::mutex mQueueMutex;
        std::condition_variable mQueueCondition;
        std::function<void(const std::string&)> mEpicLoginCallback;

};


