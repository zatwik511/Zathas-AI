cmake_minimum_required(VERSION 3.16)
project(zathas_ai VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# ── llama.cpp ──────────────────────────────────────────────────────────────────
FetchContent_Declare(
  llama_cpp
  GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
  GIT_TAG        b5615   # pin to a stable build
  GIT_SHALLOW    TRUE
)
set(LLAMA_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(llama_cpp)

# ── cpp-httplib ────────────────────────────────────────────────────────────────
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.18.5
  GIT_SHALLOW    TRUE
)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib)

# ── nlohmann/json ──────────────────────────────────────────────────────────────
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(zathas_ai
  src/main.cpp
  src/inference.cpp
  src/server.cpp
  src/memory.cpp
)

target_include_directories(zathas_ai PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(zathas_ai PRIVATE
  llama
  httplib::httplib
  nlohmann_json::nlohmann_json
)

# Copy frontend files next to the binary after build
add_custom_command(TARGET zathas_ai POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/frontend
    $<TARGET_FILE_DIR:zathas_ai>/frontend
  COMMENT "Copying frontend files..."
)

# ── Platform tweaks ────────────────────────────────────────────────────────────
if(WIN32)
  target_link_libraries(zathas_ai PRIVATE ws2_32)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(zathas_ai PRIVATE pthread dl)
endif()
