// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NewsFeedPrivatePCH.h"


/* UNewsFeedSettings structors
 *****************************************************************************/

UNewsFeedSettings::UNewsFeedSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
	, MaxItemsToShow(10)
	, ShowOnlyUnreadItems(true)
{ }


/* UObject overrides
 *****************************************************************************/

void UNewsFeedSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	SettingChangedEvent.Broadcast(PropertyName);
}
