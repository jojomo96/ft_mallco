# ==============================================================================
#  Global Variables & Hosttype Logic [cite: 27, 28, 29, 30]
# ==============================================================================

# If HOSTTYPE is not set, detect it using uname
ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

# The name of the final shared library [cite: 24]
NAME		= libft_malloc_$(HOSTTYPE).so

# The symbolic link name
SYMLINK		= libft_malloc.so

# ==============================================================================
#  Compilation Flags
# ==============================================================================

CC			= gcc
# -fPIC is crucial for creating shared libraries (.so)
CFLAGS		= -Wall -Wextra -Werror -fPIC
# -shared tells the linker to create a shared library
LDFLAGS		= -shared

# ==============================================================================
#  Directories & Sources
# ==============================================================================

SRC_DIR		= src
OBJ_DIR		= obj
INC_DIR		= include
LIBFT_DIR	= libft

# Library paths
LIBFT		= $(LIBFT_DIR)/libft.a

SRCS        := $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJS		= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ==============================================================================
#  Rules
# ==============================================================================

# Default rule
all: $(LIBFT_DIR)/.git $(NAME)

# Initialize and update submodules if libft is not present
$(LIBFT_DIR)/.git:
	@echo "Initializing git submodules..."
	git submodule update --init --recursive


# Link the library and create the symlink
$(NAME): $(LIBFT) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBFT) -o $@
	ln -sf $(NAME) $(SYMLINK)
	@echo "Created $(NAME) and symlink $(SYMLINK)"

# Compile Libft
$(LIBFT): $(LIBFT_DIR)/.git
	@echo "Compiling libft..."
	@$(MAKE) -C $(LIBFT_DIR)

# Compile Object Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I $(INC_DIR) -I $(LIBFT_DIR)/include -c $< -o $@

# Clean Objects
clean:
	rm -rf $(OBJ_DIR)
	@$(MAKE) -C $(LIBFT_DIR) clean

# Full Clean (Objects + Libraries + Symlink)
fclean: clean
	rm -f $(NAME) $(SYMLINK)
	@$(MAKE) -C $(LIBFT_DIR) fclean

# Recompile
re: fclean all

# Phony targets to prevent conflicts with files of the same name
.PHONY: all clean fclean re