set(SRCS
    main.cpp
    Window.cpp
    Window.h

    Shaders/diffuse.fs
    Shaders/diffuse.vs
    Textures/voronoi.png

    resources.qrc
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(demo-app ${SRCS})

if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(demo-app PRIVATE /W4 /WX)
else()
    # lots of warnings and all warnings as errors
    target_compile_options(demo-app PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_link_libraries(demo-app
    PRIVATE
        Qt5::Widgets
        FGL::Base
        draco::draco
        thirdparty::tinygltf
)