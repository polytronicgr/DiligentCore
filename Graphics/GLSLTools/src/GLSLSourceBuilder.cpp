/*     Copyright 2015-2018 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include <cstring>

#include "GLSLSourceBuilder.h"
#include "DebugUtilities.h"
#include "HLSL2GLSLConverterImpl.h"
#include "RefCntAutoPtr.h"
#include "DataBlobImpl.h"

namespace Diligent
{

String BuildGLSLSourceString(const ShaderCreationAttribs& CreationAttribs, TargetGLSLCompiler TargetCompiler, const char* ExtraDefinitions)
{
    String GLSLSource;

    auto ShaderType = CreationAttribs.Desc.ShaderType;

#if PLATFORM_WIN32 || PLATFORM_LINUX
    GLSLSource.append(
        "#version 430 core\n"
        "#define DESKTOP_GL 1\n"
    );
#   if PLATFORM_WIN32
    GLSLSource.append("#define PLATFORM_WIN32 1\n");
#   elif PLATFORM_LINUX
    GLSLSource.append("#define PLATFORM_LINUX 1\n");
#   else
#       error Unexpected platform
#   endif
#elif PLATFORM_MACOS
    GLSLSource.append(
        "#version 410 core\n"
        "#define DESKTOP_GL 1\n"
        "#define PLATFORM_MACOS 1\n"
    );
#elif PLATFORM_IOS
    GLSLSource.append(
        "#version 300 es\n"
        "#extension GL_EXT_separate_shader_objects : enable\n"
        "#ifndef GL_ES\n"
        "#  define GL_ES 1\n"
        "#endif\n"

        "#define PLATFORM_IOS 1\n"

        "precision highp float;\n"
        "precision highp int;\n"
        //"precision highp uint;\n"

        "precision highp sampler2D;\n"
        "precision highp sampler3D;\n"
        "precision highp samplerCube;\n"
        "precision highp samplerCubeShadow;\n"
        "precision highp sampler2DShadow;\n"
        "precision highp sampler2DArray;\n"
        "precision highp sampler2DArrayShadow;\n"

        "precision highp isampler2D;\n"
        "precision highp isampler3D;\n"
        "precision highp isamplerCube;\n"
        "precision highp isampler2DArray;\n"

        "precision highp usampler2D;\n"
        "precision highp usampler3D;\n"
        "precision highp usamplerCube;\n"
        "precision highp usampler2DArray;\n"
    );

    // Built-in variable 'gl_Position' must be redeclared before use, with separate shader objects.
    if (ShaderType == SHADER_TYPE_VERTEX)
        GLSLSource.append("out vec4 gl_Position;\n");

#elif PLATFORM_ANDROID
    GLSLSource.append(
        "#version 310 es\n"
        "#extension GL_EXT_texture_cube_map_array : enable\n"
    );

    if (ShaderType == SHADER_TYPE_GEOMETRY)
        GLSLSource.append("#extension GL_EXT_geometry_shader : enable\n");

    if (ShaderType == SHADER_TYPE_HULL || ShaderType == SHADER_TYPE_DOMAIN)
        GLSLSource.append("#extension GL_EXT_tessellation_shader : enable\n");

    GLSLSource.append(
        "#ifndef GL_ES\n"
        "#  define GL_ES 1\n"
        "#endif\n"

        "#define PLATFORM_ANDROID 1\n"

        "precision highp float;\n"
        "precision highp int;\n"
        //"precision highp uint;\n"  // This line causes shader compilation error on NVidia!

        "precision highp sampler2D;\n"
        "precision highp sampler3D;\n"
        "precision highp samplerCube;\n"
        "precision highp samplerCubeArray;\n"
        "precision highp samplerCubeShadow;\n"
        "precision highp samplerCubeArrayShadow;\n"
        "precision highp sampler2DShadow;\n"
        "precision highp sampler2DArray;\n"
        "precision highp sampler2DArrayShadow;\n"
        "precision highp sampler2DMS;\n"       // ES3.1

        "precision highp isampler2D;\n"
        "precision highp isampler3D;\n"
        "precision highp isamplerCube;\n"
        "precision highp isamplerCubeArray;\n"
        "precision highp isampler2DArray;\n"
        "precision highp isampler2DMS;\n"      // ES3.1

        "precision highp usampler2D;\n"
        "precision highp usampler3D;\n"
        "precision highp usamplerCube;\n"
        "precision highp usamplerCubeArray;\n"
        "precision highp usampler2DArray;\n"
        "precision highp usampler2DMS;\n"      // ES3.1

        "precision highp image2D;\n"
        "precision highp image3D;\n"
        "precision highp imageCube;\n"
        "precision highp image2DArray;\n"

        "precision highp iimage2D;\n"
        "precision highp iimage3D;\n"
        "precision highp iimageCube;\n"
        "precision highp iimage2DArray;\n"

        "precision highp uimage2D;\n"
        "precision highp uimage3D;\n"
        "precision highp uimageCube;\n"
        "precision highp uimage2DArray;\n"
    );
#elif
#   error "Undefined platform"
#endif
    // It would be much more convenient to use row_major matrices.
    // But unfortunatelly on NVIDIA, the following directive 
    // layout(std140, row_major) uniform;
    // does not have any effect on matrices that are part of structures
    // So we have to use column-major matrices which are default in both
    // DX and GLSL.
    GLSLSource.append(
        "layout(std140) uniform;\n"
    );

    if(ShaderType == SHADER_TYPE_VERTEX && TargetCompiler == TargetGLSLCompiler::glslang)
    {
        // https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt
        GLSLSource.append("#define gl_VertexID gl_VertexIndex\n"
                          "#define gl_InstanceID gl_InstanceIndex\n");
    }

    const Char* ShaderTypeDefine = nullptr;
    switch (ShaderType)
    {
        case SHADER_TYPE_VERTEX:    ShaderTypeDefine = "#define VERTEX_SHADER\n";         break;
        case SHADER_TYPE_PIXEL:     ShaderTypeDefine = "#define FRAGMENT_SHADER\n";       break;
        case SHADER_TYPE_GEOMETRY:  ShaderTypeDefine = "#define GEOMETRY_SHADER\n";       break;
        case SHADER_TYPE_HULL:      ShaderTypeDefine = "#define TESS_CONTROL_SHADER\n";   break;
        case SHADER_TYPE_DOMAIN:    ShaderTypeDefine = "#define TESS_EVALUATION_SHADER\n"; break;
        case SHADER_TYPE_COMPUTE:   ShaderTypeDefine = "#define COMPUTE_SHADER\n";        break;
        default: UNEXPECTED("Shader type is not specified");
    }
    GLSLSource += ShaderTypeDefine;

    if(ExtraDefinitions != nullptr)
    {
        GLSLSource.append(ExtraDefinitions);
    }

    if (CreationAttribs.Macros != nullptr)
    {
        auto *pMacro = CreationAttribs.Macros;
        while (pMacro->Name != nullptr && pMacro->Definition != nullptr)
        {
            GLSLSource += "#define ";
            GLSLSource += pMacro->Name;
            GLSLSource += ' ';
            GLSLSource += pMacro->Definition;
            GLSLSource += "\n";
            ++pMacro;
        }
    }

    RefCntAutoPtr<IDataBlob> pFileData(MakeNewRCObj<DataBlobImpl>()(0));
    auto ShaderSource = CreationAttribs.Source;
    size_t SourceLen = 0;
    if (ShaderSource)
    {
        SourceLen = strlen(ShaderSource);
    }
    else
    {
        VERIFY(CreationAttribs.pShaderSourceStreamFactory, "Input stream factory is null");
        RefCntAutoPtr<IFileStream> pSourceStream;
        CreationAttribs.pShaderSourceStreamFactory->CreateInputStream(CreationAttribs.FilePath, &pSourceStream);
        if (pSourceStream == nullptr)
            LOG_ERROR_AND_THROW("Failed to open shader source file");

        pSourceStream->Read(pFileData);
        ShaderSource = reinterpret_cast<char*>(pFileData->GetDataPtr());
        SourceLen = pFileData->GetSize();
    }

    if (CreationAttribs.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL)
    {
        // Convert HLSL to GLSL
        const auto &Converter = HLSL2GLSLConverterImpl::GetInstance();
        HLSL2GLSLConverterImpl::ConversionAttribs Attribs;
        Attribs.pSourceStreamFactory = CreationAttribs.pShaderSourceStreamFactory;
        Attribs.ppConversionStream = CreationAttribs.ppConversionStream;
        Attribs.HLSLSource = ShaderSource;
        Attribs.NumSymbols = SourceLen;
        Attribs.EntryPoint = CreationAttribs.EntryPoint;
        Attribs.ShaderType = CreationAttribs.Desc.ShaderType;
        Attribs.IncludeDefinitions = true;
        Attribs.InputFileName = CreationAttribs.FilePath;
        auto ConvertedSource = Converter.Convert(Attribs);
        
        GLSLSource.append(ConvertedSource);
    }
    else
        GLSLSource.append(ShaderSource, SourceLen);

    return GLSLSource;
}

}
