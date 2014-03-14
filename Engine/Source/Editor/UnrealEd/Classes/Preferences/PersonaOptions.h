// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PersonaOptions
//
// A configuration class that holds information for the setup of the Persona.
// Supplied so that the editor 'remembers' the last setup the user had.
//=============================================================================

#pragma once
#include "PersonaOptions.generated.h"

UCLASS(hidecategories=Object, config=EditorUserSettings)
class UNREALED_API UPersonaOptions : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bShowFloor:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bShowSky:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bShowGrid:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bHighlightOrigin:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bMuteAudio:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 bShowMeshStats:1;

	UPROPERTY(EditAnywhere, config, Category=Options)
	int32 GridSize;

	UPROPERTY(EditAnywhere, config, Category=Options)
	int32 ViewModeIndex;

	UPROPERTY(EditAnywhere, config, Category=Options)
	FLinearColor ViewportBackgroundColor;

	UPROPERTY(EditAnywhere, config, Category=Options)
	float ViewFOV;

	UPROPERTY(EditAnywhere, config, Category=Options)
	uint32 DefaultLocalAxesSelection;

public:
	void SetViewportBackgroundColor( const FLinearColor & InViewportBackgroundColor);
	void SetShowGrid( bool bInShowGrid );
	void SetHighlightOrigin( bool bInHighlightOrigin );
	void SetShowFloor( bool bInShowFloor );
	void SetShowSky( bool bInShowSky );
	void SetMuteAudio( bool bInMuteAudio );
	void SetGridSize( int32 InGridSize );
	void SetViewModeIndex( int32 InViewModeIndex );
	void SetViewFOV( float InViewFOV );
	void SetDefaultLocalAxesSelection( uint32 InDefaultLocalAxesSelection );
	void SetShowMeshStats( bool bInShowMeshStats );
};

