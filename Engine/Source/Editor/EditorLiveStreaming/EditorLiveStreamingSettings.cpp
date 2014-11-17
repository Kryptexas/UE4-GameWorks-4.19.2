// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EditorLiveStreamingModule.h"
#include "EditorLiveStreamingSettings.h"


UEditorLiveStreamingSettings::UEditorLiveStreamingSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}


void UEditorLiveStreamingSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	const FName Name = PropertyThatChanged ? PropertyThatChanged->GetFName() : NAME_None;
// 	if( Name == ??? )
// 	{
//		...
//	}

	// Save config to file, but only if we are not the build machine since game agnostic settings may put the builder in an unclean state
	if( !GIsBuildMachine )
	{
		this->SaveConfig();
	}	
}