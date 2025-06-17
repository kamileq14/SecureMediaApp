cmake_minimum_required(VERSION 3.16) # Minimum CMake version
project(SecureMediaApp LANGUAGES CXX) # Define the project and specify C++ language

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
# --- C++ Standard Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Enforce strict C++17 compliance

# --- Global Compiler Options ---
if (MSVC)
    # Compiler flags for MSVC
    add_compile_options(
            /W4          # Enable all level 4 warnings
            /EHsc        # Enable C++ exception handling
            /Zi          # Enable debug information
            /Ob1         # Expand inline functions
            /Oi          # Enable intrinsic functions
            /Ot          # Optimize for speed
            /Oy-         # Disable frame pointer omission (better debugging)
            /GF          # Enable read-only string pooling
            /Gm-         # Disable minimal rebuild (faster incremental builds)
            /GS          # Enable security checks (buffer overrun detection)
            /Gy          # Enable function-level linking
            /MP          # Enable multi-processor compilation
            /MD          # Link with dynamic C runtime library (recommended for GDI+)
            /permissive- # Enforce standard C++ conformance
            /Zc:__cplusplus # Enable C++17 standard conformance macro
            /Zc:externC- # Enforce standard C++ extern "C" behavior
            /utf-8       # Use UTF-8 for source files
    )
    add_link_options(
            /DEBUG       # Generate debug information for linker
            /OPT:REF     # Eliminate unreferenced functions and data
            /OPT:ICF     # Perform COMDAT folding (reduces executable size)
    )
else() # GCC/Clang (MinGW falls under this)
    # Compiler flags for MinGW/GCC
    add_compile_options(
            -Wall           # Enable all common warnings
            -Wextra         # Enable extra warnings
            -Wpedantic      # Enforce strict ISO C++ standards
            -Wconversion    # Warn about implicit conversions that may change value
            -Wshadow        # Warn about shadowed variables
            -Wno-unused-parameter # Suppress warnings for unused function parameters
            -O2             # Optimize for speed
            -g              # Generate debug information
            -std=c++17      # Use C++17 standard
            -fdiagnostics-color=always # Enable colored diagnostics
    )
    add_link_options(
            -s              # Strip all symbol table and relocation information (smaller executable)
    )
endif()

# --- Windows Specific Settings ---
if(WIN32)
    # Define UNICODE and Windows SDK version for Windows API compatibility
    add_compile_definitions(
            UNICODE
            _UNICODE
            WINVER=0x0601      # Target Windows 7 (0x0601) or later
            _WIN32_WINNT=0x0601 # Target Windows 7 (0x0601) or later
    )

    # --- Resource File Compilation ---
    find_program(RC_COMPILER NAMES rc windres)
    if(NOT RC_COMPILER)
        message(FATAL_ERROR "Resource compiler (rc.exe for MSVC or windres.exe for MinGW) not found! Ensure it's in your PATH.")
    endif()

    set(RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    set(RESOURCE_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/resources.obj")
    set(RESOURCE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/resources.h")

    if(MSVC)
        add_custom_command(
                OUTPUT ${RESOURCE_OBJECT}
                COMMAND rc /fo ${RESOURCE_OBJECT} ${RESOURCE_FILE}
                DEPENDS ${RESOURCE_FILE} ${RESOURCE_HEADER}
                COMMENT "Compiling resource file with MSVC RC"
        )
    else() # MinGW/GCC
        add_custom_command(
                OUTPUT ${RESOURCE_OBJECT}
                COMMAND ${RC_COMPILER} ${RESOURCE_FILE} -O coff -o ${RESOURCE_OBJECT}
                DEPENDS ${RESOURCE_FILE} ${RESOURCE_HEADER}
                COMMENT "Compiling resource file with windres"
        )
    endif()

    # Create a custom target for resources to ensure compilation order
    add_custom_target(resources_target DEPENDS ${RESOURCE_OBJECT})

    # --- Source Files Collection ---
    # Explicitly list all C++ source files
    set(PROJECT_SOURCES
            main.cpp
            configuration.cpp
            # ToLower.cpp is not needed as ToLower is an inline function in ToLower.h

            # Components Source Files
            Components/WallpaperManager.cpp
            Components/ExplorerPolicyManager.cpp
            Components/FirewallManager.cpp
            Components/KeyboardHookManager.cpp
            Components/ProcessMonitor.cpp
            Components/NarratorManager.cpp
            Components/Watchdog.cpp
            Components/SoundPlayer.cpp
            Components/TaskSchedulerManager.cpp
            Components/GuiManager.cpp
    )

    # --- Executable Target Definition ---
    # Define the main executable. WIN32 property ensures it's a GUI application.
    add_executable(SecureMediaApp
            WIN32
            ${PROJECT_SOURCES}
            ${RESOURCE_OBJECT}
    )

    # --- Target Dependencies and Properties ---

    # Ensure resources are built before the executable
    add_dependencies(SecureMediaApp resources_target)

    # Include directories for headers
    # Look in Components folder and the project root folder
    target_include_directories(SecureMediaApp PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Components"
            "${CMAKE_CURRENT_SOURCE_DIR}" # For ToLower.h
    )

    # Link necessary Windows libraries
    target_link_libraries(SecureMediaApp PRIVATE
            user32              # User Interface functions (windows, messages)
            kernel32            # Core operating system functionality
            gdi32               # Graphics Device Interface (basic drawing)
            winmm               # Windows Multimedia (for Beep sound)
            gdiplus             # GDI+ library (for advanced graphics, e.g., rounded rectangles, alpha)
            advapi32            # Advanced API (e.g., registry, security)
            psapi               # Process Status API (for process information)
            ole32               # COM (Component Object Model) library (for CoInitializeEx, CoCreateInstance)
            oleaut32            # OLE Automation library (for BSTR, needed by FirewallManager)
            shell32             # Windows Shell API (for ShellExecute, e.g., launching Narrator)
    )

    # Specific linker options for MinGW/GCC for static linking and GUI subsystem
    if(NOT MSVC)
        target_link_options(SecureMediaApp PRIVATE
                -static-libgcc      # Statically link GCC runtime library
                -static-libstdc++   # Statically link C++ standard library
                -mwindows           # Set subsystem to WINDOWS (creates GUI app, no console window)
        )
    endif()

else()
    message(FATAL_ERROR "This project is designed exclusively for Windows and cannot be built on other operating systems.")
endif()