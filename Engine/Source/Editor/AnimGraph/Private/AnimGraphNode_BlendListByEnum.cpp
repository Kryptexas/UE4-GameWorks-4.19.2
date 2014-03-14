// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "CompilerResultsLog.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

#define LOCTEXT_NAMESPACE "BlendListByEnum"

/////////////////////////////////////////////////////
// UAnimGraphNode_BlendListByEnum

UAnimGraphNode_BlendListByEnum::UAnimGraphNode_BlendListByEnum(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_BlendListByEnum::GetNodeCategory() const
{
	return FString::Printf(TEXT("%s, Blend List by enum"), *Super::GetNodeCategory() );
}

FString UAnimGraphNode_BlendListByEnum::GetTooltip() const
{
	if (BoundEnum != NULL)
	{
		return FString::Printf(TEXT("Blend Poses based on enum (%s). Use context menu to add new entries."), *BoundEnum->GetName());
	}
	else
	{
		return FString::Printf(TEXT("ERROR: Blend Poses (by missing enum)"));
	}
}

FString UAnimGraphNode_BlendListByEnum::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (BoundEnum != NULL)
	{
		return FString::Printf(TEXT("Blend Poses (%s)"), *BoundEnum->GetName());
	}
	else
	{
		return FString::Printf(TEXT("ERROR: Blend Poses (by missing enum)"));
	}
}

void UAnimGraphNode_BlendListByEnum::PostPlacedNewNode()
{
	// Make sure we start out with a pin
	Node.AddPose();
}

void UAnimGraphNode_BlendListByEnum::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();

	// add all blendlist enum entries
	for (TObjectIterator<UEnum> EnumIt; EnumIt; ++EnumIt)
	{
		UEnum* CurrentEnum = *EnumIt;

		const bool bIsBlueprintType = UEdGraphSchema_K2::IsAllowableBlueprintVariableType(CurrentEnum);
		if (bIsBlueprintType)
		{
			UAnimGraphNode_BlendListByEnum* EnumTemplate = NewObject<UAnimGraphNode_BlendListByEnum>();
			EnumTemplate->BoundEnum = CurrentEnum;

			TSharedPtr<FEdGraphSchemaAction_K2NewNode> Action = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, EnumTemplate->GetNodeCategory(), EnumTemplate->GetNodeTitle(ENodeTitleType::ListView), EnumTemplate->GetTooltip(), 0, EnumTemplate->GetKeywords());
			Action->NodeTemplate = EnumTemplate;
		}
	}
}

void UAnimGraphNode_BlendListByEnum::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if (!Context.bIsDebugging && (BoundEnum != NULL))
	{
		if ((Context.Pin != NULL) && (Context.Pin->Direction == EGPD_Input))
		{
			int32 RawArrayIndex = 0;
			bool bIsPosePin = false;
			bool bIsTimePin = false;
			GetPinInformation(Context.Pin->PinName, /*out*/ RawArrayIndex, /*out*/ bIsPosePin, /*out*/ bIsTimePin);

			if (bIsPosePin || bIsTimePin)
			{
				const int32 ExposedEnumIndex = RawArrayIndex - 1;

				if (ExposedEnumIndex != INDEX_NONE)
				{
					// Offer to remove this specific pin
					FUIAction Action = FUIAction( FExecuteAction::CreateUObject( this, &UAnimGraphNode_BlendListByEnum::RemovePinFromBlendList, const_cast<UEdGraphPin*>(Context.Pin)) );
					Context.MenuBuilder->AddMenuEntry( LOCTEXT("RemovePose", "Remove Pose"), FText::GetEmpty(), FSlateIcon(), Action );
				}
			}
		}

		// Offer to add any not-currently-visible pins
		bool bAddedHeader = false;
		const int32 MaxIndex = BoundEnum->NumEnums() - 1; // we don't want to show _MAX enum
		for (int32 Index = 0; Index < MaxIndex; ++Index)
		{
			FName ElementName = BoundEnum->GetEnum(Index);
			if (!VisibleEnumEntries.Contains(ElementName))
			{
				FText PrettyElementName = BoundEnum->GetEnumText(Index);

				// Offer to add this entry
				if (!bAddedHeader)
				{
					bAddedHeader = true;
					Context.MenuBuilder->BeginSection("AnimGraphNodeAddElementPin", LOCTEXT("ExposeHeader", "Add pin for element"));
					{
						FUIAction Action = FUIAction( FExecuteAction::CreateUObject( this, &UAnimGraphNode_BlendListByEnum::ExposeEnumElementAsPin, ElementName) );
						Context.MenuBuilder->AddMenuEntry(PrettyElementName, PrettyElementName, FSlateIcon(), Action);
					}
					Context.MenuBuilder->EndSection();
				}
				else
				{
					FUIAction Action = FUIAction( FExecuteAction::CreateUObject( this, &UAnimGraphNode_BlendListByEnum::ExposeEnumElementAsPin, ElementName) );
					Context.MenuBuilder->AddMenuEntry(PrettyElementName, PrettyElementName, FSlateIcon(), Action);
				}
			}
		}
	}
}

