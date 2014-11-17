// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateDynamicImageBrush.cpp: Implements the FSlateDynamicImageBrush structure.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FSlateDynamicImageBrush structors
 *****************************************************************************/

FSlateDynamicImageBrush::~FSlateDynamicImageBrush( )
{
	if (FSlateApplicationBase::IsInitialized())
	{
		TSharedPtr<FSlateRenderer> Renderer = FSlateApplicationBase::Get().GetRenderer();

		if (Renderer.IsValid())
		{
			FSlateApplicationBase::Get().GetRenderer()->ReleaseDynamicResource(*this);
		}
	}
}
