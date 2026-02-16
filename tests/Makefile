# Name of the test executables
NAME_BASIC  = test_basic
NAME_COMP   = test_comprehensive
NAME_LONG   = test_long

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
SRC_BASIC   = test_basic.c
SRC_COMP    = test_comprehensive.c
SRC_LONG    = test_long.c

OBJ_BASIC   = $(SRC_BASIC:.c=.o)
OBJ_COMP    = $(SRC_COMP:.c=.o)
OBJ_LONG    = $(SRC_LONG:.c=.o)

# Rules
all: lib $(NAME_BASIC) $(NAME_COMP) $(NAME_LONG)

lib:
	@make -C $(ROOT_DIR) > /dev/null

$(NAME_BASIC): $(OBJ_BASIC)
	$(CC) $(CFLAGS) $(OBJ_BASIC) $(LIBS) $(LDFLAGS) -o $@
	@echo "\033[32m[OK] $@ compiled.\033[0m"

$(NAME_COMP): $(OBJ_COMP)
	$(CC) $(CFLAGS) $(OBJ_COMP) $(LIBS) $(LDFLAGS) -o $@
	@echo "\033[32m[OK] $@ compiled.\033[0m"

$(NAME_LONG): $(OBJ_LONG)
	$(CC) $(CFLAGS) $(OBJ_LONG) $(LIBS) $(LDFLAGS) -o $@
	@echo "\033[32m[OK] $@ compiled.\033[0m"

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -I$(LIBFT_INC) -c $< -o $@

clean:
	rm -f $(OBJ_BASIC) $(OBJ_COMP) $(OBJ_LONG)

fclean: clean
	rm -f $(NAME_BASIC) $(NAME_COMP) $(NAME_LONG)

re: fclean all

# --- Helper Rules ---

# Run basic test
run_basic: $(NAME_BASIC)
	./$(NAME_BASIC)

# Run comprehensive test
run_comp: $(NAME_COMP)
	./$(NAME_COMP)

# Run long test
run_long: $(NAME_LONG)
	./$(NAME_LONG)

.PHONY: all clean fclean re lib run_basic run_comp run_long
