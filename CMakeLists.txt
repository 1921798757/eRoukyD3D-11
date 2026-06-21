cmake_minimum_required (VERSION 3.14)

# ------------------------------------------------------------------------------
# 用户配置
# ------------------------------------------------------------------------------
# 设置解决方案名
project("eRouky_DX11")
# 设置项目名(不能有空格) 
set(TARGET_NAME "eRouky_DX11")
# 设置HLSL文件的存放位置
set(HLSL_DIR "HLSL")
# Win7系统设置为ON
set(WIN7_SYSTEM OFF)
# 如果需要编译期编译着色器，设置为ON
# 要求着色器命名格式为：ShaderName_ShaderType.hlsl
# ShaderType为右边任意一种：VS, vs, HS, hs, DS, ds, GS, gs, PS, ps, CS, cs
set(SHADER_STATIC_COMPILE ON) 
# 使用ImGui时设置为ON
# 需要将项目中的ImGui文件夹复制进来，保留*.h/*.cpp/CMakeLists.txt
set(USE_IMGUI ON)
# 使用Assimp时设置为ON
# 需要将解压的assimp文件夹内CMakeList.txt所在的全部文件复制到当前项目的Assimp文件夹内
set(USE_ASSIMP OFF)

# ------------------------------------------------------------------------------
# 项目细节配置(需熟悉CMake)
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17) 
add_compile_definitions(UNICODE _UNICODE)
if (${WIN7_SYSTEM})
    add_compile_definitions(_WIN32_WINNT=0x601)
endif()
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# 拷贝着色器目录到输出目录
file(COPY ${HLSL_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
# 新增：拷贝纹理目录到输出目录
file(COPY Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 收集当前目录下的源码、头文件以及 HLSL 目录下的着色器文件
file(GLOB HLSL_FILES ${HLSL_DIR}/*.hlsl ${HLSL_DIR}/*.hlsli)
file(GLOB HEADER_FILES ./*.h)

aux_source_directory(. DIR_SRCS)

# 创建可执行文件并链接库 (直接将HLSL_FILES放入可执行文件，确保不管是否开启静态编译，在IDE中都始终可见)
add_executable(${TARGET_NAME} WIN32 ${DIR_SRCS} ${HEADER_FILES} ${HLSL_FILES})
target_link_libraries(${TARGET_NAME} d3d11.lib dxgi.lib dxguid.lib D3DCompiler.lib winmm.lib d2d1.lib dwrite.lib)

# 在 IDE 中对 HLSL 文件进行分组显示
source_group("HLSL Files" FILES ${HLSL_FILES})

# 设置目标属性：输出文件名及输出路径 (已融合：指定的带空格名字以及 FOLDER 分组)
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "Lighting")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})

# ------------------------------------------------------------------------------
# 可选功能模块配置
# ------------------------------------------------------------------------------

# 着色器静态编译
if (${SHADER_STATIC_COMPILE})
    # 已将 target_sources 移除，因为上方 add_executable 已经包含了 HLSL_FILES
    foreach(HLSL_FILE ${HLSL_FILES})
        get_filename_component(HLSL_FDIR ${HLSL_FILE} DIRECTORY)
        get_filename_component(HLSL_FBASENAME_WE ${HLSL_FILE} NAME_WE)
        string(CONCAT HLSL_FNAME_WE ${HLSL_FDIR} / ${HLSL_FBASENAME_WE})
        string(LENGTH ${HLSL_FBASENAME_WE} LEN_FNAME_WE)
        math(EXPR LEN_FNAME_WE "${LEN_FNAME_WE}-2")
        string(SUBSTRING ${HLSL_FBASENAME_WE} ${LEN_FNAME_WE} 2 ENTRY_POINT)
        string(TOLOWER ${ENTRY_POINT} SHADER_TYPE)
        
        if ("${SHADER_TYPE}" STREQUAL "vs") 
            set(SHADER_TYPE "Vertex")
        elseif("${SHADER_TYPE}" STREQUAL "hs") 
            set(SHADER_TYPE "Hull")
        elseif("${SHADER_TYPE}" STREQUAL "ds") 
            set(SHADER_TYPE "Domain")
        elseif("${SHADER_TYPE}" STREQUAL "gs") 
            set(SHADER_TYPE "Geometry")
        elseif("${SHADER_TYPE}" STREQUAL "ps")
            set(SHADER_TYPE "Pixel")
        elseif("${SHADER_TYPE}" STREQUAL "cs")
            set(SHADER_TYPE "Compute")
        endif()
        
        set_source_files_properties(${HLSL_FILE} PROPERTIES
                                    VS_SHADER_OBJECT_FILE_NAME HLSL/${HLSL_FBASENAME_WE}.cso
                                    VS_SHADER_TYPE ${SHADER_TYPE}
                                    VS_SHADER_MODEL 5.0
                                    VS_SHADER_ENTRYPOINT ${ENTRY_POINT}
                                    VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:true>
                                    VS_SHADER_ENABLE_DEBUG $<$<CONFIG:Debug>:true>)
    endforeach()
endif()

# ImGui 支持
if (${USE_IMGUI})
    add_subdirectory(ImGui)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_IMGUI)
    target_link_libraries(${TARGET_NAME} ImGui)
    file(COPY imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Assimp 支持
if (${USE_ASSIMP})
    set(ASSIMP_BUILD_ZLIB ON)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
    set(ASSIMP_BUILD_TESTS OFF)
    set(ASSIMP_INSTALL OFF)
    set(BUILD_SHARED_LIBS OFF)
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
    add_subdirectory("assimp")
    target_link_libraries(${TARGET_NAME} assimp)
endif()
