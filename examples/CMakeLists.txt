add_executable(hb_demo_render demo_render.cpp)
target_link_libraries(hb_demo_render PRIVATE HB::CoreStatic)
hbcore_warnings(hb_demo_render)

add_executable(hb_demo_c demo_c.c)
target_link_libraries(hb_demo_c PRIVATE HB::CoreStatic)

if(WIN32 AND TARGET hbcore_win32)
    add_executable(hb_win32_demo WIN32 win32_demo.cpp)
    target_link_libraries(hb_win32_demo PRIVATE HB::Win32)
endif()
