#include <csignal>
#include <iostream>
#include "signal/signal_handler.hpp"
#include "main.hpp"
#include "logger.hpp"

void signalHandler(int signal)
{
	(void)signal;
	DEBUG_LOG("Signal caught, terminating server now");
	g_running = 0;
}
