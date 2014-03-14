// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __ActorDragDropOp_h__
#define __ActorDragDropOp_h__

#include "ClassIconFinder.h"

class FActorDragDropOp : public FDragDropOperation, public TSharedFromThis<FActorDragDropOp>
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FActorDragDropOp"); return Type;}

	/** Actor that we are dragging */
	TArray< TWeakObjectPtr<AActor> >	Actors;

	/** String to show as hover text */
	FString								CurrentHoverText;

	/** Icon to be displayed */
	const FSlateBrush*					CurrentIconBrush;

	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		// Create hover widget
		return SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			.Content()
			[			
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0.0f, 0.0f, 3.0f, 0.0f )
				[
					SNew( SImage )
					.Image( this, &FActorDragDropOp::GetIcon )
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign( VAlign_Center )
				[
					SNew(STextBlock) 
					.Text( this, &FActorDragDropOp::GetHoverText )
				]
			];
	}

	FString GetHoverText() const
	{
		return CurrentHoverText;
	}

	const FSlateBrush* GetIcon( ) const
	{
		return CurrentIconBrush;
	}

	void Init(const TArray< TWeakObjectPtr<AActor> >& InActors)
	{
		for(int32 i=0; i<InActors.Num(); i++)
		{
			if( InActors[i].IsValid() )
			{
				Actors.Add( InActors[i] );
			}
		}

		// Set text and icon
		UClass* CommonSelClass = NULL;
		CurrentIconBrush = FClassIconFinder::FindIconForActors(Actors, CommonSelClass);
		if(Actors.Num() == 0)
		{
			CurrentHoverText =  TEXT("None");
		}
		else if(Actors.Num() == 1)
		{
			// Find icon for actor
			AActor* TheActor = Actors[0].Get();
			CurrentHoverText = TheActor->GetActorLabel();
		}
		else
		{
			CurrentHoverText = FString::Printf(TEXT("%d Actors"), InActors.Num());
		}
	}
};

#endif // __ActorDragDropOp_h__
