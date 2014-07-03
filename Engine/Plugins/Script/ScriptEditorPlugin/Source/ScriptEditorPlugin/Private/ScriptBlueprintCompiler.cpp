// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorPluginPrivatePCH.h"
#include "ScriptBlueprint.h"
#include "ScriptBlueprintGeneratedClass.h"
#include "ScriptBlueprintCompiler.h"
#include "Kismet2NameValidators.h"
#include "KismetReinstanceUtilities.h"
#include "ScriptFunction.h"

///-------------------------------------------------------------

FScriptBlueprintCompiler::FScriptBlueprintCompiler(UScriptBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: Super(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{
}

FScriptBlueprintCompiler::~FScriptBlueprintCompiler()
{
}

void FScriptBlueprintCompiler::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	Super::CopyTermDefaultsToDefaultObject(DefaultObject);
}

void FScriptBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, OldCDO);

	// Make sure our typed pointer is set
	check(ClassToClean == NewClass);
	NewScriptBlueprintClass = CastChecked<UScriptBlueprintGeneratedClass>((UObject*)NewClass);
}

void FScriptBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();

	UScriptBlueprint* Blueprint = ScriptBlueprint();
	UScriptBlueprintGeneratedClass* NewScripClass = CastChecked<UScriptBlueprintGeneratedClass>(NewClass);
	NewScripClass->ScriptProperties.Empty();

	for (auto& Field : ScriptDefinedFields)
	{
		if (Field.Class->IsChildOf(UFunction::StaticClass()))
		{
			// Functions are currently only supported for AScriptActors
			if (Blueprint->ParentClass->IsChildOf(AScriptActor::StaticClass()))
			{
				UFunction* NewFunction = new(NewClass, Field.Name, RF_Public) UScriptFunction(FPostConstructInitializeProperties());
				NewFunction->Bind();
				NewFunction->StaticLink();
				NewClass->AddFunctionToFunctionMap(NewFunction);
				NewFunction->Next = NewClass->Children;
				NewClass->Children = NewFunction;
			}
		}
		else if (Field.Class->IsChildOf(UProperty::StaticClass()))
		{
			FString PinCategory;
			if (Field.Class->IsChildOf(UStrProperty::StaticClass()))
			{
				PinCategory = Schema->PC_String;
			}
			else if (Field.Class->IsChildOf(UFloatProperty::StaticClass()))
			{
				PinCategory = Schema->PC_Float;
			}
			else if (Field.Class->IsChildOf(UIntProperty::StaticClass()))
			{
				PinCategory = Schema->PC_Int;
			}
			else if (Field.Class->IsChildOf(UBoolProperty::StaticClass()))
			{
				PinCategory = Schema->PC_Boolean;
			}
			else if (Field.Class->IsChildOf(UObjectProperty::StaticClass()))
			{
				PinCategory = Schema->PC_Object;
			}
			if (!PinCategory.IsEmpty())
			{			
				FEdGraphPinType ScriptPinType(PinCategory, TEXT(""), Field.Class, false, false);
				UProperty* ScriptProperty = CreateVariable(Field.Name, ScriptPinType);
				if (ScriptProperty != NULL)
				{
					ScriptProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
					ScriptProperty->SetPropertyFlags(CPF_BlueprintVisible | CPF_Edit);					
					NewScripClass->ScriptProperties.Add(ScriptProperty);
				}
			}
		}
	}
}

void FScriptBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UScriptBlueprint* Blueprint = ScriptBlueprint();

	UScriptBlueprintGeneratedClass* ScriptClass = CastChecked<UScriptBlueprintGeneratedClass>(Class);
	ScriptClass->SourceCode = Blueprint->SourceCode;
	ScriptClass->ByteCode = Blueprint->ByteCode;

	Super::FinishCompilingClass(Class);

	// Allow Blueprint Components to be used in Blueprints
	if (Blueprint->ParentClass->IsChildOf(UScriptComponent::StaticClass()) && Class != Blueprint->SkeletonGeneratedClass)
	{
		Class->SetMetaData(TEXT("BlueprintSpawnableComponent"), TEXT("true"));
	}
}

void FScriptBlueprintCompiler::Compile()
{
	ScriptContext = NewScriptBlueprintClass->CreateContext();
	bool Result = true;
	if (ScriptContext.IsValid())
	{
		ScriptBlueprint()->UpdateSourceCodeIfChanged();
		Result = ScriptContext->Initialize(ScriptBlueprint()->SourceCode, NULL);
		if (!Result)
		{
			ScriptContext.Reset();
		}
	}
	if (Result)
	{
		ScriptDefinedFields.Empty();
		ScriptContext->GetScriptDefinedFields(ScriptDefinedFields);
	}

	Super::Compile();
}

void FScriptBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if ( TargetUClass && !( (UObject*)TargetUClass )->IsA(UScriptBlueprintGeneratedClass::StaticClass()) )
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = NULL;
	}
}

void FScriptBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewScriptBlueprintClass = FindObject<UScriptBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if ( NewScriptBlueprintClass == NULL )
	{
		NewScriptBlueprintClass = ConstructObject<UScriptBlueprintGeneratedClass>(UScriptBlueprintGeneratedClass::StaticClass(), Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer GeneratedClassReinstancer(NewScriptBlueprintClass);
	}
	NewClass = NewScriptBlueprintClass;
}

bool FScriptBlueprintCompiler::ValidateGeneratedClass(UBlueprintGeneratedClass* Class)
{
	bool SuperResult = Super::ValidateGeneratedClass(Class);
	bool Result = UScriptBlueprint::ValidateGeneratedClass(Class);
	return SuperResult && Result;
}
