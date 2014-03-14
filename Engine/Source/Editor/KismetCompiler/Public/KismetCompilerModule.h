// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"

class FCompilerResultsLog;

#define KISMET_COMPILER_MODULENAME "KismetCompiler"

//////////////////////////////////////////////////////////////////////////
// IKismetCompilerInterface

class IKismetCompilerInterface : public IModuleInterface
{
public:
	/**
	 * Compiles a blueprint.
	 *
	 * @param	Blueprint	The blueprint to compile.
	 * @param	Results  	The results log for warnings and errors.
	 * @param	Options		Compiler options.
	 */
	virtual void CompileBlueprint(class UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results, class FBlueprintCompileReinstancer* ParentReinstancer = NULL)=0;

	/**
	 * Attempts to recover a corrupted blueprint package.
	 *
	 * @param	Blueprint	The blueprint to recover.
	 */
	virtual void RecoverCorruptedBlueprint(class UBlueprint* Blueprint)=0;

	/**
	 * Clears the blueprint's generated classes, and consigns them to oblivion
	 *
	 * @param	Blueprint	The blueprint to clear the classes for
	 */
	virtual void RemoveBlueprintGeneratedClasses(class UBlueprint* Blueprint)=0;
};

//////////////////////////////////////////////////////////////////////////
// FKismet2CompilerModule

/**
 * The Kismet 2 Compiler module
 */
class FKismet2CompilerModule : public IKismetCompilerInterface
{
public:
	// Implementation of the IKismetCompilerInterface
	virtual void CompileBlueprint(class UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results, class FBlueprintCompileReinstancer* ParentReinstancer = NULL) OVERRIDE;
	virtual void RecoverCorruptedBlueprint(class UBlueprint* Blueprint) OVERRIDE;
	virtual void RemoveBlueprintGeneratedClasses(class UBlueprint* Blueprint) OVERRIDE;
	// End implementation
private:
	void CompileBlueprintInner(class UBlueprint* Blueprint, bool bPrintResultSuccess, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results);
};

