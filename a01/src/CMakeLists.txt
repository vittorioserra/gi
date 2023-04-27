# target name to generate
set(TARGET gi)

# glob source files
file(GLOB_RECURSE SOURCES "*.cpp")

# define target
add_executable(${TARGET} ${SOURCES})

# ----------------------------------------------------------
# dependencies

target_link_libraries(${TARGET} Threads::Threads OpenMP::OpenMP_CXX assimp)
if (EMBREE_FOUND)
    target_link_libraries(${TARGET} embree3)
else()
    target_link_libraries(${TARGET} embree)
endif()
if(OpenImageDenoise_FOUND)
    target_link_libraries(${TARGET} OpenImageDenoise)
endif()

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
include_directories(${TARGET} ${OPENGL_INCLUDE_DIR})
target_link_libraries(${TARGET} ${OPENGL_LIBRARIES})
target_link_libraries(${TARGET} libglew_static glfw)
