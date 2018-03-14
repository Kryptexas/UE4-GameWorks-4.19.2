/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLParallelRenderCommandEncoder.h>
#include "parallel_render_command_encoder.hpp"
#include "render_command_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    RenderCommandEncoder ParallelRenderCommandEncoder::GetRenderCommandEncoder()
    {
        Validate();
		return m_table->RenderCommandEncoder(m_ptr);
    }

    void ParallelRenderCommandEncoder::SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->SetcolorstoreactionAtindex(m_ptr, MTLStoreAction(storeAction), colorAttachmentIndex);
#endif
    }

    void ParallelRenderCommandEncoder::SetDepthStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Setdepthstoreaction(m_ptr, MTLStoreAction(storeAction));
#endif
    }

    void ParallelRenderCommandEncoder::SetStencilStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->Setstencilstoreaction(m_ptr, MTLStoreAction(storeAction));
#endif
    }
	
	void ParallelRenderCommandEncoder::SetColorStoreActionOptions(StoreActionOptions options, NSUInteger colorAttachmentIndex)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->SetcolorstoreactionoptionsAtindex(m_ptr, MTLStoreActionOptions(options), colorAttachmentIndex);
#endif
	}
	
	void ParallelRenderCommandEncoder::SetDepthStoreActionOptions(StoreActionOptions options)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setdepthstoreactionoptions(m_ptr, MTLStoreActionOptions(options));
#endif
	}
	
	void ParallelRenderCommandEncoder::SetStencilStoreActionOptions(StoreActionOptions options)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_table->Setstencilstoreactionoptions(m_ptr, MTLStoreActionOptions(options));
#endif
	}
}

MTLPP_END
