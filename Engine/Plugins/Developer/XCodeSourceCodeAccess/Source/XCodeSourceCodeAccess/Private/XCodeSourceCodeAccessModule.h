// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "XCodeSourceCodeAccessor.h"

class FXCodeSourceCodeAccessModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FXCodeSourceCodeAccessor XCodeSourceCodeAccessor;
};