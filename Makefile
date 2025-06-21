CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include

NAME = ircserv

GREEN = \033[0;32m
RED = \033[0;31m
CYAN = \033[0;36m
YELLOW = \033[1;33m
PURPLE = \033[0;35m
BLUE = \033[0;34m
BOLD = \033[1m
RESET = \033[0m

HACK_SYMBOL = ▓
EMPTY_SYMBOL = ░
ARROW = ►
SKULL = ☠
GEAR = ⚙
LOCK = 🔒
KEY = 🔑

SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
CHANNEL_DIR = $(SRC_DIR)/channel
COMMANDS_DIR = $(SRC_DIR)/commands
BASIC_CMD_DIR = $(COMMANDS_DIR)/basic
CHANNEL_CMD_DIR = $(COMMANDS_DIR)/channel
MESSAGING_CMD_DIR = $(COMMANDS_DIR)/messaging
OPERATOR_CMD_DIR = $(COMMANDS_DIR)/operator

SRC = $(SRC_DIR)/main.cpp \
      $(SERVER_DIR)/Server.cpp \
      $(SERVER_DIR)/ServerSocket.cpp \
      $(SERVER_DIR)/ServerLoop.cpp \
      $(CLIENT_DIR)/Client.cpp \
      $(CHANNEL_DIR)/Channel.cpp \
      $(COMMANDS_DIR)/CommandParser.cpp \
      $(BASIC_CMD_DIR)/Nick.cpp \
      $(BASIC_CMD_DIR)/User.cpp \
      $(BASIC_CMD_DIR)/Pass.cpp \
      $(CHANNEL_CMD_DIR)/Join.cpp \
      $(CHANNEL_CMD_DIR)/Part.cpp \
      $(CHANNEL_CMD_DIR)/Topic.cpp \
      $(CHANNEL_CMD_DIR)/Names.cpp \
      $(CHANNEL_CMD_DIR)/Invite.cpp \
      $(CHANNEL_CMD_DIR)/Kick.cpp \
      $(MESSAGING_CMD_DIR)/Privmsg.cpp \
      $(OPERATOR_CMD_DIR)/Mode.cpp \

OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TOTAL_FILES := $(words $(SRC))
CURRENT_FILE = 0

$(shell mkdir -p $(OBJ_DIR) \
    $(OBJ_DIR)/server \
    $(OBJ_DIR)/client \
    $(OBJ_DIR)/channel \
    $(OBJ_DIR)/commands/basic \
    $(OBJ_DIR)/commands/channel \
    $(OBJ_DIR)/commands/messaging \
    $(OBJ_DIR)/commands/operator \
    $(OBJ_DIR)/utils)

define show_progress
	$(eval CURRENT_FILE=$(shell echo $$(($(CURRENT_FILE)+1))))
	$(eval PERCENT=$(shell echo $$(($(CURRENT_FILE)*100/$(TOTAL_FILES)))))
	$(eval FILLED=$(shell echo $$(($(PERCENT)/5))))
	$(eval EMPTY=$(shell echo $$((20-$(FILLED)))))
	@printf "$(CYAN)$(BOLD)[HACKING PROGRESS]$(RESET) "
	@printf "$(GREEN)$(BOLD)"
	@for i in $$(seq 1 $(FILLED)); do printf "$(HACK_SYMBOL)"; done
	@printf "$(RESET)$(BLUE)"
	@for i in $$(seq 1 $(EMPTY)); do printf "$(EMPTY_SYMBOL)"; done
	@printf "$(RESET) $(YELLOW)$(BOLD)%3d%%$(RESET)" $(PERCENT)
	@printf " $(PURPLE)$(ARROW) $(CYAN)%s$(RESET)\n" "$(notdir $<)"
endef

define hacker_banner
	@echo "$(GREEN)$(BOLD)"
	@echo "██╗██████╗  ██████╗███████╗███████╗██████╗ ██╗   ██╗"
	@echo "██║██╔══██╗██╔════╝██╔════╝██╔════╝██╔══██╗██║   ██║"
	@echo "██║██████╔╝██║     ███████╗█████╗  ██████╔╝██║   ██║"
	@echo "██║██╔══██╗██║     ╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝"
	@echo "██║██║  ██║╚██████╗███████║███████╗██║  ██║ ╚████╔╝ "
	@echo "╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  "
	@echo "$(RESET)"
	@echo "$(CYAN)$(BOLD)[ INITIALIZING HACK SEQUENCE... ]$(RESET)"
	@echo "$(YELLOW)Target: IRC Server Compilation$(RESET)"
	@echo "$(RED)$(BOLD)Status: $(GREEN)AUTHORIZED $(KEY)$(RESET)"
	@echo ""
endef

define success_banner
	@echo ""
	@echo "$(GREEN)$(BOLD)$(GEAR) ═══════════════════════════════════════ $(GEAR)$(RESET)"
	@echo "$(GREEN)$(BOLD)           COMPILATION SUCCESSFUL!           $(RESET)"
	@echo "$(GREEN)$(BOLD)$(GEAR) ═══════════════════════════════════════ $(GEAR)$(RESET)"
	@echo "$(CYAN)Binary: $(YELLOW)$(BOLD)./$(NAME)$(RESET)"
	@echo "$(CYAN)Status: $(GREEN)$(BOLD)READY FOR DEPLOYMENT $(LOCK)$(RESET)"
	@echo ""
endef

all: 
	@$(call hacker_banner)
	@$(MAKE) $(NAME) --no-print-directory

$(NAME): $(OBJ)
	@echo ""
	@echo "$(PURPLE)$(BOLD)[ LINKING MODULES... ]$(RESET)"
	@printf "$(YELLOW)$(BOLD)Establishing connections$(RESET)"
	@for i in 1 2 3; do printf "."; sleep 0.3; done
	@echo ""
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@$(call success_banner)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(call show_progress)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(RED)$(BOLD)[ INITIATING CLEANUP PROTOCOL ]$(RESET)"
	@echo "$(YELLOW)Removing object files...$(RESET)"
	@printf "$(RED)$(BOLD)Wiping traces$(RESET)"
	@for i in 1 2 3; do printf "."; sleep 0.2; done
	@echo ""
	@rm -rf $(OBJ_DIR)
	@echo "$(GREEN)$(BOLD)✓ Cleanup complete!$(RESET)"

fclean: clean
	@echo "$(RED)$(BOLD)[ FULL SYSTEM PURGE ]$(RESET)"
	@echo "$(YELLOW)Removing executable...$(RESET)"
	@printf "$(RED)$(BOLD)Destroying evidence$(RESET)"
	@for i in 1 2 3; do printf "."; sleep 0.2; done
	@echo ""
	@rm -f $(NAME)
	@echo "$(GREEN)$(BOLD)✓ All traces eliminated! $(SKULL)$(RESET)"

re:
	@echo "$(PURPLE)$(BOLD)[ REBUILDING SYSTEM FROM SCRATCH ]$(RESET)"
	@echo "$(CYAN)Initiating full reconstruction...$(RESET)"
	@$(MAKE) fclean --no-print-directory
	@echo ""
	@$(MAKE) all --no-print-directory

stats:
	@echo "$(CYAN)$(BOLD)[ PROJECT STATISTICS ]$(RESET)"
	@echo "$(YELLOW)Total source files: $(GREEN)$(BOLD)$(TOTAL_FILES)$(RESET)"
	@echo "$(YELLOW)Total lines of code: $(GREEN)$(BOLD)$$(find $(SRC_DIR) -name '*.cpp' -o -name '*.hpp' | xargs wc -l | tail -1 | awk '{print $$1}')$(RESET)"
	@echo "$(YELLOW)Project name: $(GREEN)$(BOLD)$(NAME)$(RESET)"
	@echo "$(YELLOW)Compiler: $(GREEN)$(BOLD)$(CXX)$(RESET)"

.PHONY: all clean fclean re stats
