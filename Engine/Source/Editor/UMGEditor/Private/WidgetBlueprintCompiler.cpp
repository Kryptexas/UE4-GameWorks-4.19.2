// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintCompiler.h"
#include "Kismet2NameValidators.h"
#include "KismetReinstanceUtilities.h"

FWidgetBlueprintCompiler::FWidgetBlueprintCompiler(UWidgetBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: Super(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{
}

FWidgetBlueprintCompiler::~FWidgetBlueprintCompiler()
{
}

void FWidgetBlueprintCompiler::ValidateWidgetNames()
{
	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	TSharedPtr<FKismetNameValidator> ParentBPNameValidator;
	if ( Blueprint->ParentClass != NULL )
	{
		UBlueprint* ParentBP = Cast<UBlueprint>(Blueprint->ParentClass->ClassGeneratedBy);
		if ( ParentBP != NULL )
		{
			ParentBPNameValidator = MakeShareable(new FKismetNameValidator(ParentBP));
		}
	}

	for ( USlateWrapperComponent* Widget : Blueprint->WidgetTemplates )
	{
		if ( ParentBPNameValidator.IsValid() && ParentBPNameValidator->IsValid(Widget->GetName()) != EValidatorResult::Ok )
		{
			// TODO Support renaming items, similar to timelines.

			//// Use the viewer displayed Timeline name (without the _Template suffix) because it will be added later for appropriate checks.
			//FString TimelineName = UTimelineTemplate::TimelineTemplateNameToVariableName(TimelineTemplate->GetFName());

			//FName NewName = FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, TimelineName);
			//MessageLog.Warning(*FString::Printf(*LOCTEXT("TimelineConflictWarning", "Found a timeline with a conflicting name (%s) - changed to %s.").ToString(), *TimelineTemplate->GetName(), *NewName.ToString()));
			//FBlueprintEditorUtils::RenameTimeline(Blueprint, FName(*TimelineName), NewName);
		}
	}
}

template<typename TOBJ>
struct FCullTemplateObjectsHelper
{
	const TArray<TOBJ*>& Templates;

	FCullTemplateObjectsHelper(const TArray<TOBJ*>& InComponentTemplates)
		: Templates(InComponentTemplates)
	{}

	bool operator()(const UObject* const RemovalCandidate) const
	{
		return ( NULL != Templates.FindByKey(RemovalCandidate) );
	}
};

void FWidgetBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, OldCDO);

	// Purge all subobjects (properties, functions, params) of the class, as they will be regenerated
	TArray<UObject*> ClassSubObjects;
	GetObjectsWithOuter(ClassToClean, ClassSubObjects, true);

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	if ( 0 != Blueprint->Timelines.Num() )
	{
		ClassSubObjects.RemoveAllSwap(FCullTemplateObjectsHelper<USlateWrapperComponent>(Blueprint->WidgetTemplates));
	}
}

void FWidgetBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	ValidateWidgetNames();

	int32 i = 0;
	for ( USlateWrapperComponent* Widget : Blueprint->WidgetTemplates )
	{
		FString VariableName = Widget->GetClass()->GetName() + TEXT("_") + FString::FromInt(i);

		FEdGraphPinType WidgetPinType(Schema->PC_Object, TEXT(""), Widget->GetClass(), false, false);
		UProperty* WidgetProperty = CreateVariable(FName(*VariableName), WidgetPinType);
		if ( WidgetProperty != NULL )
		{
			WidgetProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
			WidgetProperty->SetPropertyFlags(CPF_BlueprintVisible);

			WidgetToMemberVariableMap.Add(Widget, WidgetProperty);
		}

		i++;
	}

	printf("Compile");
}

void FWidgetBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	Super::FinishCompilingClass(Class);

	UClass* ParentClass = Class->GetSuperClass();
	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	{
		UWidgetBlueprintGeneratedClass* BPGClass = CastChecked<UWidgetBlueprintGeneratedClass>(Class);

		BPGClass->WidgetTemplates.Empty();
		BPGClass->WidgetTemplates = Blueprint->WidgetTemplates;
	}
}

void FWidgetBlueprintCompiler::Compile()
{
	Super::Compile();

	WidgetToMemberVariableMap.Empty();
}

void FWidgetBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if ( TargetUClass && !( (UObject*)TargetUClass )->IsA(UWidgetBlueprintGeneratedClass::StaticClass()) )
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = NULL;
	}
}

void FWidgetBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewWidgetBlueprintClass = FindObject<UWidgetBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if ( NewWidgetBlueprintClass == NULL )
	{
		NewWidgetBlueprintClass = ConstructObject<UWidgetBlueprintGeneratedClass>(UWidgetBlueprintGeneratedClass::StaticClass(), Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer GeneratedClassReinstancer(NewWidgetBlueprintClass);
	}
	NewClass = NewWidgetBlueprintClass;
}
