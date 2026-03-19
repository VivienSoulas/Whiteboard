#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include "signal/signal_handler.hpp"
#include "main.hpp"
#include "logger.hpp"

void handleSigchld(int signal)
{
	(void)signal;
	// Reap all dead child processes to prevent zombies
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0)
	{
		DEBUG_LOG("Child process reaped");
	}
}

void signalHandler(int signal)
{
	(void)signal;
	DEBUG_LOG("Signal caught, terminating server now");
	g_running = 0;
}
