// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

class FUserDefinedStructureCompilerUtils
{
public:
	// ASSUMPTION, no structure needs to be renamed or removed
	static void CompileStructs(class UBlueprint* Blueprint, class FCompilerResultsLog& MessageLog, bool bForceRecompileAll);

	// Default values for member variables in User Defined Structure are stored in meta data "MakeStructureDefaultValue"
	static void DefaultUserDefinedStructs(UObject* Object, FCompilerResultsLog& MessageLog);
};

