NAME = webserv

SRCS = main webserv router config/server/server_config config/config_file config/config \
       config/location/location_config config/config_parser string/string_parser path/path_utils \
	   http/multipart/multipart_parser http/http_method io/connection io/socket \
	   http/http_parser http/http_response http/http_response_factory signal/signal_handler \
	   http/static_file_handler http/upload_handler http/cgi_handler string/html_utils \
	   string/string_utils path/dir_utils io/utils io/connection_manager io/ssl_context io/tls_socket io/tls_connection

SRC_DIR = src
INC_DIR = include
OUT_DIR = out
OBJ_DIR = $(OUT_DIR)/obj
DEP_DIR = $(OUT_DIR)/dep

OUT = $(OUT_DIR)/$(NAME)

CXX = c++
CXXFLAGS = -std=c++11 -I$(INC_DIR)
DEPFLAGS = -MMD -MP -MF $(DEP_DIR)/$*.d

ifneq ($(STRICT), 0)
	CXXFLAGS += -Wall -Wextra -Werror
endif

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -DDEBUG=1
else
	CXXFLAGS += -O2
endif

MAKEFLAGS += --no-print-directory

OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(SRCS)))
DEPS = $(addprefix $(DEP_DIR)/, $(addsuffix .d, $(SRCS)))
DIRS = $(sort $(dir $(OBJS)) $(dir $(DEPS)))

GREEN = \033[0;32m
RED = \033[0;31m
RESET = \033[0m

.PHONY: all clean fclean re unchecked debug run help

all: $(OUT)

unchecked:
	@$(MAKE) all STRICT=0

debug:
	@$(MAKE) all DEBUG=1

run: $(OUT)
	@./$(OUT)

test: $(OUT)
	./assets/tester http://localhost:8082

$(NAME): $(OUT)

$(OUT): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(OUT) -lssl -lcrypto
	@printf "$(NAME): $(GREEN)compiled executable to $(OUT)$(RESET)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(DIRS)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
	@printf "$(NAME): $(GREEN)compiled $< to $@$(RESET)\n"

$(DIRS):
	@mkdir -p $@
	@printf "$(NAME): $(GREEN)created directory $@$(RESET)\n"

clean:
	@rm -rf $(OBJ_DIR)
	@printf "$(NAME): $(GREEN)$(OBJ_DIR) has been cleaned up$(RESET)\n"

fclean:
	@rm -rf $(OUT_DIR)
	@printf "$(NAME): $(GREEN)$(OUT_DIR) directory has been cleaned up$(RESET)\n"

re: fclean all

help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  unchecked - Build without -Wall -Wextra -Werror"
	@echo "  debug     - Build with debug symbols (-g)"
	@echo "  run       - Build and run the executable"
	@echo "  clean     - Remove object files"
	@echo "  fclean    - Remove all build artifacts"
	@echo "  re        - Rebuild from scratch"
	@echo "  help      - Show this help message"

-include $(DEPS) 
