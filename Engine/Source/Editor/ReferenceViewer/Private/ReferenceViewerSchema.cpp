// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ReferenceViewerPrivatePCH.h"
#include "AssetThumbnail.h"
#include "ReferenceViewerActions.h"
#include "GlobalEditorCommonCommands.h"

//////////////////////////////////////////////////////////////////////////
// UReferenceViewerSchema

UReferenceViewerSchema::UReferenceViewerSchema(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UReferenceViewerSchema::GetContextMenuActions(const UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, class FMenuBuilder* MenuBuilder, bool bIsDebugging) const
{
	MenuBuilder->AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
	MenuBuilder->AddMenuEntry(FReferenceViewerActions::Get().ReCenterGraph);
	MenuBuilder->AddMenuEntry(FReferenceViewerActions::Get().ListReferencedObjects);
	MenuBuilder->AddMenuEntry(FReferenceViewerActions::Get().ListObjectsThatReference);

	MenuBuilder->AddSubMenu(NSLOCTEXT("ReferenceViewerSchema","MakeCollectionWithReferencedAssetsTitle", "Make Collection with Referenced Assets"),
							FText::GetEmpty(),
							FNewMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithReferencedAssetsSubMenu )
							);

	MenuBuilder->AddMenuEntry(FReferenceViewerActions::Get().ShowReferenceTree);
}

FLinearColor UReferenceViewerSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

void UReferenceViewerSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	// Don't allow breaking any links
}

void UReferenceViewerSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
	// Don't allow breaking any links
}

FPinConnectionResponse UReferenceViewerSchema::MovePinLinks(UEdGraphPin& MoveFromPin, UEdGraphPin& MoveToPin) const
{
	// Don't allow moving any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

FPinConnectionResponse UReferenceViewerSchema::CopyPinLinks(UEdGraphPin& CopyFromPin, UEdGraphPin& CopyToPin) const
{
	// Don't allow copying any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

void UReferenceViewerSchema::GetMakeCollectionWithReferencedAssetsSubMenu( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.AddMenuEntry(FReferenceViewerActions::Get().MakeLocalCollectionWithReferencedAssets);
	MenuBuilder.AddMenuEntry(FReferenceViewerActions::Get().MakePrivateCollectionWithReferencedAssets);
	MenuBuilder.AddMenuEntry(FReferenceViewerActions::Get().MakeSharedCollectionWithReferencedAssets);
}
