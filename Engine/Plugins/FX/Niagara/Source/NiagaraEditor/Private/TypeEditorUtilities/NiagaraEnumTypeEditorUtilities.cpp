// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEnumTypeEditorUtilities.h"
#include "SNiagaraParameterEditor.h"
#include "NiagaraTypes.h"
#include "NiagaraEditorStyle.h"
#include "NiagaraEditorCommon.h"

#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboBox.h"

class SEnumCombobox : public SComboBox<TSharedPtr<int32>>
{
public:
	DECLARE_DELEGATE_TwoParams(FOnValueChanged, int32, ESelectInfo::Type);

public:
	SLATE_BEGIN_ARGS(SEnumCombobox) {}
		SLATE_ATTRIBUTE(int32, Value)
		SLATE_EVENT(FOnValueChanged, OnValueChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const UEnum* InEnum)
	{
		Enum = InEnum;
		Value = InArgs._Value;
		check(Value.IsBound());
		OnValueChanged = InArgs._OnValueChanged;

		bUpdatingSelectionInternally = false;

		for (int32 i = 0; i < Enum->NumEnums() - 1; i++)
		{
			if (Enum->HasMetaData(TEXT("Hidden"), i) == false)
			{
				VisibleEnumNameIndices.Add(MakeShareable(new int32(i)));
			}
		}

		SComboBox::Construct(SComboBox<TSharedPtr<int32>>::FArguments()
			//.ButtonStyle(FEditorStyle::Get(), "FlatButton.Light")
			.OptionsSource(&VisibleEnumNameIndices)
			.OnGenerateWidget(this, &SEnumCombobox::OnGenerateWidget)
			.OnSelectionChanged(this, &SEnumCombobox::OnComboSelectionChanged)
			.OnComboBoxOpening(this, &SEnumCombobox::OnComboMenuOpening)
			.ContentPadding(FMargin(2, 0))
			[
				SNew(STextBlock)
				//.Font(FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont"))
				.Text(this, &SEnumCombobox::GetValueText)
			]);
	}

private:
	FText GetValueText() const
	{
		int32 ValueNameIndex = Enum->GetIndexByValue(Value.Get());
		return Enum->GetDisplayNameTextByIndex(ValueNameIndex);
	}

	TSharedRef<SWidget> OnGenerateWidget(TSharedPtr<int32> InItem)
	{
		return SNew(STextBlock)
			.Text(Enum->GetDisplayNameTextByIndex(*InItem));
	}

	void OnComboSelectionChanged(TSharedPtr<int32> InSelectedItem, ESelectInfo::Type SelectInfo)
	{
		if (bUpdatingSelectionInternally == false)
		{
			OnValueChanged.ExecuteIfBound(*InSelectedItem, SelectInfo);
		}
	}

	void OnComboMenuOpening()
	{
		int32 CurrentNameIndex = Enum->GetIndexByValue(Value.Get());
		TSharedPtr<int32> FoundNameIndexItem;
		for (int32 i = 0; i < VisibleEnumNameIndices.Num(); i++)
		{
			if (*VisibleEnumNameIndices[i] == CurrentNameIndex)
			{
				FoundNameIndexItem = VisibleEnumNameIndices[i];
				break;
			}
		}
		if (FoundNameIndexItem.IsValid())
		{
			bUpdatingSelectionInternally = true;
			SetSelectedItem(FoundNameIndexItem);
			bUpdatingSelectionInternally = false;
		}
	}

private:
	const UEnum* Enum;

	TAttribute<int32> Value;

	TArray<TSharedPtr<int32>> VisibleEnumNameIndices;

	bool bUpdatingSelectionInternally;

	FOnValueChanged OnValueChanged;
};

class SNiagaraEnumParameterEditor : public SNiagaraParameterEditor
{
public:
	SLATE_BEGIN_ARGS(SNiagaraEnumParameterEditor) { }
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, const UEnum* Enum)
	{
		ChildSlot
		[
			SNew(SEnumCombobox, Enum)
			.Value(this, &SNiagaraEnumParameterEditor::GetValue)
			.OnValueChanged(this, &SNiagaraEnumParameterEditor::ValueChanged)
		];
	}

	virtual void UpdateInternalValueFromStruct(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetIntStruct(), TEXT("Struct type not supported."));
		IntValue = ((FNiagaraInt32*)Struct->GetStructMemory())->Value;
	}

	virtual void UpdateStructFromInternalValue(TSharedRef<FStructOnScope> Struct) override
	{
		checkf(Struct->GetStruct() == FNiagaraTypeDefinition::GetIntStruct(), TEXT("Struct type not supported."));
		((FNiagaraInt32*)Struct->GetStructMemory())->Value = IntValue;
	}

private:
	int32 GetValue() const
	{
		return IntValue;
	}

	void ValueChanged(int32 Value, ESelectInfo::Type SelectionType)
	{
		IntValue = Value;
		ExecuteOnValueChanged();
	}

private:
	int32 IntValue;
};

bool FNiagaraEditorEnumTypeUtilities::CanProvideDefaultValue() const
{
	return true;
}

void FNiagaraEditorEnumTypeUtilities::UpdateVariableWithDefaultValue(FNiagaraVariable& Variable) const
{
	const UEnum* Enum = Variable.GetType().GetEnum();
	checkf(Enum != nullptr, TEXT("Variable is not an enum type."));

	FNiagaraInt32 EnumIntValue;
	EnumIntValue.Value = Enum->GetValueByIndex(0);

	Variable.SetValue<FNiagaraInt32>(EnumIntValue);
}

TSharedPtr<SNiagaraParameterEditor> FNiagaraEditorEnumTypeUtilities::CreateParameterEditor(const FNiagaraTypeDefinition& ParameterType) const
{
	return SNew(SNiagaraEnumParameterEditor, ParameterType.GetEnum());
}

bool FNiagaraEditorEnumTypeUtilities::CanHandlePinDefaults() const
{
	return true;
}

FString FNiagaraEditorEnumTypeUtilities::GetPinDefaultStringFromValue(const FNiagaraVariable& AllocatedVariable) const
{
	checkf(AllocatedVariable.IsDataAllocated(), TEXT("Can not generate a default value string for an unallocated variable."));

	const UEnum* Enum = AllocatedVariable.GetType().GetEnum();
	checkf(Enum != nullptr, TEXT("Variable is not an enum type."));

	int64 EnumValue = AllocatedVariable.GetValue<int32>();
	if(Enum->IsValidEnumValue(EnumValue))
	{
		return Enum->GetNameStringByValue(EnumValue);
	}
	else
	{
		FString ReplacementForInvalidValue = Enum->GetNameStringByIndex(0);
		UE_LOG(LogNiagaraEditor, Error, TEXT("Error getting default value for enum pin.  Enum value %i is not supported for enum type %s.  Using value %s"),
			EnumValue, *AllocatedVariable.GetType().GetName(), *ReplacementForInvalidValue);
		return ReplacementForInvalidValue;
	}
}

bool FNiagaraEditorEnumTypeUtilities::SetValueFromPinDefaultString(const FString& StringValue, FNiagaraVariable& Variable) const
{
	const UEnum* Enum = Variable.GetType().GetEnum();
	checkf(Enum != nullptr, TEXT("Variable is not an enum type."));

	FNiagaraInt32 EnumValue;
	EnumValue.Value = (int32)Enum->GetValueByNameString(StringValue, EGetByNameFlags::ErrorIfNotFound);
	if(EnumValue.Value != INDEX_NONE)
	{
		Variable.AllocateData();
		Variable.SetValue<FNiagaraInt32>(EnumValue);
		return true;
	}
	return false;
}