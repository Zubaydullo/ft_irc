# Compiler settings
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include

# Project name
NAME = ircserv

# Source directories
SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
CHANNEL_DIR = $(SRC_DIR)/channel
COMMANDS_DIR = $(SRC_DIR)/commands
BASIC_CMD_DIR = $(COMMANDS_DIR)/basic
CHANNEL_CMD_DIR = $(COMMANDS_DIR)/channel
MESSAGING_CMD_DIR = $(COMMANDS_DIR)/messaging
OPERATOR_CMD_DIR = $(COMMANDS_DIR)/operator

# Source files
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
      $(BASIC_CMD_DIR)/Quit.cpp \
      $(CHANNEL_CMD_DIR)/Join.cpp \
      $(CHANNEL_CMD_DIR)/Part.cpp \
      $(CHANNEL_CMD_DIR)/Topic.cpp \
      $(CHANNEL_CMD_DIR)/Names.cpp \
      $(CHANNEL_CMD_DIR)/Invite.cpp \
      $(CHANNEL_CMD_DIR)/Kick.cpp \
      $(MESSAGING_CMD_DIR)/Privmsg.cpp \
      $(OPERATOR_CMD_DIR)/Mode.cpp \

# Object files
OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Create object directories
$(shell mkdir -p $(OBJ_DIR) \
    $(OBJ_DIR)/server \
    $(OBJ_DIR)/client \
    $(OBJ_DIR)/channel \
    $(OBJ_DIR)/commands/basic \
    $(OBJ_DIR)/commands/channel \
    $(OBJ_DIR)/commands/messaging \
    $(OBJ_DIR)/commands/operator \
    $(OBJ_DIR)/utils)

# Main target
all: $(NAME)

# Linking
$(NAME): $(OBJ)
	@echo "Linking..."
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Done!"

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object files
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJ_DIR)
	@echo "Done!"

# Clean everything
fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)
	@echo "Done!"

# Rebuild everything
re:
	@echo "Rebuilding project..."
	@$(MAKE) fclean
	@$(MAKE) all
	@echo "Rebuild complete!"

# Phony targets
.PHONY: all clean fclean re
