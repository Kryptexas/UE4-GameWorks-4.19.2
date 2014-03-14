// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FAnimEditAppMode

class FAnimEditAppMode : public FPersonaAppMode
{
public:
	FAnimEditAppMode(TSharedPtr<class FPersona> InPersona);
};

/////////////////////////////////////////////////////
// SAnimDifferentAssetBeingPreviewedWarning

class SAnimDifferentAssetBeingPreviewedWarning : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimDifferentAssetBeingPreviewedWarning) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FPersona> InPersona);

private:
	// Pointer back to owning Persona instance (the keeper of state)
	TWeakPtr<class FPersona> PersonaPtr;
private:
	EVisibility GetVisibility() const;
};