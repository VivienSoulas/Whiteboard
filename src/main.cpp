#include "main.hpp"
#include "webserv.hpp"
#include "config/config_path.hpp"
#include "signal/signal_handler.hpp"

volatile sig_atomic_t g_running = 1;

int main(int argc, char *argv[])
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	if (argc > 2)
	{
		std::string prog_name = argv[0];
		print_usage(prog_name);
		return -1;
	}

	std::string config_path;

	if (argc == 1)
		config_path = DEFAULT_CONFIG_PATH;
	else
		config_path = argv[1];

	try
	{
		Webserv webserv(config_path);
		while (g_running)
		{
			webserv.accept();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

void print_usage(const std::string &prog_name)
{
	std::cout << "Error: Invalid usage\n\tUsage: " << prog_name << " <config_path>" << std::endl;
}
