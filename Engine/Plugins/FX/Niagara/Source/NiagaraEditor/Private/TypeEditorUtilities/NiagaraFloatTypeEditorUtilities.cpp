// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraFloatTypeEditorUtilities.h"
#include "SNiagaraParameterEditor.h"
#include "NiagaraTypes.h"
#include "NiagaraEditorStyle.h"

#include "SSpinBox.h"

class SNiagaraFloatParameterEditor : public SNiagaraParameterEditor
{
public:
	SLATE_BEGIN_ARGS(SNiagaraFloatParameterEditor) { }
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpinBox<float>)
				.Style(FNiagaraEditorStyle::Get(), "NiagaraEditor.ParameterSpinBox")
				.Font(FNiagaraEditorStyle::Get().GetFontStyle("NiagaraEditor.ParameterFont"))
				.MinValue(TOptional<float>())
				.MaxValue(TOptional<float>())
				.MaxSliderValue(TOptional<float>())
				.MinSliderValue(TOptional<float>())
				.Delta(0.0f)
				.Value(this, &SNiagaraFloatParameterEditor::GetValue)
				.OnValueChanged(this, &SNiagaraFloatParameterEditor::ValueChanged)
				.OnValueCommitted(this, &SNiagaraFloatParameterEditor::ValueCommitted)
				.OnBeginSliderMovement(this, &SNiagaraFloatParameterEditor::BeginSliderMovement)
				.OnEndSliderMovement(this, &SNiagaraFloatParameterEditor::EndSliderMovement)
				.MinDesiredWidth(100)
			]
		];
	}

	virtual void UpdateInternalValueFromStruct(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetFloatStruct(), TEXT("Struct type not supported."));
		FloatValue = ((FNiagaraFloat*)Struct->GetStructMemory())->Value;
	}

	virtual void UpdateStructFromInternalValue(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetFloatStruct(), TEXT("Struct type not supported."));
		((FNiagaraFloat*)Struct->GetStructMemory())->Value = FloatValue;
	}

private:
	void BeginSliderMovement()
	{
		ExecuteOnBeginValueChange();
	}

	void EndSliderMovement(float Value)
	{
		ExecuteOnEndValueChange();
	}

	float GetValue() const
	{
		return FloatValue;
	}

	void ValueChanged(float Value)
	{
		FloatValue = Value;
		ExecuteOnValueChanged();
	}

	void ValueCommitted(float Value, ETextCommit::Type CommitInfo)
	{
		if (CommitInfo == ETextCommit::OnEnter || CommitInfo == ETextCommit::OnUserMovedFocus)
		{
			ValueChanged(Value);
		}
	}

private:
	float FloatValue;
};

TSharedPtr<SNiagaraParameterEditor> FNiagaraEditorFloatTypeUtilities::CreateParameterEditor(const FNiagaraTypeDefinition& ParameterType) const
{
	return SNew(SNiagaraFloatParameterEditor);
}

bool FNiagaraEditorFloatTypeUtilities::CanHandlePinDefaults() const
{
	return true;
}

FString FNiagaraEditorFloatTypeUtilities::GetPinDefaultStringFromValue(const FNiagaraVariable& AllocatedVariable) const
{
	checkf(AllocatedVariable.IsDataAllocated(), TEXT("Can not generate a default value string for an unallocated variable."));
	return Lex::ToString(AllocatedVariable.GetValue<FNiagaraFloat>().Value);
}

bool FNiagaraEditorFloatTypeUtilities::SetValueFromPinDefaultString(const FString& StringValue, FNiagaraVariable& Variable) const
{
	FNiagaraFloat FloatValue;
	if (Lex::TryParseString(FloatValue.Value, *StringValue))
	{
		Variable.SetValue<FNiagaraFloat>(FloatValue);
		return true;
	}
	return false;
}