cmake_minimum_required(VERSION 3.17)

project(finite_em)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-O3")

add_executable(${PROJECT_NAME}
    glad.c 
    main.cpp 
    general/event_handler.cpp general/state.cpp
    general/graphics/buffer.cpp general/graphics/object.cpp general/graphics/render_target.cpp general/graphics/texture.cpp general/graphics/vao.cpp
    general/graphics/mesh.cpp general/graphics/model.cpp general/graphics/mesh.cpp
    general/graphics/shader/shader.cpp general/graphics/shader/shader_loader.cpp
    solver/femmesh.cpp solver/solver.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC GL dl glfw)
target_include_directories(${PROJECT_NAME} PUBLIC 
    "${PROJECT_SOURCE_DIR}/general" 
    "${PROJECT_SOURCE_DIR}/general/graphics"
    "${PROJECT_SOURCE_DIR}/general/graphics/shader"
    "${PROJECT_SOURCE_DIR}/solver")
