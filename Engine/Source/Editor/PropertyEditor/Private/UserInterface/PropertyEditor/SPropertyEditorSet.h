// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "PropertyEditor.h"
#include "PropertyEditorConstants.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

class SPropertyEditorSet : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPropertyEditorSet)
		: _Font(FEditorStyle::GetFontStyle(PropertyEditorConstants::PropertyFontStyle))
		{}
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)
	SLATE_END_ARGS()

	static bool Supports(const TSharedRef< class FPropertyEditor >& InPropertyEditor);

	void Construct(const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor);

	void GetDesiredWidth(float& OutMinDesiredWidth, float& OutMaxDesiredWidth);

private:

	FText GetSetTextValue() const;
	FText GetSetTooltipText() const;

	/** @return True if the property can be edited */
	bool CanEdit() const;

private:
	TSharedPtr< FPropertyEditor > PropertyEditor;
};