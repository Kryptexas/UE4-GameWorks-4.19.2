// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VisualStudioSourceCodeAccessor.h"

class FVisualStudioSourceCodeAccessModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	FVisualStudioSourceCodeAccessor& GetAccessor();

private:
	FVisualStudioSourceCodeAccessor VisualStudioSourceCodeAccessor;
};