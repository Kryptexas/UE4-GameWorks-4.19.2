// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ModuleInterface.h"

class FAdjustEditorModule :
	public IModuleInterface
{
private:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

