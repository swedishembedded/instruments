add_executable(instrument-dcmotor main.cpp dc_motor.c)

target_include_directories(instruments
    PRIVATE
        "${CMAKE_SOURCE_DIR}/include/"
)

target_include_directories(instrument-dcmotor PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(instrument-dcmotor instruments GL control dl pthread)

install(TARGETS instrument-dcmotor
    RUNTIME DESTINATION "/usr/bin/"
    DESTINATION "/usr/bin/"
)
