# Compiler settings
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include

# Project name
NAME = ftbot

# Source directories
SRC_DIR = src
INCLUDE_DIR = include

# Source files
SRC = $(SRC_DIR)/main.cpp \
      $(SRC_DIR)/Bot.cpp

# Object files
OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Create object directory
$(shell mkdir -p $(OBJ_DIR))

# Main target
all: $(NAME)

# Linking
$(NAME): $(OBJ)
	@echo "Linking bot..."
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Bot compilation complete!"

# Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean object files
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJ_DIR)
	@echo "Clean complete!"

# Clean everything
fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)
	@echo "Full clean complete!"

# Rebuild everything
re:
	@echo "Rebuilding bot..."
	@$(MAKE) fclean
	@$(MAKE) all
	@echo "Rebuild complete!"

# Run the bot (for testing)
run: $(NAME)
	@echo "Usage: ./$(NAME) <port> <password>"
	@echo "Example: ./$(NAME) 6667 pass123"

# Show help
help:
	@echo "Available targets:"
	@echo "  all     - Build the bot"
	@echo "  clean   - Remove object files"
	@echo "  fclean  - Remove object files and executable"
	@echo "  re      - Rebuild everything"
	@echo "  run     - Show usage information"
	@echo "  help    - Show this help"

# Phony targets
.PHONY: all clean fclean re run help