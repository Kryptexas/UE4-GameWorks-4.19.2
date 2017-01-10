// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraObjectSelection.h"
#include "NiagaraScriptSource.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScriptOutputCollectionViewModel.h"

#include "ScopedTransaction.h"
#include "AssetEditorManager.h"

#define LOCTEXT_NAMESPACE "EmitterHandleViewModel"

FNiagaraEmitterHandleViewModel::FNiagaraEmitterHandleViewModel(FNiagaraEmitterHandle* InEmitterHandle, FNiagaraSimulation* InSimulation, UNiagaraEffect& InOwningEffect, ENiagaraParameterEditMode InParameterEditMode)
	: EmitterHandle(InEmitterHandle)
	, OwningEffect(InOwningEffect)
	, ParameterEditMode(InParameterEditMode)
	, EmitterViewModel(MakeShareable(new FNiagaraEmitterViewModel((InEmitterHandle ? InEmitterHandle->GetInstance() : nullptr), InSimulation, ParameterEditMode)))
{
}

bool FNiagaraEmitterHandleViewModel::Set(FNiagaraEmitterHandle* InEmitterHandle, FNiagaraSimulation* InSimulation, UNiagaraEffect& InOwningEffect, ENiagaraParameterEditMode InParameterEditMode)
{
	if (&OwningEffect != &InOwningEffect)
	{
		return false;
	}

	SetEmitterHandle(InEmitterHandle);
	SetSimulation(InSimulation);
	
	ParameterEditMode = InParameterEditMode;
	UNiagaraEmitterProperties* EmitterProperties = nullptr;
	if (InEmitterHandle != nullptr)
	{
		EmitterProperties = InEmitterHandle->GetInstance();
	}
	return EmitterViewModel->Set(EmitterProperties, InSimulation, ParameterEditMode);
}


void FNiagaraEmitterHandleViewModel::SetEmitterHandle(FNiagaraEmitterHandle* InEmitterHandle)
{
	EmitterHandle = InEmitterHandle;
}

void FNiagaraEmitterHandleViewModel::SetSimulation(FNiagaraSimulation* InSimulation)
{
	EmitterViewModel->SetSimulation(InSimulation);
}

FGuid FNiagaraEmitterHandleViewModel::GetId() const
{
	if (EmitterHandle)
	{
		return EmitterHandle->GetId();
	}
	return FGuid();
}

FName FNiagaraEmitterHandleViewModel::GetName() const
{
	if (EmitterHandle)
	{
		return EmitterHandle->GetName();
	}
	return FName();
}

void FNiagaraEmitterHandleViewModel::SetName(FName InName)
{
	if (EmitterHandle && EmitterHandle->GetName() == InName)
	{
		return;
	}

	if (EmitterHandle)
	{
		TSet<FName> OtherEmitterNames;
		for (const FNiagaraEmitterHandle& OtherEmitterHandle : OwningEffect.GetEmitterHandles())
		{
			if (OtherEmitterHandle.GetId() != EmitterHandle->GetId())
			{
				OtherEmitterNames.Add(OtherEmitterHandle.GetName());
			}
		}
		FName UniqueName = FNiagaraEditorUtilities::GetUniqueName(InName, OtherEmitterNames);

		FScopedTransaction ScopedTransaction(NSLOCTEXT("NiagaraEmitterEditor", "EditEmitterNameTransaction", "Edit emitter name"));
		OwningEffect.Modify();
		EmitterHandle->SetName(UniqueName);
		OnPropertyChangedDelegate.Broadcast();
	}
}

FText FNiagaraEmitterHandleViewModel::GetNameText() const
{
	if (EmitterHandle)
	{
		return FText::FromName(EmitterHandle->GetName());
	}
	return FText();
}

void FNiagaraEmitterHandleViewModel::OnNameTextComitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	SetName(*InText.ToString());
}

bool FNiagaraEmitterHandleViewModel::GetIsEnabled() const
{
	if (EmitterHandle)
	{
		return EmitterHandle->GetIsEnabled();
	}
	return false;
}

void FNiagaraEmitterHandleViewModel::SetIsEnabled(bool bInIsEnabled)
{
	if (EmitterHandle && EmitterHandle->GetIsEnabled() != bInIsEnabled)
	{
		FScopedTransaction ScopedTransaction(NSLOCTEXT("NiagaraEmitterEditor", "EditEmitterEnabled", "Change emitter enabled state"));
		OwningEffect.Modify();
		EmitterHandle->SetIsEnabled(bInIsEnabled);
		OnPropertyChangedDelegate.Broadcast();
	}
}

