#ifndef MAIN_HPP
#define MAIN_HPP

#include <csignal>
#include <string>

extern volatile sig_atomic_t g_running;

int main(int argc, char *argv[]);
void print_usage(const std::string &prog_name);

#endif
