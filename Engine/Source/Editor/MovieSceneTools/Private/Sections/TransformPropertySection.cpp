// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Sections/TransformPropertySection.h"
#include "FloatCurveKeyArea.h"
#include "ISectionLayoutBuilder.h"
#include "Sections/MovieScene3DTransformSection.h"

void FTransformPropertySection::GenerateSectionLayout(class ISectionLayoutBuilder& LayoutBuilder) const
{
	static const FLinearColor BlueKeyAreaColor(0.0f, 0.0f, 0.7f, 0.5f);
	static const FLinearColor GreenKeyAreaColor(0.0f, 0.7f, 0.0f, 0.5f);
	static const FLinearColor RedKeyAreaColor(0.7f, 0.0f, 0.0f, 0.5f);

	UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(&SectionObject);

	// Translation
	TAttribute<TOptional<float>> TranslationXExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetTranslationValue, EAxis::X));
	TSharedRef<FFloatCurveKeyArea> TranslationXKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetTranslationCurve(EAxis::X), TranslationXExternalValue, TransformSection, RedKeyAreaColor));

	TAttribute<TOptional<float>> TranslationYExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetTranslationValue, EAxis::Y));
	TSharedRef<FFloatCurveKeyArea> TranslationYKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetTranslationCurve(EAxis::Y), TranslationYExternalValue, TransformSection, GreenKeyAreaColor));

	TAttribute<TOptional<float>> TranslationZExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetTranslationValue, EAxis::Z));
	TSharedRef<FFloatCurveKeyArea> TranslationZKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetTranslationCurve(EAxis::Z), TranslationZExternalValue, TransformSection, BlueKeyAreaColor));

	// Rotation
	TAttribute<TOptional<float>> RotationXExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetRotationValue, EAxis::X));
	TSharedRef<FFloatCurveKeyArea> RotationXKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetRotationCurve(EAxis::X), RotationXExternalValue, TransformSection, RedKeyAreaColor));

	TAttribute<TOptional<float>> RotationYExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetRotationValue, EAxis::Y));
	TSharedRef<FFloatCurveKeyArea> RotationYKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetRotationCurve(EAxis::Y), RotationYExternalValue, TransformSection, GreenKeyAreaColor));

	TAttribute<TOptional<float>> RotationZExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetRotationValue, EAxis::Z));
	TSharedRef<FFloatCurveKeyArea> RotationZKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetRotationCurve(EAxis::Z), RotationZExternalValue, TransformSection, BlueKeyAreaColor));

	// Scale
	TAttribute<TOptional<float>> ScaleXExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetScaleValue, EAxis::X));
	TSharedRef<FFloatCurveKeyArea> ScaleXKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetScaleCurve(EAxis::X), ScaleXExternalValue, TransformSection, RedKeyAreaColor));

	TAttribute<TOptional<float>> ScaleYExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetScaleValue, EAxis::Y));
	TSharedRef<FFloatCurveKeyArea> ScaleYKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetScaleCurve(EAxis::Y), ScaleYExternalValue, TransformSection, GreenKeyAreaColor));

	TAttribute<TOptional<float>> ScaleZExternalValue = TAttribute<TOptional<float>>::Create(
		TAttribute<TOptional<float>>::FGetter::CreateRaw(this, &FTransformPropertySection::GetScaleValue, EAxis::Z));
	TSharedRef<FFloatCurveKeyArea> ScaleZKeyArea = MakeShareable(new FFloatCurveKeyArea(&TransformSection->GetScaleCurve(EAxis::Z), ScaleZExternalValue, TransformSection, BlueKeyAreaColor));

	// This generates the tree structure for the transform section
	LayoutBuilder.PushCategory("Location", NSLOCTEXT("FTransformPropertySection", "LocationArea", "Location"));
	LayoutBuilder.AddKeyArea("Location.X", NSLOCTEXT("FTransformPropertySection", "LocXArea", "X"), TranslationXKeyArea);
	LayoutBuilder.AddKeyArea("Location.Y", NSLOCTEXT("FTransformPropertySection", "LocYArea", "Y"), TranslationYKeyArea);
	LayoutBuilder.AddKeyArea("Location.Z", NSLOCTEXT("FTransformPropertySection", "LocZArea", "Z"), TranslationZKeyArea);
	LayoutBuilder.PopCategory();

	LayoutBuilder.PushCategory("Rotation", NSLOCTEXT("FTransformPropertySection", "RotationArea", "Rotation"));
	LayoutBuilder.AddKeyArea("Rotation.X", NSLOCTEXT("FTransformPropertySection", "RotXArea", "X"), RotationXKeyArea);
	LayoutBuilder.AddKeyArea("Rotation.Y", NSLOCTEXT("FTransformPropertySection", "RotYArea", "Y"), RotationYKeyArea);
	LayoutBuilder.AddKeyArea("Rotation.Z", NSLOCTEXT("FTransformPropertySection", "RotZArea", "Z"), RotationZKeyArea);
	LayoutBuilder.PopCategory();

	LayoutBuilder.PushCategory("Scale", NSLOCTEXT("FTransformPropertySection", "ScaleArea", "Scale"));
	LayoutBuilder.AddKeyArea("Scale.X", NSLOCTEXT("FTransformPropertySection", "ScaleXArea", "X"), ScaleXKeyArea);
	LayoutBuilder.AddKeyArea("Scale.Y", NSLOCTEXT("FTransformPropertySection", "ScaleYArea", "Y"), ScaleYKeyArea);
	LayoutBuilder.AddKeyArea("Scale.Z", NSLOCTEXT("FTransformPropertySection", "ScaleZArea", "Z"), ScaleZKeyArea);
	LayoutBuilder.PopCategory();
}

TOptional<float> FTransformPropertySection::GetTranslationValue(EAxis::Type Axis) const
{
	FTransform Transform = GetPropertyValue<FTransform>().Get(FTransform::Identity);
	switch (Axis)
	{
	case EAxis::X:
		return TOptional<float>(Transform.GetTranslation().X);
	case EAxis::Y:
		return TOptional<float>(Transform.GetTranslation().Y);
	case EAxis::Z:
		return TOptional<float>(Transform.GetTranslation().Z);
	}
	return TOptional<float>();
}

TOptional<float> FTransformPropertySection::GetRotationValue(EAxis::Type Axis) const
{
	FTransform Transform = GetPropertyValue<FTransform>().Get(FTransform::Identity);
	switch (Axis)
	{
	case EAxis::X:
		return TOptional<float>(Transform.GetRotation().Rotator().Roll);
	case EAxis::Y:
		return TOptional<float>(Transform.GetRotation().Rotator().Pitch);
	case EAxis::Z:
		return TOptional<float>(Transform.GetRotation().Rotator().Yaw);
	}
	return TOptional<float>();
}

TOptional<float> FTransformPropertySection::GetScaleValue(EAxis::Type Axis) const
{
	FTransform Transform = GetPropertyValue<FTransform>().Get(FTransform::Identity);
	switch (Axis)
	{
	case EAxis::X:
		return TOptional<float>(Transform.GetScale3D().X);
	case EAxis::Y:
		return TOptional<float>(Transform.GetScale3D().Y);
	case EAxis::Z:
		return TOptional<float>(Transform.GetScale3D().Z);
	}
	return TOptional<float>();
}