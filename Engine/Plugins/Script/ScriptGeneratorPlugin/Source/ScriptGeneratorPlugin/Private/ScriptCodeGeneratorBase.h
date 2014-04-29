// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
* Base for script code generators.
* Contains utility functions that may be common to more specialized script glue generators.
*/
class FScriptCodeGeneratorBase
{
	/** List of temporary header files crated by SaveHeaderIfChanged */
	TArray<FString> TempHeaders;

protected:

	/** Path where generated script glue goes **/
	FString GeneratedCodePath;
	/** Local root path **/
	FString RootLocalPath;
	/** Build root path - may be different to RootBuildPath if we're building remotely. **/
	FString RootBuildPath;
	/** Set of all exported class names */
	TSet<FName> ExportedClasses;

	/** Default ctor */
	FScriptCodeGeneratorBase(const FString& InRootLocalPath, const FString& InRootBuildPath, const FString& OutputDirectory);

	/** Saves generated script glue heade to a temporary file if its contents is different from the eexisting one. */
	bool SaveHeaderIfChanged(const FString& HeaderPath, const FString& NewHeaderContents);
	/** Renames/replaces all existing script glue files with the temporary (new) ones */
	void RenameTempFiles();
	/** Re-bases the local path to build path */
	FString RebaseToBuildPath(const FString& FileName) const;
	/** Converts a UClass name to C++ class name (with U/A prefix) */
	FString GetClassNameCPP(UClass* Class);
	/** Generates a script glue header filename for the specified class */
	virtual FString GetScriptHeaderForClass(UClass* Class);
	/** Returns true if the specified class can be exported */
	virtual bool CanExportClass(UClass* Class);
	/** Returns true if the specified function can be exported */
	virtual bool CanExportFunction(const FString& ClassNameCPP, UClass* Class, UFunction* Function);
	/** Returns true if the specified property can be exported */
	virtual bool CanExportProperty(const FString& ClassNameCPP, UClass* Class, UProperty* Property);
	/** Generates a lookup table for exported proeprties. */
	virtual int32 FScriptCodeGeneratorBase::GenerateExportedPropertyTable(const FString& ClassNameCPP, UClass* Class, FString& OutFunction, TArray<UProperty*>& OutExportedProperties);

public:

	// IScriptGeneratorPlugin interface
	virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) = 0;
	virtual void FinishExport() = 0;

};
