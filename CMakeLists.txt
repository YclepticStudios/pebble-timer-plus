cmake_minimum_required(VERSION 3.2)
PROJECT (Timer++)

FILE(GLOB SOURCE
  "src/*.h"
  "src/*.c"
  "src/js/*.js"
)

FILE(GLOB HEADERS
  "/usr/local/Cellar/pebble-sdk/3.6/Pebble/basalt/include/*.h"
)

INCLUDE_DIRECTORIES(/usr/local/Cellar/pebble-sdk/3.6/Pebble/basalt/include)
ADD_EXECUTABLE(Timer++ ${SOURCE} ${HEADERS})
