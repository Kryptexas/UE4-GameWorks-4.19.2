// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"
#include "Slate.h"
#include "EditorStyle.h"

class FGammaUI : public IModuleInterface
{

public:

	// IModuleInterface implementation.
	virtual void StartupModule();
	virtual void ShutdownModule();

	/** @return a pointer to the Gamma UI panel; invalid pointer if one has already been allocated */
	virtual TSharedPtr< class SWidget > GetGammaUIPanel();
};
