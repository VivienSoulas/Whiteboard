#include "http/cgi_handler.hpp"
#include "http/http_response_factory.hpp"
#include "http/http_status.hpp"
#include "path/path_utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>
#include "string/string_utils.hpp"
#include "string/string_parser.hpp"
#include <limits.h>
#include <iostream>
#include <poll.h>
#include <sys/resource.h>
#include "logger.hpp"
#include "io/utils.hpp"
#include "webserv.hpp"

namespace cgi_handler
{
    std::string handle(const ServerConfig &server, const LocationConfig &location, const HttpRequest &req)
    {
        std::string root = location.root.empty() ? server.root : location.root;
        std::string effective_request_path = req.path;
        if (!location.root.empty())
        {
            if (effective_request_path.find(location.path) == 0 && location.path != "/")
                effective_request_path.erase(0, location.path.length());
        }

        std::string file_path = path_utils::resolveFilePath(root, effective_request_path);
        if (file_path.empty())
            return HttpResponseFactory::buildError(req, &server, HttpStatus::NOT_FOUND, true).serialize();

        file_path = path_utils::normalizeAndMakeAbsolute("", file_path);

        if (access(file_path.c_str(), R_OK) != 0)
        {
            DEBUG_LOG("403 Forbidden: access(" << file_path << ", R_OK) failed.");
            return HttpResponseFactory::buildError(req, &server, HttpStatus::FORBIDDEN, true).serialize();
        }
        DEBUG_LOG("Access R_OK check passed for " << file_path);

        int pipe_in[2];
        int pipe_out[2];
        if (pipe(pipe_in) < 0)
            return HttpResponseFactory::buildError(req, &server, HttpStatus::INTERNAL_SERVER_ERROR, true).serialize();
        if (pipe(pipe_out) < 0)
        {
            close(pipe_in[0]);
            close(pipe_in[1]);
            return HttpResponseFactory::buildError(req, &server, HttpStatus::INTERNAL_SERVER_ERROR, true).serialize();
        }

        // CGI timeout is now handled by the parent's poll loop (POLL_TIMEOUT);
        // per-request timeout is enforced by monitoring pipe read timeout
        pid_t pid = fork();
        if (pid < 0)
        {
            // Both pipes are properly closed on fork() failure
            close(pipe_in[0]); close(pipe_in[1]);
            close(pipe_out[0]); close(pipe_out[1]);
            return HttpResponseFactory::buildError(req, &server, HttpStatus::INTERNAL_SERVER_ERROR, true).serialize();
        }

        if (pid == 0) // Child
        {
            close(pipe_in[1]);
            close(pipe_out[0]);
            dup2(pipe_in[0], STDIN_FILENO);
            dup2(pipe_out[1], STDOUT_FILENO);
            close(pipe_in[0]);
            close(pipe_out[1]);

            // Reset all signals to default
            for (int sig = 1; sig < NSIG; sig++)
                signal(sig, SIG_DFL);

            // Close all inherited file descriptors using getrlimit
            struct rlimit rl;
            if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
            {
                for (int i = 3; i < (int)rl.rlim_max; i++)
                    close(i);
            }
            else
            {
                // Fallback if getrlimit fails
                for (int i = 3; i < 256; i++)
                    close(i);
            }

            std::vector<std::string> envs;
            envs.push_back("REQUEST_METHOD=" + httpMethodToString(req.method));
            envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
            envs.push_back("SERVER_SOFTWARE=Webserv/1.0");
            envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
            envs.push_back("REDIRECT_STATUS=200");
            envs.push_back("REMOTE_ADDR=127.0.0.1");
            envs.push_back("SERVER_NAME=" + server.server_name);
            // Default port if multiple listed
            envs.push_back("SERVER_PORT=" + (server.listen.empty() ? "80" : server.listen[0].second));
            
            std::string script_name = req.path;
            size_t q_mark = script_name.find('?');
            if (q_mark != std::string::npos) script_name = script_name.substr(0, q_mark);
            
            envs.push_back("SCRIPT_NAME=");
            envs.push_back("PATH_INFO=" + script_name);
            envs.push_back("PATH_TRANSLATED=" + file_path);
            
            envs.push_back("QUERY_STRING=" + req.query);

            std::stringstream ss;
            ss << req.body.size();
            envs.push_back("CONTENT_LENGTH=" + ss.str());
            
            HttpRequest::HeaderMap::const_iterator ct_it = req.headers.find("content-type");
            if (ct_it != req.headers.end()) {
                envs.push_back("CONTENT_TYPE=" + ct_it->second);
            } else {
                envs.push_back("CONTENT_TYPE=");
            }

            for (HttpRequest::HeaderMap::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it)
            {
                std::string key = "HTTP_";
                std::string hname = it->first;
                for (size_t i = 0; i < hname.size(); ++i) {
                    if (hname[i] == '-') key += '_';
                    else key += std::toupper(hname[i]);
                }
                if (hname != "content-type" && hname != "content-length")
                    envs.push_back(key + "=" + it->second);
            }

            char **envp = new char*[envs.size() + 1];
            for (size_t i = 0; i < envs.size(); ++i) {
                // strdup() allocates memory for environment strings, but this leak is not practical
                // because the child process's entire memory image is replaced by execve()
                envp[i] = strdup(envs[i].c_str());
            }
            envp[envs.size()] = NULL;

            std::string bin_path;
            if (string_utils::endsWith(file_path, ".php"))
                bin_path = "/usr/bin/php-cgi";
            else if (string_utils::endsWith(file_path, ".py"))
                bin_path = "/usr/bin/python3";
            else if (string_utils::endsWith(file_path, ".bla"))
            {
                char cwd_buf[PATH_MAX] = {};
                if (getcwd(cwd_buf, sizeof(cwd_buf)))
                    bin_path = std::string(cwd_buf) + "/assets/cgi_tester";
                else
                    bin_path = "./assets/cgi_tester";
            }
            else
                bin_path = file_path;

            // Note: argv elements are strdup'd but memory is not freed because child process
            // image is replaced by execve() - all process memory is reclaimed on exec
            char *argv[] = { strdup(bin_path.c_str()), strdup(file_path.c_str()), NULL };
            
            std::string dir = file_path;
            size_t dir_end = dir.find_last_of('/');
            if (dir_end != std::string::npos)
            {
                dir = dir.substr(0, dir_end);
                if (chdir(dir.c_str()) != 0)
                {
                    DEBUG_LOG("chdir failed: " << dir);
                    exit(1);
                }
            }

            DEBUG_LOG("Executing " << bin_path << " with argv[1]: " << file_path);
            execve(bin_path.c_str(), argv, envp);
            
            DEBUG_LOG("execve failed!");
            exit(1);
        }
        else // Parent
        {
            close(pipe_in[0]);
            close(pipe_out[1]);
            
            makeNonBlocking(pipe_in[1]);
            makeNonBlocking(pipe_out[0]);

            std::string cgi_output;
            const std::string &body = req.body;
            size_t body_written = 0;
            bool pipe_in_closed = false;
            
            struct pollfd fds[2];
            fds[0].fd = pipe_out[0];
            fds[0].events = POLLIN;
            fds[1].fd = pipe_in[1];
            fds[1].events = POLLOUT;

            while (!pipe_in_closed || fds[0].fd != -1)
            {
                int nfds = 0;
                if (fds[0].fd != -1) nfds = 1;
                if (!pipe_in_closed) nfds = 2;

                int ret = poll(fds, nfds, POLL_TIMEOUT);
                if (ret <= 0) break; // Timeout or error

                // Read from CGI
                if (fds[0].fd != -1 && (fds[0].revents & (POLLIN | POLLHUP | POLLERR)))
                {
                    char buf[8192];
                    ssize_t n = ::read(pipe_out[0], buf, sizeof(buf));
                    if (n > 0)
                        cgi_output.append(buf, n);
                    else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
                    {
                        close(pipe_out[0]);
                        fds[0].fd = -1;
                    }
                }

                // Write to CGI
                if (!pipe_in_closed && (fds[1].revents & (POLLOUT | POLLERR)))
                {
                    if (body_written < body.size())
                    {
                        ssize_t n = ::write(pipe_in[1], body.data() + body_written, body.size() - body_written);
                        if (n > 0)
                        {
                            body_written += n;
                            DEBUG_LOG("CGI pipe write: wrote " << n << " bytes");
                        }
                        else if (n == 0)
                        {
                            DEBUG_LOG("CGI pipe write: returned 0 bytes (error)");
                            close(pipe_in[1]);
                            pipe_in_closed = true;
                        }
                        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                        {
                            DEBUG_LOG("CGI pipe write: EAGAIN/EWOULDBLOCK, retrying");
                            // Don't close, will retry on next poll
                        }
                        else if (n < 0)
                        {
                            DEBUG_LOG("CGI pipe write: error " << strerror(errno));
                            close(pipe_in[1]);
                            pipe_in_closed = true;
                        }
                    }
                    else
                    {
                        close(pipe_in[1]);
                        pipe_in_closed = true;
                    }
                }
            }
            if (fds[0].fd != -1) close(pipe_out[0]);
            if (!pipe_in_closed) close(pipe_in[1]);

            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                DEBUG_LOG("Child exited with non-zero status: " << WEXITSTATUS(status));
                if (cgi_output.empty())
                    return HttpResponseFactory::buildError(req, &server, HttpStatus::INTERNAL_SERVER_ERROR, true).serialize();
            }

            size_t header_end = cgi_output.find("\r\n\r\n");
            std::string cgi_headers;
            std::string cgi_body;
            if (header_end == std::string::npos) {
                header_end = cgi_output.find("\n\n");
                if (header_end != std::string::npos) {
                    cgi_headers = cgi_output.substr(0, header_end);
                    cgi_body = cgi_output.substr(header_end + 2);
                } else {
                    cgi_body = cgi_output;
                }
            } else {
                cgi_headers = cgi_output.substr(0, header_end);
                cgi_body = cgi_output.substr(header_end + 4);
            }

            HttpResponse res;
            res.setStatus(HttpStatus::OK);
            res.setShouldClose(true);

            std::string content_type_str = "text/html";
            std::vector<std::string> header_lines = string_parser::split(cgi_headers, '\n');
            for (size_t i = 0; i < header_lines.size(); ++i) {
                std::string line = string_parser::trim(header_lines[i]);
                while (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
                
                std::pair<std::string, std::string> kv = string_parser::splitOnce(line, ':');
                std::string h_name = string_parser::toLower(string_parser::trim(kv.first));
                std::string h_val = string_parser::trim(kv.second);
                
                if (h_name == "content-type") {
                    content_type_str = h_val;
                } else if (h_name == "status") {
                    res.setStatus(std::atoi(h_val.c_str()));
                } else if (!h_name.empty()) {
                    res.setHeader(kv.first, h_val);
                }
            }
            
            res.setContentType(content_type_str);
            res.setBody(cgi_body);
            return res.serialize();
        }
    }
}
