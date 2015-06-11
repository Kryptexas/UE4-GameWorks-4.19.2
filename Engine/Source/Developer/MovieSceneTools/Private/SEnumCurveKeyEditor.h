// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A widget for editing a curve representing enum keys.
 */
class SEnumCurveKeyEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnumCurveKeyEditor) {}
		SLATE_ARGUMENT(ISequencer*, Sequencer)
		SLATE_ARGUMENT(UMovieSceneSection*, OwningSection)
		SLATE_ARGUMENT(FIntegralCurve*, Curve)
		SLATE_ARGUMENT(UEnum*, Enum)
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

private:
	FText GetCurrentValue() const;

	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<FString> InItem);

	void OnComboSelectionChanged(TSharedPtr<FString> InSelectedItem, ESelectInfo::Type SelectInfo);

	ISequencer* Sequencer;
	UMovieSceneSection* OwningSection;
	FIntegralCurve* Curve;
	TArray<TSharedPtr<FString>> EnumValues;
};