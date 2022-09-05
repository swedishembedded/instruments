add_executable(instrument-keypad main.cpp)

target_include_directories(instrument-keypad PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(instrument-keypad instruments GL control dl pthread)

install(TARGETS instrument-keypad RUNTIME DESTINATION "/usr/bin/")
