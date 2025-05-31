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
BONUS_CMD_DIR = $(COMMANDS_DIR)/bonus
UTILS_DIR = $(SRC_DIR)/utils
MESSAGE_DIR = $(SRC_DIR)/message

# Source files
SRC = $(SRC_DIR)/main.cpp \
      $(SERVER_DIR)/Server.cpp \
      $(SERVER_DIR)/ServerInit.cpp \
      $(SERVER_DIR)/ServerSocket.cpp \
      $(SERVER_DIR)/ServerLoop.cpp \
      $(CLIENT_DIR)/Client.cpp \
      $(CLIENT_DIR)/ClientAuth.cpp \
      $(CLIENT_DIR)/ClientBuffer.cpp \
      $(CHANNEL_DIR)/Channel.cpp \
      $(CHANNEL_DIR)/ChannelModes.cpp \
      $(CHANNEL_DIR)/ChannelOperations.cpp \
      $(COMMANDS_DIR)/Command.cpp \
      $(COMMANDS_DIR)/CommandParser.cpp \
      $(BASIC_CMD_DIR)/Nick.cpp \
      $(BASIC_CMD_DIR)/User.cpp \
      $(BASIC_CMD_DIR)/Pass.cpp \
      $(BASIC_CMD_DIR)/Quit.cpp \
      $(BASIC_CMD_DIR)/Ping.cpp \
      $(CHANNEL_CMD_DIR)/Join.cpp \
      $(CHANNEL_CMD_DIR)/Part.cpp \
      $(CHANNEL_CMD_DIR)/Topic.cpp \
      $(CHANNEL_CMD_DIR)/Names.cpp \
      $(CHANNEL_CMD_DIR)/List.cpp \
      $(CHANNEL_CMD_DIR)/Invite.cpp \
      $(CHANNEL_CMD_DIR)/Kick.cpp \
      $(MESSAGING_CMD_DIR)/Privmsg.cpp \
      $(MESSAGING_CMD_DIR)/Notice.cpp \
      $(OPERATOR_CMD_DIR)/Mode.cpp \
      $(OPERATOR_CMD_DIR)/Oper.cpp \
      $(BONUS_CMD_DIR)/Who.cpp \
      $(BONUS_CMD_DIR)/Whois.cpp \
      $(BONUS_CMD_DIR)/Away.cpp \
      $(BONUS_CMD_DIR)/FileTransfer.cpp \
      $(UTILS_DIR)/Utils.cpp \
      $(UTILS_DIR)/Logger.cpp \
      $(UTILS_DIR)/Config.cpp \
      $(UTILS_DIR)/Validation.cpp \
      $(MESSAGE_DIR)/Message.cpp \
      $(MESSAGE_DIR)/MessageBuffer.cpp

# Object files
OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Create object directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR) \
		$(OBJ_DIR)/server \
		$(OBJ_DIR)/client \
		$(OBJ_DIR)/channel \
		$(OBJ_DIR)/commands/basic \
		$(OBJ_DIR)/commands/channel \
		$(OBJ_DIR)/commands/messaging \
		$(OBJ_DIR)/commands/operator \
		$(OBJ_DIR)/commands/bonus \
		$(OBJ_DIR)/utils \
		$(OBJ_DIR)/message

# Main target
all: $(OBJ_DIR) $(NAME)

# Linking
$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object files
clean:
	rm -rf $(OBJ_DIR)

# Clean everything
fclean: clean
	rm -f $(NAME)

# Rebuild everything
re: fclean all

# Phony targets
.PHONY: all clean fclean re