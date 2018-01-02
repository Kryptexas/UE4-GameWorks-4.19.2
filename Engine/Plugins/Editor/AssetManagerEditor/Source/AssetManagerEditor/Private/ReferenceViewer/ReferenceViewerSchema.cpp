// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ReferenceViewerSchema.h"
#include "Textures/SlateIcon.h"
#include "Misc/Attribute.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "CollectionManagerTypes.h"
#include "AssetManagerEditorCommands.h"
#include "AssetManagerEditorModule.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "ConnectionDrawingPolicy.h"


static const FLinearColor RiceFlower = FLinearColor(FColor(236, 252, 227));
static const FLinearColor CannonPink = FLinearColor(FColor(145, 66, 117));

// Overridden connection drawing policy to use less curvy lines between nodes
class FReferenceViewerConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FReferenceViewerConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
		: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
	{
	}

	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override
	{
		const int32 Tension = FMath::Abs<int32>(Start.X - End.X);
		return Tension * FVector2D(1.0f, 0);
	}

	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override
	{
		if (OutputPin->PinType.PinCategory == TEXT("hard") || InputPin->PinType.PinCategory == TEXT("hard"))
		{
			Params.WireColor = RiceFlower;
		}
		else
		{
			Params.WireColor = CannonPink;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// UReferenceViewerSchema

UReferenceViewerSchema::UReferenceViewerSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UReferenceViewerSchema::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	MenuBuilder->BeginSection(TEXT("Asset"), NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset"));
	{
		MenuBuilder->AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().OpenSelectedInAssetEditor);
	}
	MenuBuilder->EndSection();

	MenuBuilder->BeginSection(TEXT("Misc"), NSLOCTEXT("ReferenceViewerSchema", "MiscSectionLabel", "Misc"));
	{
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ReCenterGraph);
		MenuBuilder->AddSubMenu(
			NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithTitle", "Make Collection with"),
			NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithTooltip", "Makes a collection with either the referencers or dependencies of the selected nodes."),
			FNewMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithSubMenu)
		);
	}
	MenuBuilder->EndSection();

	MenuBuilder->BeginSection(TEXT("References"), NSLOCTEXT("ReferenceViewerSchema", "ReferencesSectionLabel", "References"));
	{
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().CopyReferencedObjects);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().CopyReferencingObjects);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferencedObjects);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferencingObjects);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferenceTree);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ViewSizeMap);
		MenuBuilder->AddMenuEntry(FAssetManagerEditorCommands::Get().ViewAssetAudit, TEXT("ContextMenu"));
	}
	MenuBuilder->EndSection();
}

FLinearColor UReferenceViewerSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == TEXT("hard"))
	{
		return RiceFlower;
	}
	else
	{
		return CannonPink;
	}
}

void UReferenceViewerSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	// Don't allow breaking any links
}

void UReferenceViewerSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	// Don't allow breaking any links
}

FPinConnectionResponse UReferenceViewerSchema::MovePinLinks(UEdGraphPin& MoveFromPin, UEdGraphPin& MoveToPin, bool bIsItermediateMove) const
{
	// Don't allow moving any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString());
}

FPinConnectionResponse UReferenceViewerSchema::CopyPinLinks(UEdGraphPin& CopyFromPin, UEdGraphPin& CopyToPin, bool bIsItermediateCopy) const
{
	// Don't allow copying any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString());
}

FConnectionDrawingPolicy* UReferenceViewerSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FReferenceViewerConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements);
}

void UReferenceViewerSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	TArray<FAssetIdentifier> AssetIdentifiers;

	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);
	IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
}

void UReferenceViewerSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	OutOkIcon = true;
}

void UReferenceViewerSchema::GetMakeCollectionWithSubMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddSubMenu(NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithReferencersTitle", "Referencers <-"),
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithReferencersTooltip", "Makes a collection with assets one connection to the left of selected nodes."),
		FNewMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu, true)
		);

	MenuBuilder.AddSubMenu(NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithDependenciesTitle", "Dependencies ->"),
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithDependenciesTooltip", "Makes a collection with assets one connection to the right of selected nodes."),
		FNewMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu, false)
		);
}

void UReferenceViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu(FMenuBuilder& MenuBuilder, bool bReferencers)
{
	if (bReferencers)
	{
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeLocalCollectionWithReferencers, 
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Local), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Local))
			);
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakePrivateCollectionWithReferencers,
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Private), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Private))
			);
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeSharedCollectionWithReferencers,
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Shared), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Shared))
			);
	}
	else
	{
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeLocalCollectionWithDependencies, 
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Local), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Local))
			);
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakePrivateCollectionWithDependencies,
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Private), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Private))
			);
		MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeSharedCollectionWithDependencies,
			NAME_None, TAttribute<FText>(), 
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Shared), 
			FSlateIcon(FEditorStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Shared))
			);
	}
}
