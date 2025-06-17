cmake_minimum_required(VERSION 3.16)
project(SecureMediaApp)

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ustawienia dla Windows
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)

    # Znajdź Resource Compiler
    find_program(RC_COMPILER NAMES rc windres)
    if(NOT RC_COMPILER)
        message(FATAL_ERROR "Resource compiler not found!")
    endif()

    # Kompiluj plik zasobów
    set(RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
    set(RESOURCE_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/resources.obj)

    # Sprawdź czy używamy MSVC czy MinGW
    if(MSVC)
        # Dla Visual Studio
        add_custom_command(
                OUTPUT ${RESOURCE_OBJECT}
                COMMAND rc /fo ${RESOURCE_OBJECT} ${RESOURCE_FILE}
                DEPENDS ${RESOURCE_FILE}
                COMMENT "Compiling resource file with MSVC RC"
        )
    else()
        # Dla MinGW/GCC - używaj windres
        add_custom_command(
                OUTPUT ${RESOURCE_OBJECT}
                COMMAND windres ${RESOURCE_FILE} -o ${RESOURCE_OBJECT}
                DEPENDS ${RESOURCE_FILE}
                COMMENT "Compiling resource file with windres"
        )
    endif()

    # Utwórz cel dla zasobów
    add_custom_target(resources DEPENDS ${RESOURCE_OBJECT})

    # Główny wykonalny plik
    add_executable(SecureMediaApp
            main.cpp
            ${RESOURCE_OBJECT}
    )

    # Zależność od zasobów
    add_dependencies(SecureMediaApp resources)

    # Link biblioteki Windows
    target_link_libraries(SecureMediaApp
            user32
            kernel32
            gdi32
            winmm
            gdiplus
            advapi32
            psapi
    )

    # Ustawienia kompilatora
    if(MSVC)
        target_compile_definitions(SecureMediaApp PRIVATE
                _WIN32_WINNT=0x0601
                WINVER=0x0601
                UNICODE
                _UNICODE
        )
        # Wyłącz ostrzeżenia dla MSVC
        target_compile_options(SecureMediaApp PRIVATE /W3)
    else()
        # Dla MinGW
        target_compile_definitions(SecureMediaApp PRIVATE
                _WIN32_WINNT=0x0601
                WINVER=0x0601
                UNICODE
                _UNICODE
        )
        # Statyczne linkowanie dla przenośności - przenieś do target_link_options
        target_link_options(SecureMediaApp PRIVATE -static-libgcc -static-libstdc++ -mwindows)
    endif()

    # Ustawienie typu aplikacji (Windows GUI, nie konsola)
    if(MSVC)
        set_target_properties(SecureMediaApp PROPERTIES
                WIN32_EXECUTABLE TRUE
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
        )
    else()
        # Dla MinGW - ustawienia są już w target_link_options powyżej
        set_target_properties(SecureMediaApp PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    endif()

else()
    message(FATAL_ERROR "This project is designed for Windows only!")
endif()

# Opcjonalne: kopiuj obraz do folderu build (dla testów)
configure_file(${CMAKE_SOURCE_DIR}/image.jpg ${CMAKE_BINARY_DIR}/image.jpg COPYONLY)