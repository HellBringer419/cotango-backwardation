cmake_minimum_required(VERSION 3.23)
project(cotango-backwardation C)

add_executable(cotango-backwardation)

target_sources(cotango-backwardation
  PRIVATE
    main.c
)

set_target_properties(cotango-backwardation
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_SOURCE_DIR}/out
)

target_include_directories(cotango-backwardation
  PRIVATE
    /usr/local/include
)

target_compile_options(cotango-backwardation
  PRIVATE
    -Wall -Wextra -Wpedantic -fsanitize=undefined,address
)

target_link_directories(cotango-backwardation
  PRIVATE
    /usr/local/lib
)

target_link_options(cotango-backwardation
  PRIVATE
    -fsanitize=undefined,address -Wl,-R/usr/local/lib
)

# add_library(cotango-backwardation)
# target_sources(cotango-backwardation
#   PRIVATE
#     cotango-backwardation.c
#
#   PUBLIC
#     FILE_SET HEADERS
#     BASE_DIRS
#       cotango-backwardation
#     FILES
#       cotango-backwardation.h
# )

target_link_libraries(cotango-backwardation
  PRIVATE
    cjson
    fyers-api-c
)
