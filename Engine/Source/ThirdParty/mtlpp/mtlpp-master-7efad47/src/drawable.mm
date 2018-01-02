/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLDrawable.h>
#include "drawable.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    double Drawable::GetPresentedTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
		return m_table->PresentedTime(m_ptr);
#else
        return 0.0;
#endif
    }

    uint64_t Drawable::GetDrawableID() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
		return m_table->DrawableID(m_ptr);
#else
        return 0;
#endif
    }

    void Drawable::Present()
    {
        Validate();
		m_table->Present(m_ptr);
    }

    void Drawable::PresentAtTime(double presentationTime)
    {
        Validate();
		m_table->PresentAtTime(m_ptr, presentationTime);
    }

    void Drawable::PresentAfterMinimumDuration(double duration)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
		m_table->PresentAfterMinimumDuration(m_ptr, duration);
#endif
    }

    void Drawable::AddPresentedHandler(PresentHandler handler)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
		ITable* table = m_table;
		m_table->AddPresentedHandler(m_ptr, ^(id <MTLDrawable> mtlDrawable){
			Drawable drawable(mtlDrawable, table);
			handler(drawable);
		});
#endif
    }

}

MTLPP_END
