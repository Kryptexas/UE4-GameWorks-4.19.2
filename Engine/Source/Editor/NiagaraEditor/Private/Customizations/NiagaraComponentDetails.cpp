// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraComponentDetails.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterface.h"
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
#include "NiagaraParameterCollectionViewModel.h"
#include "NiagaraScriptSource.h"
#include "NiagaraNodeInput.h"
#include "NiagaraGraph.h"
#include "GameDelegates.h"
#define LOCTEXT_NAMESPACE "NiagaraComponentDetails"

class FNiagaraParameterViewModelCustomDetails : public FNiagaraParameterViewModel
{
public:
	FNiagaraParameterViewModelCustomDetails(UNiagaraComponent* InComponent, const FNiagaraVariable& InSourceVariable, ENiagaraParameterEditMode ParameterEditMode) : FNiagaraParameterViewModel(ParameterEditMode)
	{
		Component = InComponent;
		Id = InSourceVariable.GetId();
		DefaultValueType = INiagaraParameterViewModel::EDefaultValueType::Struct;
		SynchronizeWithSourceParameter();
		RefreshParameterValue();
	}

	FNiagaraParameterViewModelCustomDetails(UNiagaraComponent* InComponent, const FGuid& InId, const FName& InName, UNiagaraDataInterface* InDataInterface, ENiagaraParameterEditMode ParameterEditMode) : FNiagaraParameterViewModel(ParameterEditMode)
	{
		Component = InComponent;
		Id = InId;
		DefaultValueType = INiagaraParameterViewModel::EDefaultValueType::Object;
		ObjectValue.Id = InId;
		ObjectValue.Name = InName;
		// This needs to be in the component's package because otherwise we cannot select actors from the scene.
		ObjectValue.DataInterface =  Cast<UNiagaraDataInterface>(StaticDuplicateObject(InDataInterface, Component->GetOutermost(), NAME_None, RF_Transient));
		Component->EditorTemporaries.Add(ObjectValue.DataInterface);

		SynchronizeWithSourceDataInterface();
		RefreshDataInterfaceValue();
	}

	~FNiagaraParameterViewModelCustomDetails()
	{
		Reset();
	}
	
	//~ INiagaraParameterViewModel interface
	virtual FGuid GetId() const override 
	{
		return Id;
	};
	
