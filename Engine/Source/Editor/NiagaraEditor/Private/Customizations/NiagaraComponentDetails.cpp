// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraComponentDetails.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "NiagaraComponent.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "Widgets/SToolTip.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "NiagaraEffectInstance.h"
#include "Private/ViewModels/NiagaraParameterViewModel.h"
#include "Private/NiagaraEditorUtilities.h"
#include "INiagaraEditorTypeUtilities.h"
#include "NiagaraTypes.h"
#include "ScopedTransaction.h"
#include "SNiagaraParameterEditor.h"
#include "NiagaraEditorModule.h"
#include "PropertyEditorModule.h"
#include "ModuleManager.h"
#include "IStructureDetailsView.h"
#include "IDetailsView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableText.h"
#include "Editor.h"
#define LOCTEXT_NAMESPACE "NiagaraComponentDetails"

class FNiagaraParameterViewModelCustomDetails : public FNiagaraParameterViewModel
{
public:
	FNiagaraParameterViewModelCustomDetails(UNiagaraComponent* InComponent, const FNiagaraVariable& InSourceVariable, ENiagaraParameterEditMode ParameterEditMode) : FNiagaraParameterViewModel(ParameterEditMode)
	{
		Component = InComponent;
		Id = InSourceVariable.GetId();
		DefaultValueType = INiagaraParameterViewModel::EDefaultValueType::Struct;
		SynchronizeWithSource();
		RefreshParameterValue();
	}

	//~ INiagaraParameterViewModel interface
	virtual FGuid GetId() const override 
	{
		return Id;
	};
	
