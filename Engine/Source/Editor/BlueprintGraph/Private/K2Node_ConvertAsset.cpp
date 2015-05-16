// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "Kismet/KismetSystemLibrary.h"
#include "K2Node_ClassDynamicCast.h"
#include "KismetCompiler.h"
#include "K2Node_ConvertAsset.h"

#define LOCTEXT_NAMESPACE "K2Node_ConvertAsset"

namespace UK2Node_ConvertAssetImpl
{
	static const FString InputPinName("Asset");
	static const FString OutputPinName("Object");
}

void UK2Node_ConvertAsset::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = Cast<UEdGraphSchema_K2>(GetSchema());
	const bool bReferenceObsoleteClass = !TargetType || TargetType->HasAnyClassFlags(CLASS_NewerVersionExists);
	if (bReferenceObsoleteClass)
	{
		Message_Error(FString::Printf(TEXT("Node '%s' references obsolete class '%s'"), *GetPathName(), *GetPathNameSafe(TargetType)));
	}
	ensure(!bReferenceObsoleteClass);

	if (TargetType && K2Schema)
	{
		if (bIsAssetClass)
		{
			CreatePin(EGPD_Input, K2Schema->PC_AssetClass, TEXT(""), *TargetType, false, false, UK2Node_ConvertAssetImpl::InputPinName);
			CreatePin(EGPD_Output, K2Schema->PC_Class, TEXT(""), *TargetType, false, false, UK2Node_ConvertAssetImpl::OutputPinName);
		}
		else
		{
			CreatePin(EGPD_Input, K2Schema->PC_Asset, TEXT(""), *TargetType, false, false, UK2Node_ConvertAssetImpl::InputPinName);
			CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), *TargetType, false, false, UK2Node_ConvertAssetImpl::OutputPinName);
		}
	}
}

void UK2Node_ConvertAsset::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	bool bIsErrorFree = TargetType && Schema && (2 == Pins.Num());
	if (bIsErrorFree)
	{
		//Create Convert Function
		auto ConvertToObjectFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		const FName ConvertFunctionName = bIsAssetClass 
			? GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_AssetClassToClass) 
			: GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_AssetToObject);
		ConvertToObjectFunc->FunctionReference.SetExternalMember(ConvertFunctionName, UKismetSystemLibrary::StaticClass());
		ConvertToObjectFunc->AllocateDefaultPins();

		//Connect input to convert
		auto InputPin = FindPin(UK2Node_ConvertAssetImpl::InputPinName);
		const FString ConvertInputName = bIsAssetClass
			? FString(TEXT("AssetClass"))
			: FString(TEXT("Asset"));
		auto ConvertInput = ConvertToObjectFunc->FindPin(ConvertInputName);
		bIsErrorFree &= InputPin && ConvertInput && CompilerContext.MovePinLinksToIntermediate(*InputPin, *ConvertInput).CanSafeConnect();

		//Create Cast Node
		UK2Node_DynamicCast* CastNode = bIsAssetClass
			? CompilerContext.SpawnIntermediateNode<UK2Node_ClassDynamicCast>(this, SourceGraph)
			: CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
		CastNode->SetPurity(true);
		CastNode->TargetType = TargetType;
		CastNode->AllocateDefaultPins();

		// Connect Object/Class to Cast
		auto CastInput = CastNode->GetCastSourcePin();
		auto ConvertOutput = ConvertToObjectFunc->GetReturnValuePin();
		bIsErrorFree &= ConvertOutput && CastInput && Schema->TryCreateConnection(ConvertOutput, CastInput);

		// Connect output to cast
		auto OutputPin = FindPin(UK2Node_ConvertAssetImpl::InputPinName);
		auto CastOutput = CastNode->GetCastResultPin();
		bIsErrorFree &= OutputPin && CastOutput && CompilerContext.MovePinLinksToIntermediate(*OutputPin, *CastOutput).CanSafeConnect();
	}

	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "K2Node_ConvertAsset: Internal connection error. @@").ToString(), this);
	}

	BreakAllNodeLinks();
}

bool UK2Node_ConvertAsset::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	const UBlueprint* SourceBlueprint = GetBlueprint();
	UClass* SourceClass = *TargetType;
	const bool bResult = (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
	if (bResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass);
	}
	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bSuperResult || bResult;
}

FText UK2Node_ConvertAsset::GetCompactNodeTitle() const
{
	return FText::FromString(TEXT("\x2022"));
}

#undef LOCTEXT_NAMESPACE