// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleManager.h"

/**
 * The public interface to script generator plugins.
 */
class IScriptGeneratorPluginInterface : public IModuleInterface
{
public:

	virtual FString GetGeneratedCodeModuleName() const = 0;
	virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory) = 0;
	virtual void ExportClass(class UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) = 0;
	virtual void FinishExport() = 0;
};

