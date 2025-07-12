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
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoCryptod.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoJSONd.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoXMLd.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoFoundationd.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoUtild.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoDatad.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoDataSQLited.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoNetd.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoNetSSLWind.lib DESTINATION ${CMAKE_BINARY_DIR})
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoCrypto64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoJSON64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoXML64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoFoundation64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoUtil64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoData64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoDataSQLite64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoNet64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoNetSSLWin64d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/glfw3.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avformat-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avutil-59.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avcodec-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avdevice-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/swscale-8.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/freetype.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/datachannel.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/libssl-3-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/libcrypto-3-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/swresample-5.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avfilter-10.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleTextProjector.properties DESTINATION ${CMAKE_BINARY_DIR}/Debug)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${CMAKE_BINARY_DIR}/Debug)
    else()
        message(STATUS "Configuring for Release build")
        target_link_libraries(SimpleTextProjector ${POCO_LIBS_RELEASE} ${OTHER_LIBS})
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoCrypto.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoJSON.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoXML.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoFoundation.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoUtil.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoData.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoDataSQLite.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoNet.lib DESTINATION ${CMAKE_BINARY_DIR})
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/PocoNetSSLWin.lib DESTINATION ${CMAKE_BINARY_DIR})
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoCrypto64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoJSON64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoXML64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoFoundation64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoUtil64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoData64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoDataSQLite64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoNet64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/PocoNetSSLWin64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/glfw3.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avformat-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avutil-59.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avcodec-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avdevice-61.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/swscale-8.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/freetype.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/datachannel.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/libssl-3-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/libcrypto-3-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/swresample-5.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dlls/avfilter-10.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SimpleTextProjector.properties DESTINATION ${CMAKE_BINARY_DIR}/Release)
		
		file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${CMAKE_BINARY_DIR}/Release)
    endif()

    # Copy the lib folder (for DLLs) to the build output dir
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw3.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/avformat.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/avutil.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/avcodec.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/avdevice.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/swscale.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/datachannel.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/libssl.lib DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/libcrypto.lib DESTINATION ${CMAKE_BINARY_DIR})

endif()