	virtual FName GetName() const 
	{
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			const FNiagaraVariable* SrcVariable = GetSourceVariable();
			return SrcVariable->GetName();
		}
		else
		{
			const FNiagaraScriptDataInterfaceInfo* SrcDataInterface = GetSourceDataInterface();
			return SrcDataInterface->Name;
		}
	}

	virtual bool CanRenameParameter() const override
	{
		return false; 
	}

	virtual bool CanChangeSortOrder() const override
	{
		return false;
	}
	virtual int32 GetSortOrder() const override
	{
		if (Component ==nullptr || Component->GetAsset() == nullptr || Component->GetAsset()->GetEffectScript() == nullptr)
		{
			return 0;
		}
		UNiagaraScript* Script = Component->GetAsset()->GetEffectScript();
		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Script->Source);
		if (Source == nullptr || Source->NodeGraph == nullptr)
		{
			return 0;
		}
		TArray<UNiagaraNodeInput*> InputNodes;
		Source->NodeGraph->FindInputNodes(InputNodes);
		if (InputNodes.Num() == 0)
		{
			return 0;
		}

		for (UNiagaraNodeInput* Input : InputNodes)
		{
			if (Input->Usage == ENiagaraInputNodeUsage::Parameter && Input->Input.GetId() == GetId())
			{
				return Input->CallSortPriority;
			}
		}

		return 0;
		
	}
	virtual void SetSortOrder(int32 SortOrder) override
	{
		// do nothing
	}
	
	virtual FText GetNameText() const override 
	{
		FName Name;
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			const FNiagaraVariable* SrcVariable = GetSourceVariable();
			Name = SrcVariable->GetName();
		}
		else
		{
			const FNiagaraScriptDataInterfaceInfo* SrcDataInterface = GetSourceDataInterface();
			Name = SrcDataInterface->Name;
		}
		return FText::FromName(Name); 
	}
	
	virtual bool CanChangeParameterType() const override 
	{
		return false; 
	}

	virtual void NameTextComitted(const FText& Name, ETextCommit::Type CommitInfo) override
	{
		/*do nothing*/ 
	}

	virtual bool VerifyNodeNameTextChanged(const FText& NewText, FText& OutErrorMessage)
	{
		/* do nothing*/
		return true;
	}

	virtual FText GetTypeDisplayName() const override
	{
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			const FNiagaraVariable* SrcVariable = GetSourceVariable();
			return SrcVariable->GetType().GetNameText();
		}
		else
		{
			const FNiagaraScriptDataInterfaceInfo* SrcDataInterface = GetSourceDataInterface();
			FNiagaraTypeDefinition TypeDef(SrcDataInterface->DataInterface->GetClass());
			return TypeDef.GetNameText();
		}
	}

	virtual TSharedPtr<FNiagaraTypeDefinition> GetType() const override 
	{
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			const FNiagaraVariable* SrcVariable = GetSourceVariable();
			return MakeShareable(new FNiagaraTypeDefinition(SrcVariable->GetType()));
		}
		else
		{
			const FNiagaraScriptDataInterfaceInfo* SrcDataInterface = GetSourceDataInterface();
			return MakeShareable(new FNiagaraTypeDefinition(SrcDataInterface->DataInterface->GetClass()));
		}
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
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct);
		return ParameterValue.ToSharedRef(); 
	}
	
	virtual UObject* GetDefaultValueObject() override
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Object);
		return ObjectValue.DataInterface;
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
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			FNiagaraVariable* ExistingVar = GetExistingParameterOverride();
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
			if (SrcVariable != nullptr)
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
					FNiagaraVariable* CopyOnWriteVariable = CopyOnWriteParameter();
					if (CopyOnWriteVariable != nullptr)
					{
						CopyOnWriteVariable->SetData(ParameterValue->GetStructMemory());
					}
				}
			}
		}
		else
		{
			FNiagaraScriptDataInterfaceInfo* ExistingDataInterface = GetExistingDataInterfaceOverride();
			const FNiagaraScriptDataInterfaceInfo* SrcDataInterface = GetSourceDataInterface();

			if (ObjectValue.DataInterface == nullptr)
			{
				return;
			}

			// Now let's see if we need to update the value because it has changed...
			if ( SrcDataInterface != nullptr)
			{
				// Does the new value match exactly the source variable's value?
				bool bDataMatchesSrc = SrcDataInterface->DataInterface->Equals(ObjectValue.DataInterface);

				// Does the new value match exactly the override variable's value?
				bool bDataMatchesOverride = ExistingDataInterface != nullptr ? ExistingDataInterface->DataInterface->Equals(ObjectValue.DataInterface) : false;

				// If we have an override, use whether or not it matches the current override value to determine if we set the variable.
				bool bUpdateValue = true;
				if (ExistingDataInterface != nullptr && bDataMatchesOverride)
				{
					bUpdateValue = false;
				}
				// If we don't have an override, use whether or not it matches the source variable's value to determine if we set the variable.
				else if (SrcDataInterface == nullptr && bDataMatchesSrc)
				{
					bUpdateValue = false;
				}

				// Actually transact the change if needed...
				if (bUpdateValue)
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("EditParameterValue", "Edit parameter value"));
					Component->Modify();
					FNiagaraScriptDataInterfaceInfo* CopyOnWriteVariable = CopyOnWriteDataInterface();
					if (CopyOnWriteVariable != nullptr)
					{
						ensure(ObjectValue.DataInterface->CopyTo(CopyOnWriteVariable->DataInterface));
						// Once we've changed a data interface, it may be wired to multiple
						// simulations. For instance, the mesh for mesh emission might affect
						// three different simulations. We need to reinitialize the data interfaces
						// as a result.
						Component->GetEffectInstance()->ReinitializeDataInterfaces();
					}
				}
			}
		}
	}

	void RefreshParameterValue()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct);
		// If the compiled variable is null or the type of the graph variable has been changed,
		// then we edit the value in the graph variable so that the value is of the correct type and can be edited.
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		const FNiagaraVariable* CurrentOverride = GetExistingParameterOverride();
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

	void RefreshDataInterfaceValue()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Object);

		// If the compiled variable is null or the type of the graph variable has been changed,
		// then we edit the value in the graph variable so that the value is of the correct type and can be edited.
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetSourceDataInterface();
		const FNiagaraScriptDataInterfaceInfo* CurrentOverride = GetExistingDataInterfaceOverride();
		const FNiagaraScriptDataInterfaceInfo* ValueVariable = CurrentOverride != nullptr ? CurrentOverride : SrcVariable;

		if (ValueVariable == nullptr)
		{
			UE_LOG(LogNiagaraEditor, Warning, TEXT("Data interface doesn't exist!"));
			return;
		}

		check(ObjectValue.DataInterface != nullptr);

		check(ValueVariable->DataInterface->GetClass() == ObjectValue.DataInterface->GetClass());
		ensure(ValueVariable->DataInterface->CopyTo(ObjectValue.DataInterface));

		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(ObjectValue.DataInterface);
		}
	}

	FNiagaraVariable* CopyOnWriteParameter()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct);

		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		FNiagaraVariable* ExistingVar = GetExistingParameterOverride();
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

	FNiagaraScriptDataInterfaceInfo* CopyOnWriteDataInterface()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Object);

		bool bInvalidate = false;
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetSourceDataInterface();
		FNiagaraScriptDataInterfaceInfo* ExistingVar = GetExistingDataInterfaceOverride();
		if (ExistingVar == nullptr && SrcVariable != nullptr)
		{
			int32 AddIndex = Component->EffectDataInterfaceLocalOverrides.AddDefaulted();
			ExistingVar = &Component->EffectDataInterfaceLocalOverrides[AddIndex];
			SrcVariable->CopyTo(ExistingVar, Component);
			bInvalidate = true;
		}

		if (ExistingVar != nullptr && ExistingVar->DataInterface->GetClass() != SrcVariable->DataInterface->GetClass())
		{
			SrcVariable->CopyTo(ExistingVar, Component);
		}

		if (ExistingVar != nullptr && ExistingVar->Name != SrcVariable->Name)
		{
			ExistingVar->Name = SrcVariable->Name;
		}

		if (bInvalidate)
		{
			TSharedRef<FNiagaraEffectInstance> Instance = Component->GetEffectInstance();
			Instance->InvalidateComponentBindings();
		}
		return ExistingVar;
	}

	FNiagaraVariable* GetExistingParameterOverride() const
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct);
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

	FNiagaraScriptDataInterfaceInfo* GetExistingDataInterfaceOverride() const
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Object);
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetSourceDataInterface();
		if (SrcVariable == nullptr)
		{
			return nullptr;
		}

		return Component->EffectDataInterfaceLocalOverrides.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
		{
			return Var.Id == SrcVariable->Id;
		});
	}

	void SynchronizeWithSourceParameter()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct);
		const FNiagaraVariable* SrcVariable = GetSourceVariable();
		FNiagaraVariable* ExistingVar = GetExistingParameterOverride();
		if (ExistingVar != nullptr && ExistingVar->GetType() != SrcVariable->GetType())
		{
			ExistingVar->SetType(SrcVariable->GetType());
		}
		if (ExistingVar != nullptr && ExistingVar->GetName() != SrcVariable->GetName())
		{
			ExistingVar->SetName(SrcVariable->GetName());
		}
	}

	void SynchronizeWithSourceDataInterface()
	{
		check(DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Object);
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetSourceDataInterface();
		FNiagaraScriptDataInterfaceInfo* ExistingVar = GetExistingDataInterfaceOverride();
		if (ExistingVar != nullptr && ExistingVar->DataInterface->GetClass() != SrcVariable->DataInterface->GetClass())
		{
			SrcVariable->CopyTo(ExistingVar, Component);
		}
		if (ExistingVar != nullptr && ExistingVar->Name != SrcVariable->Name)
		{
			ExistingVar->Name = SrcVariable->Name;
		}
	}

	bool IsOverriddenLocally() const
	{
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
		{
			return GetExistingParameterOverride() != nullptr;

		}
		else
		{
			return GetExistingDataInterfaceOverride() != nullptr;
		}
	}

	void ResetToDefaults()
	{
		if (DefaultValueType == INiagaraParameterViewModel::EDefaultValueType::Struct)
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
		else
		{
			const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetSourceDataInterface();
			if (SrcVariable == nullptr)
			{
				return;
			}

			int32 OverrideIndex = Component->EffectDataInterfaceLocalOverrides.IndexOfByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
			{
				return Var.Id == SrcVariable->Id;
			});

			if (OverrideIndex >= 0)
			{
				FScopedTransaction ScopedTransaction(LOCTEXT("ResetParameterValue", "Reset parameter value to defaults."));
				Component->Modify();
				Component->EffectDataInterfaceLocalOverrides.RemoveAt(OverrideIndex);

				TSharedRef<FNiagaraEffectInstance> Instance = Component->GetEffectInstance();
				Instance->InvalidateComponentBindings();
				RefreshDataInterfaceValue();
			}
		}
	}

	void SetParameterEditor(TSharedPtr<SNiagaraParameterEditor>& InParameterEditor)
	{
		ParameterEditor = InParameterEditor;
	}

	void SetDataInterfaceEditor (TSharedPtr<IDetailsView>& InDetailsView)
	{
		DetailsView = InDetailsView;
	}

	void Reset()
	{
		if (Component)
		{
			Component->EditorTemporaries.Remove(ObjectValue.DataInterface);
		}
		Component = nullptr;
		Id.Invalidate();
		ParameterEditor = nullptr;
		ParameterValue = nullptr;
		ObjectValue.DataInterface = nullptr;
		ObjectValue.Id.Invalidate();
		ObjectValue.Name = NAME_None;

		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(nullptr);
			DetailsView = nullptr;
		}
	}

	const FNiagaraVariable* GetSourceVariable() const
	{
		if (Component != nullptr)
		{
			UNiagaraEffect* Effect = Component->GetAsset();
			const FNiagaraParameters& Params = Effect->GetEffectScript()->Parameters;
			const FNiagaraVariable* SrcVariable = Params.FindParameter(Id);
			return SrcVariable;
		}
		return nullptr;
	}

	const FNiagaraScriptDataInterfaceInfo* GetSourceDataInterface() const
	{
		if (Component != nullptr)
		{
			UNiagaraEffect* Effect = Component->GetAsset();
			const TArray<FNiagaraScriptDataInterfaceInfo>& Params = Effect->GetEffectScript()->DataInterfaceInfo;
			const FNiagaraScriptDataInterfaceInfo* SrcVariable = Params.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
			{
				return Var.Id == Id;
			});

			return SrcVariable;
		}
		return nullptr;
	}


	//~ FNotifyHook interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) 
	{
		NotifyDefaultValuePropertyChanged(PropertyChangedEvent);
	}

