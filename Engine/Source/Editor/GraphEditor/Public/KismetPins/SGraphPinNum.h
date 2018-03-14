// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Misc/DefaultValueHelper.h"
#include "ScopedTransaction.h"
#include "SGraphPin.h"

template <typename NumericType>
class SGraphPinNum : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPinNum) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	}

protected:
	// SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(SBox)
			.MinDesiredWidth(18)
			.MaxDesiredWidth(400)
			[
				SNew(SNumericEntryBox<NumericType>)
				.EditableTextBoxStyle(FEditorStyle::Get(), "Graph.EditableTextBox")
				.BorderForegroundColor(FSlateColor::UseForeground())
				.Visibility(this, &SGraphPinNum::GetDefaultValueVisibility)
				.IsEnabled(this, &SGraphPinNum::GetDefaultValueIsEnabled)
				.Value(this, &SGraphPinNum::GetNumericValue)
				.OnValueCommitted(this, &SGraphPinNum::SetNumericValue)
			];
	}

	bool GetDefaultValueIsEnabled() const
	{
		return !GraphPinObj->bDefaultValueIsReadOnly;
	}

	TOptional<NumericType> GetNumericValue() const
	{
		NumericType Num = NumericType();
		Lex::FromString(Num, *GraphPinObj->GetDefaultAsString());
		return Num;
	}

	void SetNumericValue(NumericType InValue, ETextCommit::Type CommitType)
	{
		const FString TypeValueString = Lex::ToString(InValue);
		if (GraphPinObj->GetDefaultAsString() != TypeValueString)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangeNumberPinValue", "Change Number Pin Value"));
			GraphPinObj->Modify();

			GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, *TypeValueString);
		}
	}
};
