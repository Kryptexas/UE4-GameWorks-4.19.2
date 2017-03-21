// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "EditorUndoClient.h"
#include "TickableEditorObject.h"
#include "Reply.h"
#include "Visibility.h"
#include "Misc/NotifyHook.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class FNiagaraParameterViewModelCustomDetails;
class INiagaraParameterViewModel;
class SNiagaraParameterEditor;
class FNiagaraParameterViewModelCustomDetails;
class UNiagaraEffect;

class FNiagaraComponentDetails : public IDetailCustomization, public FEditorUndoClient, public FTickableEditorObject, public FNotifyHook
{
public:
	virtual ~FNiagaraComponentDetails();

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;


	//~ FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

	// FTickableEditorObject
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	//FNOtifyHook
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) override;

protected:
	void ParameterEditorBeginValueChange(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item);
	void ParameterEditorEndValueChange(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item);
	void ParameterEditorValueChanged(TSharedRef<SNiagaraParameterEditor> ParameterEditor, TSharedRef<FNiagaraParameterViewModelCustomDetails> Item);
	void OnEffectInstanceReset();
	void OnEffectInstanceDestroyed();
	FReply OnEffectOpenRequested(UNiagaraEffect* InEffect);
	FReply OnLocationResetClicked(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item);
	EVisibility GetLocationResetVisibility(TSharedRef<FNiagaraParameterViewModelCustomDetails> Item) const;
	void OnWorldDestroyed(class UWorld* InWorld);
	void OnPiEEnd();

	FNiagaraComponentDetails();
private:
	TSharedPtr<IPropertyHandle> LocalOverridesPropertyHandle;
	TSharedPtr<IPropertyHandle> LocalDataInterfaceOverridesPropertyHandle;
	TArray<TWeakObjectPtr<UObject>> ObjectsCustomized;
	TArray<TSharedPtr<FNiagaraParameterViewModelCustomDetails> > ViewModels;
	IDetailLayoutBuilder* Builder;
	bool bQueueForDetailsRefresh;

	FDelegateHandle OnInitDelegateHandle;
	FDelegateHandle OnResetDelegateHandle;
};
