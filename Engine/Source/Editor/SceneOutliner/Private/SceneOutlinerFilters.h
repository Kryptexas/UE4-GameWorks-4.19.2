// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "DelegateFilter.h"
#include "SharedPointer.h"

namespace SceneOutlinerFilters
{
	struct FActorClassPredicate
	{
		static bool IsSelectedActor( const AActor* const InActor )
		{			
			return ( InActor != NULL && InActor->IsSelected() );
		}

		static bool ExistsInEditorWorld( const AActor* const InActor )
		{
			return (InActor != NULL && (InActor->GetWorld()->WorldType != EWorldType::PIE || GEditor->ObjectsThatExistInEditorWorld.Get(InActor)));
		}
	};

	TSharedPtr< TDelegateFilter< const AActor* const > > CreateSelectedActorFilter()
	{
		return MakeShareable( new TDelegateFilter< const AActor* const >(
			TDelegateFilter< const AActor* const >::FPredicate::CreateStatic( &FActorClassPredicate::IsSelectedActor ) ) );
	}

	TSharedPtr< TDelegateFilter< const AActor* const > > CreateHideTemporaryActorsFilter()
	{
		return MakeShareable( new TDelegateFilter< const AActor* const >(
			TDelegateFilter< const AActor* const >::FPredicate::CreateStatic( &FActorClassPredicate::ExistsInEditorWorld ) ) );
	}
}