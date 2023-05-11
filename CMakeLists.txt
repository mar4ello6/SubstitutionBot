cmake_minimum_required(VERSION 3.18)
project(SubstitutionBot VERSION 0.0.1)

set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(ext/json)

add_subdirectory(ext/tgbot-cpp)
add_subdirectory(ext/SQLiteCpp)
include_directories(ext/json/include ext/tgbot-cpp/include ext/httplib ext/SQLiteCpp/include src)

add_executable(sbot
                src/main.cpp src/Commands.cpp src/Core/Core.cpp src/edupage/Substitution.cpp src/Checkers/SubstitutionChecker.cpp)
target_link_libraries(sbot TgBot ssl gumbo SQLiteCpp)