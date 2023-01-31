cmake_minimum_required (VERSION 3.8)

# Dodaj źródło do pliku wykonywalnego tego projektu.
add_executable (project2D
        "app.cpp" "app.h"
        "game.cpp" "game.h"
        "paddle.cpp" "paddle.h"
        "ball.cpp" "ball.h"
        "brick.cpp" "brick.h"
        "brick_type.h"
        "drop.cpp" "drop.h"
        "drop_type.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET project2D PROPERTY CXX_STANDARD 20)
endif()

# Szukanie biblioteki Direct2D (d2d1.lib)
find_library(DIRECT2D d2d1)
if (NOT DIRECT2D)
    message(FATAL_ERROR "Could not find Direct2D.")
endif ()

# Dołączenie biblioteki Direct2D
target_link_libraries(project2D ${DIRECT2D})

# Szukanie biblioteki DirectWrite (dwrite.lib)
find_library(DIRECT_WRITE dwrite)
if (NOT DIRECT_WRITE)
    message(FATAL_ERROR "Could not find DirectWrite.")
endif ()

# Dołączenie biblioteki DirectWrite
target_link_libraries(project2D ${DIRECT_WRITE})

# Szukanie biblioteki WindowsCodecs (windowscodecs.lib)
find_library(WINDOWS_CODECS windowscodecs)
if (NOT WINDOWS_CODECS)
    message(FATAL_ERROR "Could not find WindowsCodecs.")
endif ()

# Dołączenie biblioteki WindowsCodecs
target_link_libraries(project2D ${WINDOWS_CODECS})