protected:
	UNiagaraComponent* Component; 
	FGuid Id;
	EDefaultValueType DefaultValueType;
	TSharedPtr<FStructOnScope> ParameterValue;
	FNiagaraScriptDataInterfaceInfo ObjectValue;
	TSharedPtr<SNiagaraParameterEditor> ParameterEditor; 
	TSharedPtr<IDetailsView> DetailsView;
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

	if (GEngine)
	{
		GEngine->OnWorldDestroyed().RemoveAll(this);
	}

	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);

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

			EffectInstance->OnDestroyed().RemoveAll(this);
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
	// We may have queued up as a result of an Undo of adding the effect itself. The objects we were 
	// referencing may therefore have been removed. If so, we'll have to take a different path later on in the code.
	int32 NumValid = 0;
	for (TWeakObjectPtr<UObject> WeakPtr : ObjectsCustomized)
	{
		if (!WeakPtr.IsStale(true) && WeakPtr.IsValid())
		{
			NumValid++;
		}
	}

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
		if (NumValid != 0)
		{
			bQueueForDetailsRefresh = true;
		}
		else
		{
			// If we no longer have any valid pointers, but previously had a builder, that means 
			// that the builder is probably dead or dying soon. We shouldn't trust it any more and
			// we should make sure that we aren't queueing for ticks to refresh either.
			Builder = nullptr;
			bQueueForDetailsRefresh = false;
		}

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

