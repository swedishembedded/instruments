add_executable(instrument-liteuart main.cpp)

target_include_directories(instrument-liteuart PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(instrument-liteuart
                           PRIVATE /usr/share/verilator/include/)
target_include_directories(instrument-liteuart
                           PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../)
target_include_directories(instrument-liteuart
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS instrument-liteuart RUNTIME DESTINATION "/usr/bin/")

target_link_libraries(
  instrument-liteuart
  instruments
  GL
  control
  dl
  pthread
  instruments)
