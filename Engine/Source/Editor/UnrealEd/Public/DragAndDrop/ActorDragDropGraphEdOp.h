// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __ActorDragDropGraphEdOp_h__
#define __ActorDragDropGraphEdOp_h__

#include "ActorDragDropOp.h"

#define LOCTEXT_NAMESPACE "ActorDragDrop"

class FActorDragDropGraphEdOp : public FActorDragDropOp
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FActorDragDropGraphEdOp"); return Type;}

	enum ToolTipTextType
	{
		ToolTip_Default,
		ToolTip_Compatible,
		ToolTip_Incompatible,
		ToolTip_MultipleSelection_Incompatible,
		ToolTip_CompatibleAttach,
		ToolTip_IncompatibleGeneric,
		ToolTip_CompatibleGeneric,
		ToolTip_CompatibleMultipleAttach,
		ToolTip_IncompatibleMultipleAttach,
		ToolTip_CompatibleDetach,
		ToolTip_CompatibleMultipleDetach
	};

	/** Copy default values*/
	void SetDefaults( )
	{
		DefaultHoverText = CurrentHoverText;
		DefaultHoverIcon = CurrentIconBrush;
	}

	/** Set the appropriate tool tip when dragging functionality is active*/
	void SetToolTip(ToolTipTextType TextType, FString ParamText = FString(TEXT("")))
	{
		switch( TextType )
		{
		case ToolTip_Default:
			{
				CurrentHoverText = DefaultHoverText;
				CurrentIconBrush = DefaultHoverIcon;
				break;
			}
		case ToolTip_Compatible:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipCompatible", "'%s' is compatible to replace object reference").ToString(), *Actors[0].Get()->GetActorLabel( ) );
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		case ToolTip_Incompatible:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipIncompatible", "'%s' is not compatible to replace object reference").ToString(), *Actors[0].Get()->GetActorLabel( ) );
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}
		case ToolTip_MultipleSelection_Incompatible:
			{
				CurrentHoverText = LOCTEXT("ToolTipMultipleSelectionIncompatible", "Cannot replace object reference when multiple objects are selected").ToString();
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}
		case ToolTip_CompatibleAttach:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipCompatibleAttach", "Attach %s to %s").ToString(), *Actors[0].Get()->GetActorLabel( ), *ParamText);
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		case ToolTip_IncompatibleGeneric:
			{
				CurrentHoverText = ParamText;
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}
		case ToolTip_CompatibleGeneric:
			{
				CurrentHoverText = ParamText;
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		case ToolTip_CompatibleMultipleAttach:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipCompatibleMultipleAttach", "Attach multiple objects to %s").ToString(), *ParamText);
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		case ToolTip_IncompatibleMultipleAttach:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipIncompatibleMultipleAttach", "Cannot attach multiple objects to %s").ToString(), *ParamText);
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}
		case ToolTip_CompatibleDetach:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipCompatibleDetach", "Detach %s from %s").ToString(), *Actors[0].Get()->GetActorLabel( ), *ParamText);
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		case ToolTip_CompatibleMultipleDetach:
			{
				CurrentHoverText = FString::Printf( *LOCTEXT("ToolTipCompatibleDetachMultiple", "Detach multiple objects from %s").ToString(), *ParamText);
				CurrentIconBrush = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;
			}
		}
	}
	
	static TSharedRef<FActorDragDropGraphEdOp> New(const TArray< TWeakObjectPtr<AActor> >& InActors)
	{
		TSharedRef<FActorDragDropGraphEdOp> Operation = MakeShareable(new FActorDragDropGraphEdOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FActorDragDropGraphEdOp>(Operation);
		
		Operation->Init(InActors);
		Operation->SetDefaults();

		Operation->Construct();
		return Operation;
	}

private:

	/** String to show as hover text */
	FString								DefaultHoverText;

	/** Icon to be displayed */
	const FSlateBrush*					DefaultHoverIcon;
};

#undef LOCTEXT_NAMESPACE
#endif // __ActorDragDropGraphEdOp_h__
