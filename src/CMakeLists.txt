set(SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_impl_sdl.cpp
    imgui_impl_opengl3.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    implot.cpp
    implot_items.cpp
    instrulink.cpp)

add_library(instruments STATIC ${SOURCES})

target_include_directories(instruments PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(instruments ${SDL2_LIBRARIES})

target_include_directories(instruments
                           PUBLIC "${CMAKE_SOURCE_DIR}/include/instruments")

target_include_directories(instruments INTERFACE "${CMAKE_SOURCE_DIR}/include/")

add_subdirectory(dcmotor)