	virtual FName GetName() const 
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		return SrcVariable->GetName();
	}

	virtual bool CanRenameParameter() const override
	{
		return false; 
	}

	virtual FText GetNameText() const override 
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		return FText::FromName(SrcVariable->GetName()); 
	}
	
	virtual bool CanChangeParameterType() const override 
	{
		return false; 
	}

	virtual void NameTextComitted(const FText& Name, ETextCommit::Type CommitInfo) override
	{
		/*do nothing*/ 
	}

	virtual FText GetTypeDisplayName() const override
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		return SrcVariable->GetType().GetNameText(); 
	}

	virtual TSharedPtr<FNiagaraTypeDefinition> GetType() const override 
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		return MakeShareable(new FNiagaraTypeDefinition(SrcVariable->GetType()));
	}

	virtual void SelectedTypeChanged(TSharedPtr<FNiagaraTypeDefinition> Item, ESelectInfo::Type SelectionType) override 
	{
		/*do nothing*/
	}

	virtual EDefaultValueType GetDefaultValueType() override 
	{
		return DefaultValueType; 
	}
	
	virtual TSharedRef<FStructOnScope> GetDefaultValueStruct() override
	{
		return ParameterValue.ToSharedRef(); 
	}
	
	virtual UObject* GetDefaultValueObject() override
	{
		return nullptr; 
	}
	
	virtual void NotifyDefaultValuePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) override
	{
		NotifyDefaultValueChanged();
		//OnDefaultValueChangedDelegate.Broadcast(&GraphVariable);
	}

	virtual void NotifyBeginDefaultValueChange() override
	{
		GEditor->BeginTransaction(LOCTEXT("BeginEditParameterValue", "Edit parameter value"));
		Component->Modify();
	}

	virtual void NotifyEndDefaultValueChange() override
	{
		if (GEditor->IsTransactionActive())
		{
			GEditor->EndTransaction();
		}
	}

	bool DataMatches(const uint8* DataA, const uint8* DataB, int32 LengthA, int32 LengthB)
	{
		if (LengthA != LengthB)
		{
			return false;
		}

		for (int32 i = 0; i < LengthA; ++i)
		{
			if (DataA[i] != DataB[i])
			{
				return false;
			}
		}
		return true;
	}

	virtual void NotifyDefaultValueChanged() override
	{
		FNiagaraVariable* ExistingVar = GetExistingOverride();
		const FNiagaraVariable* SrcVariable = GetSourceVariable();

		if (ParameterValue.IsValid() == false)
		{
			return;
		}

		// Get the size of the struct that we are referencing...
		const UStruct* Struct = ParameterValue->GetStruct();
		int32 StructSize = -1;
		if (Struct)
		{
			StructSize = Struct->GetStructureSize();
		}

		// Now let's see if we need to update the value because it has changed...
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct && SrcVariable != nullptr)
		{
			// Does the new value match exactly the source variable's value?
			bool bDataMatchesSrc = DataMatches(SrcVariable->GetData(), ParameterValue->GetStructMemory(), SrcVariable->GetSizeInBytes(), StructSize);

			// Does the new value match exactly the override variable's value?
			bool bDataMatchesOverride = ExistingVar != nullptr ? DataMatches(ExistingVar->GetData(), ParameterValue->GetStructMemory(), ExistingVar->GetSizeInBytes(), StructSize) : false;

			// If we have an override, use whether or not it matches the current override value to determine if we set the variable.
			bool bUpdateValue = true;
			if (ExistingVar != nullptr && bDataMatchesOverride)
			{
				bUpdateValue = false;
			}
			// If we don't have an override, use whether or not it matches the source variable's value to determine if we set the variable.
			else if (ExistingVar == nullptr && bDataMatchesSrc)
			{
				bUpdateValue = false;
			}

			// Actually transact the change if needed...
			if (bUpdateValue)
			{
				FScopedTransaction ScopedTransaction(LOCTEXT("EditParameterValue", "Edit parameter value"));
				Component->Modify();
				FNiagaraVariable* CopyOnWriteVariable = CopyOnWrite();
				if (CopyOnWriteVariable != nullptr)
				{
					CopyOnWriteVariable->SetData(ParameterValue->GetStructMemory());
				}
			}
		}
	}

	void RefreshParameterValue()
	{
		// If the compiled variable is null or the type of the graph variable has been changed,
		// then we edit the value in the graph variable so that the value is of the correct type and can be edited.
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		const FNiagaraVariable* CurrentOverride = GetExistingOverride();
		const FNiagaraVariable* ValueVariable = CurrentOverride != nullptr ? CurrentOverride : SrcVariable;

		if (ValueVariable == nullptr)
		{
			UE_LOG(LogNiagaraEditor, Warning, TEXT("Parameter doesn't exist!"));
			return;
		}

		if (ParameterValue.IsValid() == false)
		{
			ParameterValue = MakeShareable(new FStructOnScope(ValueVariable->GetType().GetStruct()));
		}

		const UStruct* Struct = ParameterValue->GetStruct();
		int32 StructSize = -1;
		if (Struct)
		{
			StructSize = Struct->GetStructureSize();
		}

		if (ValueVariable->IsDataAllocated() && ValueVariable->GetSizeInBytes() == StructSize)
		{
			ValueVariable->CopyTo(ParameterValue->GetStructMemory());

			if (ParameterEditor.IsValid())
			{
				ParameterEditor->UpdateInternalValueFromStruct(ParameterValue.ToSharedRef());
			}
		}
		else
		{
			UE_LOG(LogNiagaraEditor, Warning, TEXT("Parameter %s is not allocated or sizes mismatch!"), *ValueVariable->GetName().ToString());
		}
	}

	FNiagaraVariable* CopyOnWrite()
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		FNiagaraVariable* ExistingVar = GetExistingOverride();
		if (ExistingVar == nullptr && SrcVariable != nullptr)
		{
			int32 AddIndex = Component->EffectParameterLocalOverrides.Add(*SrcVariable);
			ExistingVar = &Component->EffectParameterLocalOverrides[AddIndex];

			TSharedRef<FNiagaraEffectInstance> Instance = Component->GetEffectInstance();
			Instance->InvalidateComponentBindings();
		}
		if (ExistingVar != nullptr && ExistingVar->GetType() != SrcVariable->GetType())
		{
			ExistingVar->SetType(SrcVariable->GetType());
		}
		if (ExistingVar != nullptr && ExistingVar->GetName() != SrcVariable->GetName())
		{
			ExistingVar->SetName(SrcVariable->GetName());
		}
		return ExistingVar;
	}

	FNiagaraVariable* GetExistingOverride() const
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		if (SrcVariable == nullptr)
		{
			return nullptr;
		}

		return Component->EffectParameterLocalOverrides.FindByPredicate([&](const FNiagaraVariable& Var)
		{
			return Var.GetId() == SrcVariable->GetId();
		});
	}

	void SynchronizeWithSource()
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		FNiagaraVariable* ExistingVar = GetExistingOverride();
		if (ExistingVar != nullptr && ExistingVar->GetType() != SrcVariable->GetType())
		{
			ExistingVar->SetType(SrcVariable->GetType());
		}
		if (ExistingVar != nullptr && ExistingVar->GetName() != SrcVariable->GetName())
		{
			ExistingVar->SetName(SrcVariable->GetName());
		}
	}

	bool IsOverriddenLocally() const
	{
		return GetExistingOverride() != nullptr;
	}

	void ResetToDefaults()
	{
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		if (SrcVariable == nullptr)
		{
			return;
		}

		int32 OverrideIndex = Component->EffectParameterLocalOverrides.IndexOfByPredicate([&](const FNiagaraVariable& Var)
		{
			return Var.GetId() == SrcVariable->GetId();
		});

		if (OverrideIndex >= 0)
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("ResetParameterValue", "Reset parameter value to defaults."));
			Component->Modify();
			Component->EffectParameterLocalOverrides.RemoveAt(OverrideIndex);

			TSharedRef<FNiagaraEffectInstance> Instance = Component->GetEffectInstance();
			Instance->InvalidateComponentBindings();
			RefreshParameterValue();
		}
	}

	void SetParameterEditor(TSharedPtr<SNiagaraParameterEditor>& InParameterEditor)
	{
		ParameterEditor = InParameterEditor;
	}

	void Reset()
	{
		Component = nullptr;
		Id.Invalidate();
		ParameterEditor = nullptr;
		ParameterValue = nullptr;
	}

	const FNiagaraVariable* GetSourceVariable() const
	{
		if (Component != nullptr)
		{
			TSharedRef<FNiagaraEffectInstance> EffectInstance = Component->GetEffectInstance();
			const FNiagaraParameters& Params = EffectInstance->GetExternalInstanceParameters();
			const FNiagaraVariable* SrcVariable = Params.FindParameter(Id);
			return SrcVariable;
		}
		return nullptr;
	}

