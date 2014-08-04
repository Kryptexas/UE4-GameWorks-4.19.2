// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"

class FKCHandler_TemporaryVariable : public FNodeHandlingFunctor
{
public:
	FKCHandler_TemporaryVariable(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) override
	{
		// This net is an anonymous temporary variable
		FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();

		FString NetName = Context.NetNameMap->MakeValidName(Net);

		Term->CopyFromPin(Net, NetName);

		UK2Node_TemporaryVariable* TempVarNode = CastChecked<UK2Node_TemporaryVariable>(Net->GetOwningNode());
		Term->bIsSavePersistent = TempVarNode->bIsPersistent;

		Context.NetMap.Add(Net, Term);
	}
};

UK2Node_TemporaryVariable::UK2Node_TemporaryVariable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bIsPersistent(false)
{
}

void UK2Node_TemporaryVariable::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* VariablePin = CreatePin(EGPD_Output, TEXT(""), TEXT(""), NULL, false, false, TEXT("Variable"));
	VariablePin->PinType = VariableType;

	Super::AllocateDefaultPins();
}

FString UK2Node_TemporaryVariable::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("VariableType"), UEdGraphSchema_K2::TypeToText(VariableType));
	return FText::Format(NSLOCTEXT("K2Node", "LocalTemporaryVariable", "Local temporary {VariableType} variable"), Args).ToString();
}

FText UK2Node_TemporaryVariable::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Result = !bIsPersistent ? NSLOCTEXT("K2Node", "LocalVariable", "Local {VariableType}") : NSLOCTEXT("K2Node", "PersistentLocalVariable", "Persistent Local {VariableType}");

	FFormatNamedArguments Args;
	Args.Add(TEXT("VariableType"), UEdGraphSchema_K2::TypeToText(VariableType));
	Result = FText::Format(Result, Args);
	return Result;
}

bool UK2Node_TemporaryVariable::IsNodePure() const
{
	return true;
}

FString UK2Node_TemporaryVariable::GetDescriptiveCompiledName() const
{
	FString Result = NSLOCTEXT("K2Node", "TempPinCategory", "Temp_").ToString() + VariableType.PinCategory;
		
	if (!NodeComment.IsEmpty())
	{
		Result += TEXT("_");
		Result += NodeComment;
	}

	// If this node is persistent, we need to add the NodeGuid, which should be propagated from the macro that created this, in order to ensure persistence 
	if (bIsPersistent)
	{
		Result += TEXT("_");
		Result += NodeGuid.ToString();
	}

	return Result;
}

bool UK2Node_TemporaryVariable::IsCompatibleWithGraph(UEdGraph const* TargetGraph) const
{
	bool bIsCompatible = Super::IsCompatibleWithGraph(TargetGraph);
	if (bIsCompatible)
	{
		EGraphType const GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
		bIsCompatible = (GraphType != GT_Ubergraph) && (GraphType != GT_Animation);

		if (bIsCompatible && (GraphType != GT_Macro))
		{
			bIsCompatible = !bIsPersistent;
		}
	}

	return bIsCompatible;
}

// get variable pin
UEdGraphPin* UK2Node_TemporaryVariable::GetVariablePin()
{
	return FindPin(TEXT("Variable"));
}

FNodeHandlingFunctor* UK2Node_TemporaryVariable::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_TemporaryVariable(CompilerContext);
}

void UK2Node_TemporaryVariable::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	auto MakeTempVarNodeSpawner = [](FEdGraphPinType const& VarType, bool bIsPersistent) -> UBlueprintNodeSpawner*
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(UK2Node_TemporaryVariable::StaticClass());
		check(NodeSpawner != nullptr);

		auto PostSpawnLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FEdGraphPinType VarType, bool bIsPersistent)
		{
			UK2Node_TemporaryVariable* TempVarNode = CastChecked<UK2Node_TemporaryVariable>(NewNode);
			TempVarNode->VariableType  = VarType;
			TempVarNode->bIsPersistent = bIsPersistent;
		};

		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnLambda, VarType, bIsPersistent);
		return NodeSpawner;
	};

	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();

	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Int,     TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Int,     TEXT(""), nullptr, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Float,   TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Float,   TEXT(""), nullptr, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Boolean, TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Boolean, TEXT(""), nullptr, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_String,  TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_String,  TEXT(""), nullptr, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Text,    TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Text,    TEXT(""), nullptr, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));

	UScriptStruct* VectorStruct  = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Vector"), VectorStruct, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Vector"), VectorStruct, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	
	UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Rotator"));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Rotator"), RotatorStruct, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Rotator"), RotatorStruct, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));
	
	UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Transform"), TransformStruct, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("Transform"), TransformStruct, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));

	UScriptStruct* BlendSampleStruct = FindObjectChecked<UScriptStruct>(ANY_PACKAGE, TEXT("BlendSampleData"));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("BlendSampleData"), BlendSampleStruct, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/false));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Struct, TEXT("BlendSampleData"), BlendSampleStruct, /*bIsArray =*/ true, /*bIsReference =*/false), /*bIsPersistent =*/false));

	// add persistent bool and int types (for macro graphs)
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Int,     TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/true));
	ActionListOut.Add(MakeTempVarNodeSpawner(FEdGraphPinType(K2Schema->PC_Boolean, TEXT(""), nullptr, /*bIsArray =*/false, /*bIsReference =*/false), /*bIsPersistent =*/true));
}

FText UK2Node_TemporaryVariable::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Macro);
}
