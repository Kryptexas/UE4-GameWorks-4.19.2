// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ScriptCodeGeneratorBase.h"

/**
 * Lua glue generator. 
 */
class FLuaScriptCodeGenerator : public FScriptCodeGeneratorBase
{
protected:

	/** All generated script header filenames */
	TArray<FString> AllScriptHeaders;
	/** Source header filenames for all exported classes */
	TArray<FString> AllSourceClassHeaders;
	/** All exported classes */
	TArray<UClass*> LuaExportedClasses;
	/** Functions exported for a class */
	TMap<UClass*, TArray<FName> > ClassExportedFunctions;

	/** Creats a 'glue' file that merges all generated script files */
	void GlueAllGeneratedFiles();

	/** Exports a wrapper function */
	FString ExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function);
	/** Exports a wrapper functions for properties */
	FString ExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property);
	/** Exports additional class glue code (like ctor/dtot) */
	FString ExportAdditionalClassGlue(const FString& ClassNameCPP, UClass* Class);
	/** Generates wrapper function declaration */
	FString GenerateWrapperFunctionDeclaration(const FString& ClassNameCPP, UClass* Class, UFunction* Function);
	/** Generates wrapper function declaration */
	FString GenerateWrapperFunctionDeclaration(const FString& ClassNameCPP, UClass* Class, const FString& FunctionName);
	/** Generates function parameter declaration */
	FString GenerateFunctionParamDeclaration(const FString& ClassNameCPP, UClass* Class, UFunction* Function, UProperty* Param, int32 ParamIndex);
	/** Generates code responsible for getting the object pointer from script context */
	FString GenerateObjectDeclarationFromContext(const FString& ClassNameCPP, UClass* Class);
	/** Handles the wrapped function's return value */
	FString GenerateReturnValueHandler(const FString& ClassNameCPP, UClass* Class, UFunction* Function, UProperty* ReturnValue);

	// FScriptCodeGeneratorBase interface
	virtual bool CanExportClass(UClass* Class) OVERRIDE;
	virtual bool CanExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function) OVERRIDE;

public:

	FLuaScriptCodeGenerator(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory);

	// FScriptCodeGeneratorBase interface
	virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) OVERRIDE;
	virtual void FinishExport() OVERRIDE;
};