protected:
	UNiagaraComponent* Component; 
	FGuid Id;
	EDefaultValueType DefaultValueType;
	TSharedPtr<FStructOnScope> ParameterValue;
	TSharedPtr<SNiagaraParameterEditor> ParameterEditor;
};


TSharedRef<IDetailCustomization> FNiagaraComponentDetails::MakeInstance()
{
	return MakeShareable(new FNiagaraComponentDetails);
}

FNiagaraComponentDetails::FNiagaraComponentDetails() : Builder(nullptr), bQueueForDetailsRefresh(false)
{
	GEditor->RegisterForUndo(this);
}


FNiagaraComponentDetails::~FNiagaraComponentDetails()
{
	GEditor->UnregisterForUndo(this);

	if (ObjectsCustomized.Num() == 1)
	{
		UNiagaraComponent* Component = Cast<UNiagaraComponent>(ObjectsCustomized[0].Get());

		if (Component)
		{
			TSharedRef<FNiagaraEffectInstance> EffectInstance = Component->GetEffectInstance();
			if (OnResetDelegateHandle.IsValid())
			{
				EffectInstance->OnReset().Remove(OnResetDelegateHandle);
			}

			if (OnInitDelegateHandle.IsValid())
			{
				EffectInstance->OnInitialized().Remove(OnInitDelegateHandle);
			}
		}
	}
}

bool FNiagaraComponentDetails::IsTickable() const
{
	return bQueueForDetailsRefresh;
}

void FNiagaraComponentDetails::Tick(float DeltaTime)
{
	if (bQueueForDetailsRefresh)
	{
		if (Builder)
		{
			Builder->ForceRefreshDetails();
			bQueueForDetailsRefresh = false;
		}
	}
}