//~ FNotifyHook interface
void FNiagaraComponentDetails::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	for (TSharedPtr<FNiagaraParameterViewModelCustomDetails> Parameter : ViewModels)
	{
		if (Parameter->GetDefaultValueType() == INiagaraParameterViewModel::EDefaultValueType::Object)
		{
			UObject* ParameterObject = Parameter->GetDefaultValueObject();
			const UObject* ChangedObject = PropertyChangedEvent.GetObjectBeingEdited(0);
			const UObject* CurrentObject = ChangedObject;
			bool ParameterIsInObjectChain = false;
			while (ParameterIsInObjectChain == false && CurrentObject != nullptr)
			{
				if (ParameterObject == CurrentObject)
				{
					ParameterIsInObjectChain = true;
				}
				else
				{
					CurrentObject = CurrentObject->GetOuter();
				}
			}
			if (ParameterIsInObjectChain)
			{
				Parameter->NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
			}
		}
	}
}

void FNiagaraComponentDetails::OnPiEEnd()
{
	UE_LOG(LogNiagaraEditor, Log, TEXT("onPieEnd"));
	if (ObjectsCustomized.Num() == 1)
	{
		UNiagaraComponent* Component = Cast<UNiagaraComponent>(ObjectsCustomized[0].Get());

		if (Component)
		{
			if (Component->GetOutermost()->HasAnyPackageFlags(PKG_PlayInEditor))
			{
				UE_LOG(LogNiagaraEditor, Log, TEXT("onPieEnd - has package flags"));
				UWorld* TheWorld = UWorld::FindWorldInPackage(Component->GetOutermost());
				if (TheWorld)
				{
					OnWorldDestroyed(TheWorld);
				}
			}
		}
	}
}

