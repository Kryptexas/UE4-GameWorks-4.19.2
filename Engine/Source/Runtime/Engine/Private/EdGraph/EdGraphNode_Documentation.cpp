// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EdGraph/EdGraphNode_Documentation.h"

#define LOCTEXT_NAMESPACE "EdGraph"

/////////////////////////////////////////////////////
// UEdGraphNode_Documentation

UEdGraphNode_Documentation::UEdGraphNode_Documentation( const class FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
#if WITH_EDITORONLY_DATA
	bCanResizeNode = true;
	bCanRenameNode = false;
#endif // WITH_EDITORONLY_DATA
	Link = TEXT( "Shared/GraphNodes/Blueprint" );
	Excerpt = TEXT( "UEdGraphNode_Documentation" );
}

#if WITH_EDITOR
void UEdGraphNode_Documentation::PostPlacedNewNode()
{
	NodeComment = TEXT( "" );
}

FString UEdGraphNode_Documentation::GetTooltip() const
{
	return FString::Printf( *NSLOCTEXT( "K2Node", "DocumentationBlock_Tooltip", "Documentation:\n%s" ).ToString(), *NodeComment );
}

FText UEdGraphNode_Documentation::GetNodeTitle( ENodeTitleType::Type TitleType ) const
{
	return NSLOCTEXT( "K2Node", "DocumentationBlock_Title", "UDN Documentation Excerpt" );
}

FString UEdGraphNode_Documentation::GetPinNameOverride(const UEdGraphPin& Pin) const
{
	return GetNodeTitle( ENodeTitleType::ListView ).ToString();
}

void UEdGraphNode_Documentation::ResizeNode(const FVector2D& NewSize)
{
	if( bCanResizeNode ) 
	{
		NodeHeight = NewSize.Y;
		NodeWidth = NewSize.X;
	}
}

void UEdGraphNode_Documentation::SetBounds( const class FSlateRect& Rect )
{
	NodePosX = Rect.Left;
	NodePosY = Rect.Top;

	FVector2D Size = Rect.GetSize();
	NodeWidth = Size.X;
	NodeHeight = Size.Y;
}

void UEdGraphNode_Documentation::OnRenameNode( const FString& NewName )
{
	NodeComment = NewName;
}

TSharedPtr<class INameValidatorInterface> UEdGraphNode_Documentation::MakeNameValidator() const
{
	// Documentation can be duplicated, etc...
	return MakeShareable( new FDummyNameValidator( EValidatorResult::Ok ));
}

#endif // WITH_EDITOR

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
