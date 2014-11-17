// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleManager.h"

/**
 * The public interface to script generator plugins.
 */
class IScriptGeneratorPluginInterface : public IModuleInterface
{
public:

	/** Name of module that is going to be compiling generated script glue */
	virtual FString GetGeneratedCodeModuleName() const = 0;
	/** Returns true if this plugin supports exporting scripts for the specified target. This should handle game as well as editor target names */
	virtual bool SupportsTarget(const FString& TargetName) const = 0;
	/** Initializes this plugin with build information */
	virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory) = 0;
	/** Exports a single class. May be called multiple times for the same class (as UHT processes the entire hierarchy inside modules. */
	virtual void ExportClass(class UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) = 0;
	/** Called once all classes have been exported */
	virtual void FinishExport() = 0;

};