void FNiagaraComponentDetails::PostUndo(bool bSuccess)
{
	if (Builder)
	{
		// This is tricky. There is a high chance that if the component changed, then
		// any cached FNiagaraVariable that we're holding on to may have been changed out from underneath us.
		// So we essentially must tear down and start again in the UI.
		// HOWEVER, a refresh will invoke a new constructor of the FNiagaraComponentDetails, which puts us 
		// in the queue to handle PostUndo events. This turns quickly into an infinitely loop. Therefore,
		// we circumvent this by telling the editor that we need to queue up an event
		// that we will handle in the subsequent frame's Tick event. Not the cleanest approach, 
		// but because we are doing things like CopyOnWrite, the normal Unreal property editing
		// path is not available to us.
		bQueueForDetailsRefresh = true;
		for (TSharedPtr<FNiagaraParameterViewModelCustomDetails>& ViewModel : ViewModels)
		{
			ViewModel->Reset();
		}
	}
}

TStatId FNiagaraComponentDetails::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FNiagaraComponentDetails, STATGROUP_Tickables);
}

void FNiagaraComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{

	Builder = &DetailBuilder;

	// Create a category so this is displayed early in the properties
	IDetailCategoryBuilder& NiagaraCategory = DetailBuilder.EditCategory("Niagara", FText::GetEmpty(), ECategoryPriority::Important);

	// Store off the properties for analysis in later function calls.
	LocalOverridesPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraComponent, EffectParameterLocalOverrides));
	if (LocalOverridesPropertyHandle.IsValid())
	{
		LocalOverridesPropertyHandle->MarkHiddenByCustomization();
	}

	TSharedPtr<IPropertyHandle> AssetPropertyHandle = DetailBuilder.GetProperty(FName("Asset"));
	if (AssetPropertyHandle.IsValid())
	{
		IDetailPropertyRow& AssetRow = NiagaraCategory.AddProperty(AssetPropertyHandle, EPropertyLocation::Default);
	}

	// Store off the objects that we are editing for analysis in later function calls.
	DetailBuilder.GetObjectsBeingCustomized(ObjectsCustomized);

	if (ObjectsCustomized.Num() == 1)
	{
		UNiagaraComponent* Component = Cast<UNiagaraComponent>(ObjectsCustomized[0].Get());
		if (Component == nullptr)
		{
			return;
		}

		TSharedRef<FNiagaraEffectInstance> EffectInstance = Component->GetEffectInstance();
		ViewModels.Empty();

		UNiagaraEffect* Effect = Component->GetAsset();
		
		// We'll want to monitor for any serious changes that might require a rebuild of the UI from scratch.
		OnResetDelegateHandle = EffectInstance->OnReset().AddSP(this, &FNiagaraComponentDetails::OnEffectInstanceReset);
		OnInitDelegateHandle = EffectInstance->OnInitialized().AddSP(this, &FNiagaraComponentDetails::OnEffectInstanceReset);

		// Go through each instance variable and add new GUI for it.
		for (const FNiagaraVariable& Variable : EffectInstance->GetExternalInstanceParameters().Parameters)
		{
			TSharedPtr<FNiagaraParameterViewModelCustomDetails> ViewModel = MakeShareable(new FNiagaraParameterViewModelCustomDetails(Component, Variable, ENiagaraParameterEditMode::EditValueOnly));
			
			TSharedPtr<SNiagaraParameterEditor> ParameterEditor;
			TSharedPtr<SWidget> DetailsWidget;
			if (ViewModel->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Struct)
			{
				FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
				TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilities = NiagaraEditorModule.GetTypeUtilities(*ViewModel->GetType().Get());
				if (TypeEditorUtilities.IsValid() && TypeEditorUtilities->CanCreateParameterEditor())
				{
					ParameterEditor = TypeEditorUtilities->CreateParameterEditor();
					ParameterEditor->UpdateInternalValueFromStruct(ViewModel->GetDefaultValueStruct());
					ParameterEditor->SetOnBeginValueChange(SNiagaraParameterEditor::FOnValueChange::CreateSP(
						this, &FNiagaraComponentDetails::ParameterEditorBeginValueChange, ViewModel.ToSharedRef()));
					ParameterEditor->SetOnEndValueChange(SNiagaraParameterEditor::FOnValueChange::CreateSP(
						this, &FNiagaraComponentDetails::ParameterEditorEndValueChange, ViewModel.ToSharedRef()));
					ParameterEditor->SetOnValueChanged(SNiagaraParameterEditor::FOnValueChange::CreateSP(
						this, &FNiagaraComponentDetails::ParameterEditorValueChanged, ParameterEditor.ToSharedRef(), ViewModel.ToSharedRef()));
					ViewModel->SetParameterEditor(ParameterEditor);
					DetailsWidget = ParameterEditor;
				}
				else
				{
					UE_LOG(LogNiagaraEditor, Warning, TEXT("Cannot create editor for type: %s"), *ViewModel->GetTypeDisplayName().ToString());
				}
			}
			else if (ViewModel->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Object)
			{
				UE_LOG(LogNiagaraEditor, Warning, TEXT("Unsupported type for component editing: %s"), *ViewModel->GetTypeDisplayName().ToString());
			}
			else
			{
				UE_LOG(LogNiagaraEditor, Warning, TEXT("Unsupported type for component editing: %s"), *ViewModel->GetTypeDisplayName().ToString());
			}

			if (DetailsWidget.IsValid())
			{
				ViewModels.Add(ViewModel);
				FDetailWidgetRow& OverrideRow = NiagaraCategory.AddCustomRow(FText::FromName(Variable.GetName()));
				OverrideRow.NameContent()
				.MinDesiredWidth(0)
				.MaxDesiredWidth(0)
				[
					SNew(STextBlock)
					.Text(FText::FromName(Variable.GetName()))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				];

				OverrideRow.ValueContent()
				.MinDesiredWidth(375.0f)
				.MaxDesiredWidth(375.0f)
				//.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.Padding(4.0f)
					[
						// Add in the parameter editor factoried above.
						DetailsWidget.ToSharedRef()
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						
						// Add in the "reset to default" buttons
						SNew(SButton)
						.OnClicked(this, &FNiagaraComponentDetails::OnLocationResetClicked, ViewModel.ToSharedRef())
						.Visibility(this, &FNiagaraComponentDetails::GetLocationResetVisibility, ViewModel.ToSharedRef())
						.ContentPadding(FMargin(5.f, 0.f))
						.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
						.ButtonStyle(FEditorStyle::Get(), "NoBorder")
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
						]
					]
				];
			}
		}

		// Let the user know that there is nothing to tweak if there are no editable parameters!
		if (ViewModels.Num() == 0)
		{
			FText WarningText = LOCTEXT("NiagaraNoExposedParams", "This effect has no supported exposed parameters.\nPlease add them in the Niagara Effect Editor's Effect Script Tab.");
			FDetailWidgetRow& OverrideRow = NiagaraCategory.AddCustomRow(WarningText)
				.WholeRowContent()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(WarningText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
				];
		}

	}
	else
	{
		FText WarningText = LOCTEXT("NiagaraMultiSelectWarning", "Multiple selection currently unsupported.");
		FDetailWidgetRow& OverrideRow = NiagaraCategory.AddCustomRow(WarningText)
			.WholeRowContent()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(WarningText)
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
			];
	}
}