ECheckBoxState FNiagaraEmitterHandleViewModel::GetIsEnabledCheckState() const
{
	if (EmitterHandle)
	{
		return EmitterHandle->GetIsEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void FNiagaraEmitterHandleViewModel::OnIsEnabledCheckStateChanged(ECheckBoxState InCheckState)
{
	SetIsEnabled(InCheckState == ECheckBoxState::Checked);
}

FNiagaraEmitterHandle* FNiagaraEmitterHandleViewModel::GetEmitterHandle()
{
	return EmitterHandle;
}

TSharedRef<FNiagaraEmitterViewModel> FNiagaraEmitterHandleViewModel::GetEmitterViewModel()
{
	return EmitterViewModel;
}

void FNiagaraEmitterHandleViewModel::CompileScripts()
{
	EmitterViewModel->CompileScripts();
}

void CopyParameterValues(UNiagaraScript* Script, UNiagaraScript* PreviousScript)
{
	for (FNiagaraVariable& InputParameter : Script->Parameters.Parameters)
	{
		for (FNiagaraVariable& PreviousInputParameter : PreviousScript->Parameters.Parameters)
		{
			if (PreviousInputParameter.IsDataAllocated())
			{
				if (InputParameter.GetId() == PreviousInputParameter.GetId() &&
					InputParameter.GetType() == PreviousInputParameter.GetType())
				{
					InputParameter.AllocateData();
					PreviousInputParameter.CopyTo(InputParameter.GetData());
				}
			}
		}
	}
}

void FNiagaraEmitterHandleViewModel::RefreshFromSource()
{
	FScopedTransaction ScopedTransaction(NSLOCTEXT("NiagaraEmitterEditor", "RefreshFromSource", "Reset emitter from source."));
	OwningEffect.Modify();

	if (EmitterHandle)
	{
		{
			// Pull in changes to the emitter asset by copying the source scripts, compiling and then copying over parameter values
			// where relevant.
			// TODO: Update this to support events.
			UNiagaraEmitterProperties* Instance = EmitterHandle->GetInstance();
			const UNiagaraEmitterProperties* Source = EmitterHandle->GetSource();
			UNiagaraScript* PreviousInstanceSpawnScript = Instance->SpawnScriptProps.Script;
			UNiagaraScript* PreviousInstanceUpdateScript = Instance->UpdateScriptProps.Script;

			Instance->SpawnScriptProps.Script = CastChecked<UNiagaraScript>(StaticDuplicateObject(Source->SpawnScriptProps.Script, Instance));
			Instance->UpdateScriptProps.Script = CastChecked<UNiagaraScript>(StaticDuplicateObject(Source->UpdateScriptProps.Script, Instance));

			FString ErrorMessages;
			CastChecked<UNiagaraScriptSource>(Instance->SpawnScriptProps.Script->Source)->Compile(ErrorMessages);
			CastChecked<UNiagaraScriptSource>(Instance->UpdateScriptProps.Script->Source)->Compile(ErrorMessages);

			CopyParameterValues(Instance->SpawnScriptProps.Script, PreviousInstanceSpawnScript);
			CopyParameterValues(Instance->UpdateScriptProps.Script, PreviousInstanceUpdateScript);
		}

		EmitterViewModel->SetEmitter(EmitterHandle->GetInstance());
	}
	OnPropertyChangedDelegate.Broadcast();
}

void FNiagaraEmitterHandleViewModel::ResetToSource()
{
	FScopedTransaction ScopedTransaction(NSLOCTEXT("NiagaraEmitterEditor", "ResetToSource", "Reset emitter to source."));
	OwningEffect.Modify();
	
	if (EmitterHandle)
	{
		{
			UNiagaraEmitterProperties* Instance = CastChecked<UNiagaraEmitterProperties>(StaticDuplicateObject(EmitterHandle->GetSource(), EmitterHandle->GetInstance()->GetOuter()));
			FString ErrorMsg;
			CastChecked<UNiagaraScriptSource>(Instance->SpawnScriptProps.Script->Source)->Compile(ErrorMsg);
			CastChecked<UNiagaraScriptSource>(Instance->UpdateScriptProps.Script->Source)->Compile(ErrorMsg);
			EmitterHandle->SetInstance(Instance);
		}

		EmitterViewModel->SetEmitter(EmitterHandle->GetInstance());
	}
	OnPropertyChangedDelegate.Broadcast();
}

void FNiagaraEmitterHandleViewModel::OpenSourceEmitter()
{
	if (EmitterHandle)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(const_cast<UNiagaraEmitterProperties*>(EmitterHandle->GetSource()));
	}
}

FNiagaraEmitterHandleViewModel::FOnPropertyChanged& FNiagaraEmitterHandleViewModel::OnPropertyChanged()
{
	return OnPropertyChangedDelegate;
}

#undef LOCTEXT_NAMESPACE