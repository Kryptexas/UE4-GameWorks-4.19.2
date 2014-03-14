// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"
#include "EditorStyle.h"
#include "ModuleInterface.h"


/**
 * Slate toolbox module public interface
 */
class FToolboxModule : public IModuleInterface
{

public:
	virtual void StartupModule();
	virtual void ShutdownModule();
};
