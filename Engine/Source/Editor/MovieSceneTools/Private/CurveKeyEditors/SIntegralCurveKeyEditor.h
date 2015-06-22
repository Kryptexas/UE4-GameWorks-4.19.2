// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A widget for editing a curve representing integer keys.
 */
class SIntegralCurveKeyEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SIntegralCurveKeyEditor) {}
		SLATE_ARGUMENT(ISequencer*, Sequencer)
		SLATE_ARGUMENT(UMovieSceneSection*, OwningSection)
		SLATE_ARGUMENT(FIntegralCurve*, Curve)
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

private:
	void OnBeginSliderMovement();

	void OnEndSliderMovement(int32 Value);

	int32 OnGetKeyValue() const;

	void OnValueChanged(int32 Value);

	ISequencer* Sequencer;
	UMovieSceneSection* OwningSection;
	FIntegralCurve* Curve;
};