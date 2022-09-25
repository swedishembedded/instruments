find_package(verilator HINTS ${USER_VERILATOR_DIR} $ENV{VERILATOR_ROOT})

set(SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_impl_sdl.cpp
    imgui_impl_opengl3.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    implot.cpp
    implot_items.cpp
    instrulink.cpp
    InstrumentContainer.cpp
    KeypadInstrument.cpp
    UARTInstrument.cpp
    LiteUART.cpp
    DCMotorInstrument.cpp)

add_library(instruments STATIC ${SOURCES})

add_library(VLiteUART)
verilate(VLiteUART SOURCES LiteUART.v)

target_compile_options(instruments PUBLIC -Wall -Werror -Wextra -faligned-new
                                          -Wno-unused-parameter)
target_include_directories(instruments PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(instruments PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_options(instruments PUBLIC -L${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(instruments ${SDL2_LIBRARIES} VLiteUART)

target_include_directories(instruments PRIVATE ${VERILATOR_ROOT}/include)
target_include_directories(instruments PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(instruments
                           PUBLIC "${CMAKE_SOURCE_DIR}/include/instruments")

target_include_directories(instruments INTERFACE "${CMAKE_SOURCE_DIR}/include/")

add_subdirectory(dcmotor)
add_subdirectory(keypad)
add_subdirectory(liteuart)
