# Name of the test executable
NAME        = test_comprehensive

# Compiler and Flags
CC          = gcc
# -g for debugging info (valgrind loves this)
# -Wno-free-nonheap-object suppresses the warning for our intentional stack-free test
CFLAGS      = -Wall -Wextra -Werror -g -Wno-free-nonheap-object

# Directories
ROOT_DIR    = ..
INC_DIR     = $(ROOT_DIR)/include
LIBFT_INC   = $(ROOT_DIR)/libft/include

# Libraries
# -L..          : Look for libraries in the parent directory
# -lft_malloc   : Link against libft_malloc.so
# -Wl,-rpath,.. : Tell the loader to look for the .so in '..' at runtime (Linux/Docker)
LIBS        = -L$(ROOT_DIR) -lft_malloc
LDFLAGS     = -Wl,-rpath,$(ROOT_DIR)

# Sources
SRCS        = test_comprehensive.c
OBJS        = $(SRCS:.c=.o)

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	@# Ensure the main library is built first
	@make -C $(ROOT_DIR) > /dev/null
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) $(LDFLAGS) -o $@
	@echo "\033[32m[OK] Test executable compiled.\033[0m"

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(LIBFT_INC) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

# --- Helper Rules ---

# Run normally
test: all
	./$(NAME)

# Run with Scribble enabled (Bonus verification)
run: all
	@echo "\033[33mRunning with MallocScribble=1...\033[0m"
	MallocScribble=1 ./$(NAME)

# Run with Valgrind
valgrind: all
	@echo "\033[33mRunning Valgrind...\033[0m"
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

.PHONY: all clean fclean re test run valgrind