void FNiagaraComponentDetails::OnWorldDestroyed(class UWorld* InWorld)
{
	// We have to clear out any temp data interfaces that were bound to the component's package when the world goes away or otherwise
	// we'll report GC leaks..
	if (ObjectsCustomized.Num() == 1)
	{
		UNiagaraComponent* Component = Cast<UNiagaraComponent>(ObjectsCustomized[0].Get());

		if (Component)
		{
			if (Component->GetWorld() == InWorld)
			{
				UE_LOG(LogNiagaraEditor, Log, TEXT("OnWorldDestroyed - matched up"));
				ViewModels.Empty();
				Builder = nullptr;
			}
		}

	}
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

	LocalDataInterfaceOverridesPropertyHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraComponent, EffectDataInterfaceLocalOverrides));
	if (LocalDataInterfaceOverridesPropertyHandle.IsValid())
	{
		LocalDataInterfaceOverridesPropertyHandle->MarkHiddenByCustomization();
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

		if (GEngine)
		{
			GEngine->OnWorldDestroyed().AddRaw(this, &FNiagaraComponentDetails::OnWorldDestroyed);
		}

		FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FNiagaraComponentDetails::OnPiEEnd);

		TSharedRef<FNiagaraEffectInstance> EffectInstance = Component->GetEffectInstance();
		ViewModels.Empty();

		UNiagaraEffect* Effect = Component->GetAsset();

		if (Effect == nullptr || Component->IsPendingKillOrUnreachable())
		{
			return;
		}
		
		// We'll want to monitor for any serious changes that might require a rebuild of the UI from scratch.
		UE_LOG(LogNiagaraEditor, Log, TEXT("Registering with effect instance %p"), &EffectInstance.Get());
		EffectInstance->OnReset().RemoveAll(this);
		EffectInstance->OnInitialized().RemoveAll(this);
		EffectInstance->OnDestroyed().RemoveAll(this);
		OnResetDelegateHandle = EffectInstance->OnReset().AddSP(this, &FNiagaraComponentDetails::OnEffectInstanceReset);
		OnInitDelegateHandle = EffectInstance->OnInitialized().AddSP(this, &FNiagaraComponentDetails::OnEffectInstanceReset);
		EffectInstance->OnDestroyed().AddSP(this, &FNiagaraComponentDetails::OnEffectInstanceDestroyed);

		TArray<TSharedRef<INiagaraParameterViewModel>> SortedViewModels;
		
		for (const FNiagaraVariable& Variable :Effect->GetEffectScript()->Parameters.Parameters)
		{
			TSharedPtr<FNiagaraParameterViewModelCustomDetails> ViewModel = MakeShareable(new FNiagaraParameterViewModelCustomDetails(Component, Variable, ENiagaraParameterEditMode::EditValueOnly));
			TSharedRef<INiagaraParameterViewModel> ViewModelRef = ViewModel.ToSharedRef();
			SortedViewModels.Add(ViewModelRef);
		}
		for (const FNiagaraScriptDataInterfaceInfo& Info : Effect->GetEffectScript()->DataInterfaceInfo)
		{
			TSharedPtr<FNiagaraParameterViewModelCustomDetails> ViewModel = MakeShareable(new FNiagaraParameterViewModelCustomDetails(Component, Info.Id, Info.Name, Info.DataInterface, ENiagaraParameterEditMode::EditValueOnly));
			TSharedRef<INiagaraParameterViewModel> ViewModelRef = ViewModel.ToSharedRef();
			SortedViewModels.Add(ViewModelRef);
		}
		
		INiagaraParameterCollectionViewModel::SortViewModels(SortedViewModels);

		// Go through each instance variable and add new GUI for it.
		for (int32 i = 0; i < SortedViewModels.Num(); i++)
		{
			TSharedPtr<INiagaraParameterViewModel> BaseViewModel = SortedViewModels[i];
			TSharedPtr<FNiagaraParameterViewModelCustomDetails> ViewModel = StaticCastSharedPtr<FNiagaraParameterViewModelCustomDetails>(BaseViewModel);

			UE_LOG(LogNiagaraEditor, Log, TEXT("Param '%s' [%d]"), *ViewModel->GetName().ToString(), ViewModel->GetSortOrder());

			bool bIsDataInterface = ViewModel->GetSourceDataInterface() != nullptr;
			bool bIsParameter = ViewModel->GetSourceVariable() != nullptr;
			if (bIsParameter)
			{
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
					FDetailWidgetRow& OverrideRow = NiagaraCategory.AddCustomRow(FText::FromName(ViewModel->GetName()));
					OverrideRow.NameContent()
						.MinDesiredWidth(0)
						.MaxDesiredWidth(0)
						[
							SNew(STextBlock)
							.Text(FText::FromName(ViewModel->GetName()))
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
			else if (bIsDataInterface)
			{
				FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

				FDetailsViewArgs Args(false, false, false, FDetailsViewArgs::HideNameArea, true, this);
				TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(Args);
				TSharedPtr<IDetailsView> SetDetailsView = DetailsView;
				ViewModel->SetDataInterfaceEditor(SetDetailsView);
				DetailsView->SetObject(ViewModel->GetDefaultValueObject());
				ViewModels.Add(ViewModel);

				TSharedPtr<SWidget> DetailsWidget;
				DetailsWidget = DetailsView;
				FDetailWidgetRow& OverrideRow = NiagaraCategory.AddCustomRow(FText::FromName(ViewModel->GetName()));
				OverrideRow.NameContent()
					.MinDesiredWidth(0)
					.MaxDesiredWidth(0)
					[
						SNew(STextBlock)
						.Text(FText::FromName(ViewModel->GetName()))
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
	UE_LOG(LogNiagaraEditor, Log, TEXT("OnEffectInstanceReset()"));
	bQueueForDetailsRefresh = true;
	for (TSharedPtr<FNiagaraParameterViewModelCustomDetails>& ViewModel : ViewModels)
	{
		ViewModel->Reset();
	}
}

void FNiagaraComponentDetails::OnEffectInstanceDestroyed()
{
	UE_LOG(LogNiagaraEditor, Log, TEXT("OnEffectInstanceDestroyed()"));
	bQueueForDetailsRefresh = true;
	for (TSharedPtr<FNiagaraParameterViewModelCustomDetails>& ViewModel : ViewModels)
	{
		ViewModel->Reset();
	}
	ViewModels.Empty();
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
