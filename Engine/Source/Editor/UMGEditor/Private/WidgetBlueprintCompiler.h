// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// FWidgetBlueprintCompiler

class FWidgetBlueprintCompiler : public FKismetCompilerContext
{
protected:
	typedef FKismetCompilerContext Super;

public:
	FWidgetBlueprintCompiler(UWidgetBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded);
	virtual ~FWidgetBlueprintCompiler();

	// FKismetCompilerContext
	virtual void Compile() OVERRIDE;
	// End FKismetCompilerContext

protected:
	UWidgetBlueprint* WidgetBlueprint() const { return Cast<UWidgetBlueprint>(Blueprint); }

	void ValidateWidgetNames();

	// FKismetCompilerContext
	virtual void SpawnNewClass(const FString& NewClassName) OVERRIDE;
	virtual void EnsureProperGeneratedClass(UClass*& TargetClass) OVERRIDE;
	virtual void CreateClassVariablesFromBlueprint() OVERRIDE;
	virtual void FinishCompilingClass(UClass* Class) OVERRIDE;
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO) OVERRIDE;
	virtual bool ValidateGeneratedClass(UBlueprintGeneratedClass* Class) OVERRIDE;
	// End FKismetCompilerContext

protected:
	UWidgetBlueprintGeneratedClass* NewWidgetBlueprintClass;
	//UWidgetBlueprint* AnimBlueprint;

	// Map of properties created for widgets; to aid in debug data generation
	TMap<class USlateWrapperComponent*, class UProperty*> WidgetToMemberVariableMap;

};

