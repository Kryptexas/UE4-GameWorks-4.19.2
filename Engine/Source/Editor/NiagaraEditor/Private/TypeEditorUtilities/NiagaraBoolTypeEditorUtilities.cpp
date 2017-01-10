// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraBoolTypeEditorUtilities.h"
#include "NiagaraTypes.h"
#include "SNiagaraParameterEditor.h"
#include "DeclarativeSyntaxSupport.h"
#include "SBoxPanel.h"
#include "SCheckBox.h"

class SNiagaraBoolParameterEditor : public SNiagaraParameterEditor
{
public:
	SLATE_BEGIN_ARGS(SNiagaraBoolParameterEditor) { }
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(0)
			.AutoWidth()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SNiagaraBoolParameterEditor::GetCheckState)
				.OnCheckStateChanged(this, &SNiagaraBoolParameterEditor::OnCheckStateChanged)
			]
		];
	}

	virtual void UpdateInternalValueFromStruct(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetBoolStruct(), TEXT("Struct type not supported."));
		bBoolValue = ((FNiagaraBool*)Struct->GetStructMemory())->Value != 0;
	}

	virtual void UpdateStructFromInternalValue(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetBoolStruct(), TEXT("Struct type not supported."));
		((FNiagaraBool*)Struct->GetStructMemory())->Value = bBoolValue;
	}

private:
	ECheckBoxState GetCheckState() const
	{
		return bBoolValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	void OnCheckStateChanged(ECheckBoxState InCheckState)
	{
		bBoolValue = InCheckState == ECheckBoxState::Checked;
		ExecuteOnValueChanged();
	}

private:
	bool bBoolValue;
};

TSharedPtr<SNiagaraParameterEditor> FNiagaraEditorBoolTypeUtilities::CreateParameterEditor() const
{
	return SNew(SNiagaraBoolParameterEditor);
}