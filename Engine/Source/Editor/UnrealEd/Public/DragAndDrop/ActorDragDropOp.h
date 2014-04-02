// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __ActorDragDropOp_h__
#define __ActorDragDropOp_h__

#include "ClassIconFinder.h"
#include "DecoratedDragDropOp.h"

class FActorDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FActorDragDropOp, FDecoratedDragDropOp)

	/** Actor that we are dragging */
	TArray< TWeakObjectPtr<AActor> >	Actors;

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