void UAnimGraphNode_BlendListByEnum::ExposeEnumElementAsPin(FName EnumElementName)
{
	if (!VisibleEnumEntries.Contains(EnumElementName))
	{
		FScopedTransaction Transaction( LOCTEXT("ExposeElement", "ExposeElement") );
		Modify();

		VisibleEnumEntries.Add(EnumElementName);

		Node.AddPose();
	
		ReconstructNode();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UAnimGraphNode_BlendListByEnum::RemovePinFromBlendList(UEdGraphPin* Pin)
{
	int32 RawArrayIndex = 0;
	bool bIsPosePin = false;
	bool bIsTimePin = false;
	GetPinInformation(Pin->PinName, /*out*/ RawArrayIndex, /*out*/ bIsPosePin, /*out*/ bIsTimePin);

	const int32 ExposedEnumIndex = (bIsPosePin || bIsTimePin) ? (RawArrayIndex - 1) : INDEX_NONE;

	if (ExposedEnumIndex != INDEX_NONE)
	{
		FScopedTransaction Transaction( LOCTEXT("RemovePin", "RemovePin") );
		Modify();

		// Record it as no longer exposed
		VisibleEnumEntries.RemoveAt(ExposedEnumIndex);

		// Remove the pose from the node
		UProperty* AssociatedProperty;
		int32 ArrayIndex;
		GetPinAssociatedProperty(GetFNodeType(), Pin, /*out*/ AssociatedProperty, /*out*/ ArrayIndex);

		ensure(ArrayIndex == (ExposedEnumIndex + 1));

		// setting up removed pins info 
		RemovedPinArrayIndex = ArrayIndex;
		Node.RemovePose(ArrayIndex);
		ReconstructNode();
		//@TODO: Just want to invalidate the visual representation currently
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UAnimGraphNode_BlendListByEnum::GetPinInformation(const FString& InPinName, int32& Out_PinIndex, bool& Out_bIsPosePin, bool& Out_bIsTimePin)
{
	const int32 UnderscoreIndex = InPinName.Find(TEXT("_"));
	if (UnderscoreIndex != INDEX_NONE)
	{
		const FString ArrayName = InPinName.Left(UnderscoreIndex);
		Out_PinIndex = FCString::Atoi(*(InPinName.Mid(UnderscoreIndex + 1)));

		Out_bIsPosePin = ArrayName == TEXT("BlendPose");
		Out_bIsTimePin = ArrayName == TEXT("BlendTime");
	}
	else
	{
		Out_bIsPosePin = false;
		Out_bIsTimePin = false;
		Out_PinIndex = INDEX_NONE;
	}
}

void UAnimGraphNode_BlendListByEnum::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const
{
	// if pin name starts with BlendPose or BlendWeight, change to enum name 
	bool bIsPosePin;
	bool bIsTimePin;
	int32 RawArrayIndex;
	GetPinInformation(Pin->PinName, /*out*/ RawArrayIndex, /*out*/ bIsPosePin, /*out*/ bIsTimePin);
	checkSlow(RawArrayIndex == ArrayIndex);

	if (bIsPosePin || bIsTimePin)
	{
		if (RawArrayIndex > 0)
		{
			const int32 ExposedEnumPinIndex = RawArrayIndex - 1;

			// find pose index and see if it's mapped already or not
			if (VisibleEnumEntries.IsValidIndex(ExposedEnumPinIndex) && (BoundEnum != NULL))
			{
				const FName& EnumElementName = VisibleEnumEntries[ExposedEnumPinIndex];
				const int32 EnumIndex = BoundEnum->FindEnumIndex(EnumElementName);
				if (EnumIndex != INDEX_NONE)
				{
					Pin->PinFriendlyName = BoundEnum->GetEnumString(EnumIndex);
				}
				else
				{
					Pin->PinFriendlyName = EnumElementName.ToString();
				}
			}
			else
			{
				Pin->PinFriendlyName = TEXT("Invalid index");
			}
		}
		else if (ensure(RawArrayIndex == 0))
		{
			Pin->PinFriendlyName = TEXT("Default");
		}

		// Append the pin type
		if (bIsPosePin)
		{
			Pin->PinFriendlyName += TEXT(" Pose");
		}

		if (bIsTimePin)
		{
			Pin->PinFriendlyName += TEXT(" Blend Time");
		}
	}
}

void UAnimGraphNode_BlendListByEnum::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	if (BoundEnum == NULL)
	{
		MessageLog.Error(TEXT("@@ references an unknown enum; please delete the node and recreate it"), this);
	}
}

void UAnimGraphNode_BlendListByEnum::PreloadRequiredAssets()
{
	PreloadObject(BoundEnum);

	Super::PreloadRequiredAssets();
}

void UAnimGraphNode_BlendListByEnum::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	if (BoundEnum != NULL)
	{
		PreloadObject(BoundEnum);

		// Zero the array out so it looks up the default value, and stat counting at index 1
		Node.EnumToPoseIndex.Empty();
		Node.EnumToPoseIndex.AddZeroed(BoundEnum->NumEnums());
		int32 PinIndex = 1;

		// Run thru the enum entries
		for (auto ExposedIt = VisibleEnumEntries.CreateConstIterator(); ExposedIt; ++ExposedIt)
		{
			const FName& EnumElementName = *ExposedIt;
			const int32 EnumIndex = BoundEnum->FindEnumIndex(EnumElementName);

			if (EnumIndex != INDEX_NONE)
			{
				Node.EnumToPoseIndex[EnumIndex] = PinIndex;
			}
			else
			{
				MessageLog.Error(*FString::Printf(TEXT("@@ references an unknown enum entry %s"), *EnumElementName.ToString()), this);
			}

			++PinIndex;
		}
	}
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE