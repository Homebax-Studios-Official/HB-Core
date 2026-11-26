add_executable(hb_demo_render demo_render.cpp)
target_link_libraries(hb_demo_render PRIVATE HB::CoreStatic)
hbcore_warnings(hb_demo_render)

add_executable(hb_demo_c demo_c.c)
target_link_libraries(hb_demo_c PRIVATE HB::CoreStatic)
