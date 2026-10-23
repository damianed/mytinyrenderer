PLATFORM_COMPILE_FILES="main.c des_image.c"
PLATFORM_LINKER_FLAGS=""

GAME_COMPILE_FILES=""

SHARED_COMP_FLAGS="-DDEBUG -ggdb -Wall -Wextra -Werror -std=gnu99 -Wvla -Wno-unused-parameter"

#TODO: add a project path as -D var so I can know where to get the game.so
mkdir -p build
#gcc $SHARED_COMP_FLAGS -shared -fPIC $GAME_COMPILE_FILES -o build/game.so

gcc $SHARED_COMP_FLAGS $PLATFORM_COMPILE_FILES -o build/main $PLATFORM_LINKER_FLAGS
