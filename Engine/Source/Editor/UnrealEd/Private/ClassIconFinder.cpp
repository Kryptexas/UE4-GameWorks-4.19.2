// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "Slate.h"
#include "ClassIconFinder.h"

const FSlateBrush* FClassIconFinder::FindIconForActors(const TArray< TWeakObjectPtr<AActor> >& InActors, UClass*& CommonBaseClass)
{
	// Get the common base class of the selected actors
	const FSlateBrush* CommonIcon = NULL;
	for( int32 ActorIdx = 0; ActorIdx < InActors.Num(); ++ActorIdx )
	{
		TWeakObjectPtr<AActor> Actor = InActors[ActorIdx];
		UClass* ObjClass = Actor->GetClass();

		if (!CommonBaseClass)
		{
			CommonBaseClass = ObjClass;
		}
		while (!ObjClass->IsChildOf(CommonBaseClass))
		{
			CommonBaseClass = CommonBaseClass->GetSuperClass();
		}

		const FSlateBrush* ActorIcon = FindIconForActor(Actor);

		if (!CommonIcon)
		{
			CommonIcon = ActorIcon;
		}
		if (CommonIcon != ActorIcon)
		{
			CommonIcon = FindIconForClass(CommonBaseClass);
		}
	}

	return CommonIcon;
}

const FSlateBrush* FClassIconFinder::FindIconForActor( const TWeakObjectPtr<AActor>& InActor )
{
	return FEditorStyle::GetBrush( FindIconNameForActor( InActor ) );
}

FName FClassIconFinder::FindIconNameForActor( const TWeakObjectPtr<AActor>& InActor )
{
	// Actor specific overrides to normal per-class icons
	AActor* Actor = InActor.Get();
	FName BrushName = NAME_None;

	if ( Actor != NULL )
	{
		ABrush* Brush = Cast< ABrush >( Actor );
		if ( Brush )
		{
			if (Brush_Add == Brush->BrushType)
			{
				BrushName = TEXT( "ClassIcon.BrushAdditive" );
			}
			else if (Brush_Subtract == Brush->BrushType)
			{
				BrushName = TEXT( "ClassIcon.BrushSubtractive" );
			}
		}

		// Actor didn't specify an icon - fallback on the class icon
		if ( BrushName == NAME_None )
		{
			BrushName = FindIconNameForClass( Actor->GetClass() );
		}
	}
	else
	{
		// If the actor reference is NULL it must have been deleted
		BrushName = TEXT( "ClassIcon.Deleted" );
	}

	return BrushName;
}


const FSlateBrush* FClassIconFinder::FindIconForClass(UClass* InClass, const FName& InDefaultName )
{
	return FEditorStyle::GetBrush( FindIconNameForClass( InClass, InDefaultName ) );
}

FName FClassIconFinder::FindIconNameForClass(UClass* InClass, const FName& InDefaultName )
{
	FName BrushName = InDefaultName;
	const FSlateBrush* Brush = NULL;

	if ( InClass != NULL )
	{
		// walk up class hierarchy until we find an icon
		UClass* ActorClass = InClass;
		while( (Brush == NULL || Brush == FEditorStyle::GetDefaultBrush()) && ActorClass && (ActorClass != AActor::StaticClass()) )
		{
			BrushName = *FString::Printf( TEXT( "ClassIcon.%s" ), *ActorClass->GetName() );
			Brush = FEditorStyle::GetBrush( BrushName );
			ActorClass = ActorClass->GetSuperClass();
		}
	}

	if( Brush == NULL || Brush == FEditorStyle::GetDefaultBrush() )
	{
		// If we didn't supply an override name for the default icon use default class icon.
		if( InDefaultName == "" )
		{
			BrushName = TEXT( "ClassIcon.Default" );
		}
		else
		{
			BrushName = InDefaultName;
		}
	}

	return BrushName;
}