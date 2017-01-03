// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IViewportWorldInteractionManager
{
public:
	
	/** 
	 * Sets the current ViewportWorldInteraction
	 *
	 * @param UViewportWorldInteraction the next ViewportWorldInteraction used
	 */
	virtual void SetCurrentViewportWorldInteraction( class UViewportWorldInteraction* /*WorldInteraction*/ ) = 0;

};
