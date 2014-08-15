// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "Animation/AnimNodeBase.h"
#include "WidgetBlueprintCompiler.h"
#include "Kismet2NameValidators.h"
#include "KismetReinstanceUtilities.h"
#include "MovieScene.h"
#include "K2Node_FunctionEntry.h"


///-------------------------------------------------------------

FWidgetBlueprintCompiler::FWidgetBlueprintCompiler(UWidgetBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: Super(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{
}

FWidgetBlueprintCompiler::~FWidgetBlueprintCompiler()
{
}

void FWidgetBlueprintCompiler::CreateFunctionList()
{
	Super::CreateFunctionList();

	for ( FDelegateEditorBinding& EditorBinding : WidgetBlueprint()->Bindings )
	{
		FName PropertyName = EditorBinding.SourceProperty;

		UProperty* Property = FindField<UProperty>(Blueprint->SkeletonGeneratedClass, PropertyName);
		if ( Property )
		{
			// Create the function graph.
			FString FunctionName = FString(TEXT("__Get")) + PropertyName.ToString();
			UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, FunctionName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

			// Update the function binding to match the generated graph name
			EditorBinding.FunctionName = FunctionGraph->GetFName();

			const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(FunctionGraph->GetSchema());

			Schema->CreateDefaultNodesForGraph(*FunctionGraph);

			// We need to flag the entry node to make sure that the compiled function is callable from Kismet2
			int32 ExtraFunctionFlags = ( FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public );
			if ( BPTYPE_FunctionLibrary == Blueprint->BlueprintType )
			{
				ExtraFunctionFlags |= FUNC_Static;
			}
			K2Schema->AddExtraFunctionFlags(FunctionGraph, ExtraFunctionFlags);
			K2Schema->MarkFunctionEntryAsEditable(FunctionGraph, true);

			// Create a function entry node
			FGraphNodeCreator<UK2Node_FunctionEntry> FunctionEntryCreator(*FunctionGraph);
			UK2Node_FunctionEntry* EntryNode = FunctionEntryCreator.CreateNode();
			EntryNode->SignatureClass = NULL;
			EntryNode->SignatureName = FunctionGraph->GetFName();
			FunctionEntryCreator.Finalize();

			FGraphNodeCreator<UK2Node_FunctionResult> FunctionReturnCreator(*FunctionGraph);
			UK2Node_FunctionResult* ReturnNode = FunctionReturnCreator.CreateNode();
			ReturnNode->SignatureClass = NULL;
			ReturnNode->SignatureName = FunctionGraph->GetFName();
			ReturnNode->NodePosX = EntryNode->NodePosX + EntryNode->NodeWidth + 256;
			ReturnNode->NodePosY = EntryNode->NodePosY;
			FunctionReturnCreator.Finalize();

			UProperty* Property = FindField<UProperty>(Blueprint->SkeletonGeneratedClass, PropertyName);

			FEdGraphPinType PinType;
			K2Schema->ConvertPropertyToPinType(Property, /*out*/ PinType);

			UEdGraphPin* ReturnPin = ReturnNode->CreateUserDefinedPin(TEXT("ReturnValue"), PinType);

			// Auto-connect the pins for entry and exit, so that by default the signature is properly generated
			UEdGraphPin* EntryNodeExec = K2Schema->FindExecutionPin(*EntryNode, EGPD_Output);
			UEdGraphPin* ResultNodeExec = K2Schema->FindExecutionPin(*ReturnNode, EGPD_Input);
			EntryNodeExec->MakeLinkTo(ResultNodeExec);

			FGraphNodeCreator<UK2Node_VariableGet> MemberGetCreator(*FunctionGraph);
			UK2Node_VariableGet* VarNode = MemberGetCreator.CreateNode();
			VarNode->VariableReference.SetSelfMember(PropertyName);
			MemberGetCreator.Finalize();

			ReturnPin->MakeLinkTo(VarNode->GetValuePin());

			//Blueprint->FunctionGraphs.Add(FunctionGraph);

			ProcessOneFunctionGraph(FunctionGraph);
			//FEdGraphUtilities::MergeChildrenGraphsIn(Ubergraph, FunctionGraph, /*bRequireSchemaMatch=*/ true);
		}
	}
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

	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);
	for ( UWidget* Widget : Widgets )
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

	// Make sure our typed pointer is set
	check(ClassToClean == NewClass);
	NewWidgetBlueprintClass = CastChecked<UWidgetBlueprintGeneratedClass>((UObject*)NewClass);

	//// Purge all subobjects (properties, functions, params) of the class, as they will be regenerated
	//TArray<UObject*> ClassSubObjects;
	//GetObjectsWithOuter(ClassToClean, ClassSubObjects, true);

	//UWidgetBlueprint* Blueprint = WidgetBlueprint();

	//if ( 0 != Blueprint->WidgetTree->WidgetTemplates.Num() )
	//{
	//	ClassSubObjects.RemoveAllSwap(FCullTemplateObjectsHelper<UWidget>(Blueprint->WidgetTree->WidgetTemplates));
	//}
}

void FWidgetBlueprintCompiler::SaveSubObjectsFromCleanAndSanitizeClass(FSubobjectCollection& SubObjectsToSave, UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO)
{
	// Make sure our typed pointer is set
	check(ClassToClean == NewClass);
	NewWidgetBlueprintClass = CastChecked<UWidgetBlueprintGeneratedClass>((UObject*)NewClass);

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	SubObjectsToSave.AddObject(Blueprint->WidgetTree);

	SubObjectsToSave.AddObject( NewWidgetBlueprintClass->WidgetTree );
}

void FWidgetBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	ValidateWidgetNames();

	// Build the set of variables based on the variable widgets in the widget tree.
	TArray<UWidget*> Widgets;
	Blueprint->WidgetTree->GetAllWidgets(Widgets);

	// Sort the widgets alphabetically
	{
		struct FWidgetSorter
		{
			bool operator()(const UWidget& A, const UWidget& B) const
			{
				return B.GetFName() < A.GetFName();
			}
		};
		Widgets.Sort(FWidgetSorter());
	}

	for ( UWidget* Widget : Widgets )
	{
		// Skip non-variable widgets
		if ( !Widget->bIsVariable )
		{
			continue;
		}

		FString VariableName = Widget->GetName();

		FEdGraphPinType WidgetPinType(Schema->PC_Object, TEXT(""), Widget->GetClass(), false, false);
		UProperty* WidgetProperty = CreateVariable(FName(*VariableName), WidgetPinType);
		if ( WidgetProperty != NULL )
		{
			WidgetProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
			//WidgetProperty->SetMetaData(TEXT("Category"), TEXT("Widget"));
			WidgetProperty->SetPropertyFlags(CPF_BlueprintVisible);

			WidgetToMemberVariableMap.Add(Widget, WidgetProperty);
		}
	}

	// Add movie scenes variables here
	for(UWidgetAnimation* Animation : Blueprint->Animations)
	{
		FString VariableName = Animation->GetName();

		FEdGraphPinType WidgetPinType(Schema->PC_Object, TEXT(""), Animation->GetClass(), false, true);
		UProperty* WidgetProperty = CreateVariable(FName(*VariableName), WidgetPinType);

		if(WidgetProperty != NULL)
		{
			WidgetProperty->SetMetaData(TEXT("Category"), TEXT("Animations") );

			WidgetProperty->SetPropertyFlags(CPF_BlueprintVisible);
		}
	}
}

void FWidgetBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	UWidgetBlueprintGeneratedClass* BPGClass = CastChecked<UWidgetBlueprintGeneratedClass>(Class);

	BPGClass->WidgetTree = DuplicateObject<UWidgetTree>(Blueprint->WidgetTree, BPGClass);

	BPGClass->Animations.Empty();

	// Dont duplicate animation data on the skeleton class
	if( Blueprint->SkeletonGeneratedClass != Class )
	{
		for(const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			UWidgetAnimation* ClonedAnimation = DuplicateObject<UWidgetAnimation>(Animation, BPGClass, *(Animation->GetName()+TEXT("_INST")));

			BPGClass->Animations.Add(ClonedAnimation);
		}
	}

	BPGClass->Bindings.Reset();

	// @todo UMG Possibly need duplication here 
	for ( const FDelegateEditorBinding& EditorBinding : Blueprint->Bindings )
	{
		BPGClass->Bindings.Add(EditorBinding.ToRuntimeBinding(Blueprint));
	}

	Super::FinishCompilingClass(Class);
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

bool FWidgetBlueprintCompiler::ValidateGeneratedClass(UBlueprintGeneratedClass* Class)
{
	bool SuperResult = Super::ValidateGeneratedClass(Class);
	bool Result = UWidgetBlueprint::ValidateGeneratedClass(Class);

	return SuperResult && Result;
}