void FNiagaraComponentDetails::OnEffectInstanceReset()
{
	bQueueForDetailsRefresh = true;
	for (TSharedPtr<FNiagaraParameterViewModelCustomDetails>& ViewModel : ViewModels)
	{
		ViewModel->Reset();
	}
}

void FNiagaraComponentDetails::ParameterEditorBeginValueChange(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item)
{
	Item->NotifyBeginDefaultValueChange();
}

void FNiagaraComponentDetails::ParameterEditorEndValueChange(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item)
{
	Item->NotifyEndDefaultValueChange();
}

void FNiagaraComponentDetails::ParameterEditorValueChanged(TSharedRef<SNiagaraParameterEditor> ParameterEditor, TSharedRef<FNiagaraParameterViewModelCustomDetails> Item)
{
	ParameterEditor->UpdateStructFromInternalValue(Item->GetDefaultValueStruct());
	Item->NotifyDefaultValueChanged();
}


FReply FNiagaraComponentDetails::OnEffectOpenRequested(UNiagaraEffect* InEffect)
{
	if (InEffect)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(InEffect);
	}
	return FReply::Handled();
}

FReply FNiagaraComponentDetails::OnLocationResetClicked(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item)
{
	Item->ResetToDefaults();
	return FReply::Handled();
}

EVisibility FNiagaraComponentDetails::GetLocationResetVisibility(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item) const
{
	return Item->IsOverriddenLocally() ? EVisibility::Visible : EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
