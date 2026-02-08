cmake_minimum_required(VERSION 3.10)
project(ft_malloc C)

# 1. Standard & Flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# -fPIC is required for Shared Libraries
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fPIC -D_REENTRANT")

# 2. Handle HOSTTYPE (Environment Variable Logic)
set(HOSTTYPE $ENV{HOSTTYPE})

if(NOT HOSTTYPE)
    execute_process(COMMAND uname -m OUTPUT_VARIABLE UNAME_M OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND uname -s OUTPUT_VARIABLE UNAME_S OUTPUT_STRIP_TRAILING_WHITESPACE)
    # The subject implies a concatenation. Standard 42 convention is usually with underscores.
    set(HOSTTYPE "${UNAME_M}_${UNAME_S}")
endif()

message(STATUS "Compiling for HOSTTYPE: ${HOSTTYPE}")

# 2.b Bring in libft (matches top-level Makefile behavior)
# This defines a target called `libft`.
add_subdirectory(libft)

# 3. Source Files
file(GLOB_RECURSE SRC_FILES "src/*.c")

# 4. Define the Library Target
add_library(ft_malloc SHARED ${SRC_FILES})

# 5. EXACT NAMING
# We disable the automatic "lib" prefix so we can set the full name manually.
set_target_properties(ft_malloc PROPERTIES
        PREFIX ""                                # Disable auto "lib" prefix
        OUTPUT_NAME "libft_malloc_${HOSTTYPE}"   # Explicit full name
)

# Force ".so" like the subject/Makefile, even on macOS.
set_target_properties(ft_malloc PROPERTIES SUFFIX ".so")

# 6. Include Directories
target_include_directories(ft_malloc PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/libft/include
)

# 7. Link dependencies
# - libft: required for show_alloc_mem helpers
# - pthread: for mutex/thread-safety
target_link_libraries(ft_malloc PRIVATE libft pthread)

# 8. Create Symbolic Link
# Creates libft_malloc.so -> libft_malloc_$HOSTTYPE.so
add_custom_command(TARGET ft_malloc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:ft_malloc>
        libft_malloc.so
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating symlink: libft_malloc.so -> libft_malloc_${HOSTTYPE}.so"
)