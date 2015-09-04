// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward declares
struct FBlueprintNativeCodeGenCoordinator;

/**  */
struct FBlueprintNativeCodeGenUtils
{
public: 
	/**  */
	static bool GenerateCodeModule(FBlueprintNativeCodeGenCoordinator& Coordinator);

	/** 
	 * Recompiles the bytecode of a blueprint only. Should only be run for 
	 * recompiling dependencies during compile on load. 
	 */
	static void GenerateCppCode(UObject* Obj, TSharedPtr<FString> OutHeaderSource, TSharedPtr<FString> OutCppSource);
};

