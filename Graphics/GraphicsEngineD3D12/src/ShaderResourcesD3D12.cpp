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

#include "pch.h"

#include <d3dcompiler.h>
#include "ShaderResourcesD3D12.h"
#include "ShaderD3DBase.h"
#include "ShaderBase.h"
#include "D3DShaderResourceLoader.h"


namespace Diligent
{


ShaderResourcesD3D12::ShaderResourcesD3D12(ID3DBlob *pShaderBytecode, const ShaderDesc &ShdrDesc) :
    ShaderResources(GetRawAllocator(), ShdrDesc.ShaderType)
{
    Uint32 CurrCB = 0, CurrTexSRV = 0, CurrTexUAV = 0, CurrBufSRV = 0, CurrBufUAV = 0, CurrSampler = 0;
    LoadD3DShaderResources<D3D12_SHADER_DESC, D3D12_SHADER_INPUT_BIND_DESC, ID3D12ShaderReflection>(
        pShaderBytecode,

        [&](Uint32 NumCBs, Uint32 NumTexSRVs, Uint32 NumTexUAVs, Uint32 NumBufSRVs, Uint32 NumBufUAVs, Uint32 NumSamplers, size_t ResourceNamesPoolSize)
        {
            Initialize(GetRawAllocator(), NumCBs, NumTexSRVs, NumTexUAVs, NumBufSRVs, NumBufUAVs, NumSamplers, ResourceNamesPoolSize);
        },

        [&](const D3DShaderResourceAttribs& CBAttribs)
        {
            new (&GetCB(CurrCB++)) D3DShaderResourceAttribs(m_ResourceNames, CBAttribs);
        },

        [&](const D3DShaderResourceAttribs& TexUAV)
        {
            new (&GetTexUAV(CurrTexUAV++)) D3DShaderResourceAttribs(m_ResourceNames, TexUAV);
        },

        [&](const D3DShaderResourceAttribs& BuffUAV)
        {
            new (&GetBufUAV(CurrBufUAV++)) D3DShaderResourceAttribs(m_ResourceNames, BuffUAV);
        },

        [&](const D3DShaderResourceAttribs& BuffSRV)
        {
            new (&GetBufSRV(CurrBufSRV++)) D3DShaderResourceAttribs(m_ResourceNames, BuffSRV);
        },

        [&](const D3DShaderResourceAttribs& SamplerAttribs)
        {
            new (&GetSampler(CurrSampler++)) D3DShaderResourceAttribs(m_ResourceNames, SamplerAttribs);
        },

        [&](const D3DShaderResourceAttribs& TexAttribs)
        {
            VERIFY(CurrSampler == GetNumSamplers(), "All samplers must be initialized before texture SRVs" );

            auto SamplerId = FindAssignedSamplerId(TexAttribs);
            new (&GetTexSRV(CurrTexSRV++)) D3DShaderResourceAttribs(m_ResourceNames, TexAttribs, SamplerId);
        },

        ShdrDesc,
        D3DSamplerSuffix);

    VERIFY_EXPR(m_ResourceNames.GetRemainingSize() == 0);
    VERIFY(CurrCB == GetNumCBs(), "Not all CBs are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called");
    VERIFY(CurrTexSRV == GetNumTexSRV(), "Not all Tex SRVs are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called" );
    VERIFY(CurrTexUAV == GetNumTexUAV(), "Not all Tex UAVs are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called" );
    VERIFY(CurrBufSRV == GetNumBufSRV(), "Not all Buf SRVs are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called" );
    VERIFY(CurrBufUAV == GetNumBufUAV(), "Not all Buf UAVs are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called" );
    VERIFY(CurrSampler == GetNumSamplers(), "Not all Samplers are initialized, which will result in a crash when ~D3DShaderResourceAttribs() is called" );
}

}
