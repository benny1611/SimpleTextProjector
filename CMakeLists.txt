cmake_minimum_required(VERSION 3.10)

project(SimpleTextProjector)

set(CMAKE_CXX_STANDARD 17)

# Source files
set(SOURCES
    src/CommandRequestHandler.cpp
    src/HTTPSCommandServer.cpp
    src/Main.cpp
    src/ScreenStreamer.cpp
    src/SimpleTextProjectorUI.cpp
    src/glad.c
    src/HandlerList.cpp
    src/HTTPCommandServer.cpp
    src/ScreenStreamerTask.cpp
    src/TextBoxRenderer.cpp
    src/qrcodegen.cpp
    src/imgui/imgui_impl_glfw.cpp
    src/imgui/imgui_impl_opengl2.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
)

add_executable(SimpleTextProjector ${SOURCES})

set_target_properties(SimpleTextProjector PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug"
		RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
)

if(MSVC)
    set_target_properties(SimpleTextProjector PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()


include_directories(include src)

# Tell CMake to look for libraries in the lib folder
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Platform-specific configuration
if(WIN32)

    # Poco Debug libraries
    set(POCO_LIBS_DEBUG
        PocoCryptod
        PocoJSONd
        PocoXMLd
        PocoFoundationd
        PocoUtild
        PocoDatad
        PocoDataSQLited
        PocoNetd
        PocoNetSSLWind
    )

    # Poco Release libraries
    set(POCO_LIBS_RELEASE
        PocoCrypto
        PocoJSON
        PocoXML
        PocoFoundation
        PocoUtil
        PocoData
        PocoDataSQLite
        PocoNet
        PocoNetSSLWin
    )

    # Static libraries common to both configs
    set(OTHER_LIBS
        glfw3
        avformat
        avutil
        avcodec
        avdevice
        swscale
        freetype
        datachannel
        libssl
        libcrypto
        opengl32
    )

    # Link based on build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Configuring for Debug build")
        target_link_libraries(SimpleTextProjector ${POCO_LIBS_DEBUG} ${OTHER_LIBS})

		file(GLOB LIB_DEBUG "lib/libd/*")
		file(COPY ${LIB_DEBUG} DESTINATION ${CMAKE_BINARY_DIR})

		file(GLOB DLL_DEBUG "dlls/dlls_d/*")
		file(COPY ${DLL_DEBUG} DESTINATION ${CMAKE_BINARY_DIR}/Debug)

		file(GLOB DLL_COMMON "dlls/dlls_common/*")
		file(COPY ${DLL_COMMON} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleTextProjector.properties DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${CMAKE_BINARY_DIR}/Debug)
    else()
        message(STATUS "Configuring for Release build")
        target_link_libraries(SimpleTextProjector ${POCO_LIBS_RELEASE} ${OTHER_LIBS})

		file(GLOB LIB_RELEASE "lib/libr/*")
		file(COPY ${LIB_RELEASE} DESTINATION ${CMAKE_BINARY_DIR})

		file(GLOB DLL_RELEASE "dlls/dlls_r/*")
		file(COPY ${DLL_DEBUG} DESTINATION ${CMAKE_BINARY_DIR}/Release)

		file(GLOB DLL_COMMON "dlls/dlls_common/*")
		file(COPY ${DLL_COMMON} DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleTextProjector.properties DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${CMAKE_BINARY_DIR}/Release)
    endif()

    # Copy the lib folder (for DLLs) to the build output dir
	file(GLOB LIB_COMMON "lib/libcommon/*")
	file(COPY ${LIB_COMMON} DESTINATION ${CMAKE_BINARY_DIR})

endif()
