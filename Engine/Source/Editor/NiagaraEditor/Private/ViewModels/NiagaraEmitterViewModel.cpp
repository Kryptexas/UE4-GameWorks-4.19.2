// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterViewModel.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraSimulation.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraObjectSelection.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraScriptOutputCollectionViewModel.h"
#include "NiagaraScriptSource.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "EmitterEditorViewModel"

const FText FNiagaraEmitterViewModel::StatsFormat = NSLOCTEXT("NiagaraEmitterViewModel", "StatsFormat", "{0} Particles | {1} ms | {2} MB");
const float Megabyte = 1024.0f * 1024.0f;

FNiagaraEmitterViewModel::FNiagaraEmitterViewModel(UNiagaraEmitterProperties* InEmitter, FNiagaraSimulation* InSimulation, ENiagaraParameterEditMode InParameterEditMode)
	: Emitter(InEmitter)
	, Simulation(InSimulation)
	, SpawnScriptViewModel(MakeShareable(new FNiagaraScriptViewModel((Emitter ? Emitter->SpawnScriptProps.Script : nullptr), LOCTEXT("SpawnDisplayName", "Spawn"), InParameterEditMode)))
	, UpdateScriptViewModel(MakeShareable(new FNiagaraScriptViewModel((Emitter ? Emitter->UpdateScriptProps.Script : nullptr), LOCTEXT("UpdateDisplayName", "Update"), InParameterEditMode)))
	, ParameterNameColumnWidth(.3f)
	, ParameterContentColumnWidth(.7f)
	, bUpdatingSelectionInternally(false)
	, LastEventScriptStatus(NCS_Unknown)
{
	SpawnScriptViewModel->GetGraphViewModel()->GetSelection()->OnSelectedObjectsChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptViewModelSelectedNodesChanged, UpdateScriptViewModel->GetGraphViewModel());

	SpawnScriptViewModel->GetOutputCollectionViewModel()->OnOutputParametersChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::SpawnScriptOutputParametersChanged);

	UpdateScriptViewModel->GetGraphViewModel()->GetSelection()->OnSelectedObjectsChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptViewModelSelectedNodesChanged, SpawnScriptViewModel->GetGraphViewModel());

	SpawnScriptViewModel->GetInputCollectionViewModel()->OnParameterValueChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptParameterValueChanged);

	SpawnScriptViewModel->GetOutputCollectionViewModel()->OnParameterValueChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptParameterValueChanged);

	UpdateScriptViewModel->GetInputCollectionViewModel()->OnParameterValueChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptParameterValueChanged);

	UpdateScriptViewModel->GetOutputCollectionViewModel()->OnParameterValueChanged().AddRaw(
		this, &FNiagaraEmitterViewModel::ScriptParameterValueChanged);
}


bool FNiagaraEmitterViewModel::Set(UNiagaraEmitterProperties* InEmitter, FNiagaraSimulation* InSimulation, ENiagaraParameterEditMode ParameterEditMode)
{
	SetEmitter(InEmitter);
	SetSimulation(InSimulation);
	return true;
}


void FNiagaraEmitterViewModel::SetEmitter(UNiagaraEmitterProperties* InEmitter)
{
	Emitter = InEmitter;
	UNiagaraScript* CurrentScript = nullptr;
	if (Emitter)
	{
		CurrentScript = Emitter->SpawnScriptProps.Script;
	}
	SpawnScriptViewModel->SetScript(CurrentScript);
	if (Emitter)
	{
		CurrentScript = Emitter->UpdateScriptProps.Script;
	}
	UpdateScriptViewModel->SetScript(CurrentScript);

	OnEmitterChanged().Broadcast();
	OnPropertyChangedDelegate.Broadcast();
	OnParameterValueChangedDelegate.Broadcast(nullptr);
}


void FNiagaraEmitterViewModel::SetSimulation(FNiagaraSimulation* InSimulation)
{
	Simulation = InSimulation;
}


float FNiagaraEmitterViewModel::GetStartTime() const
{
	if (Emitter)
	{
		return Emitter->StartTime;
	}
	
	return 0.0f;
}


void FNiagaraEmitterViewModel::SetStartTime(float InStartTime)
{
	if (Emitter && Emitter->StartTime != InStartTime)
	{
		Emitter->Modify();
		Emitter->StartTime = InStartTime;
		OnPropertyChanged().Broadcast();
	}
}


float FNiagaraEmitterViewModel::GetEndTime() const
{
	if (Emitter)
	{
		return Emitter->EndTime;
	}
	return 0.0f;
}


void FNiagaraEmitterViewModel::SetEndTime(float InEndTime)
{
	if (Emitter && Emitter->EndTime != InEndTime)
	{
		Emitter->Modify();
		Emitter->EndTime = InEndTime;
		OnPropertyChanged().Broadcast();
	}
}


int32 FNiagaraEmitterViewModel::GetNumLoops() const
{
	if (Emitter)
	{
		return Emitter->NumLoops;
	}
	return 1;
}


UNiagaraEmitterProperties* FNiagaraEmitterViewModel::GetEmitter()
{
	return Emitter;
}


FText FNiagaraEmitterViewModel::GetStatsText() const
{
	if (Simulation != nullptr)
	{
		return FText::Format(StatsFormat,
			FText::AsNumber(Simulation->GetNumParticles()),
			FText::AsNumber(Simulation->GetTotalCPUTime()),
			FText::AsNumber(Simulation->GetTotalBytesUsed() / Megabyte));
	}
	else
	{
		return LOCTEXT("InvalidSimulation", "Simulation is invalid.");
	}
}


TSharedRef<FNiagaraScriptViewModel> FNiagaraEmitterViewModel::GetSpawnScriptViewModel()
{
	return SpawnScriptViewModel;
}


TSharedRef<FNiagaraScriptViewModel> FNiagaraEmitterViewModel::GetUpdateScriptViewModel()
{
	return UpdateScriptViewModel;
}


float FNiagaraEmitterViewModel::GetParameterNameColumnWidth() const
{
	return ParameterNameColumnWidth;
}


float FNiagaraEmitterViewModel::GetParameterContentColumnWidth() const
{
	return ParameterContentColumnWidth;
}


void FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged(float Width)
{
	ParameterNameColumnWidth = Width;
}


void FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged(float Width)
{
	ParameterContentColumnWidth = Width;
}

bool FNiagaraEmitterViewModel::DoesAssetSavedInduceRefresh(const FString& PackagePath, UObject* PackageObject, bool RecurseIntoDependencies)
{
	if (Emitter != nullptr)
	{
		if (Cast<UObject>(Emitter->GetOutermost()) == PackageObject)
		{
			return true;
		}
	}

	if (RecurseIntoDependencies)
	{
		return SpawnScriptViewModel->DoesAssetSavedInduceRefresh(PackagePath, PackageObject, RecurseIntoDependencies) ||
			UpdateScriptViewModel->DoesAssetSavedInduceRefresh(PackagePath, PackageObject, RecurseIntoDependencies);
	}
	return false;
}

void FNiagaraEmitterViewModel::CompileScripts()
{
	SpawnScriptViewModel->Compile();
	UpdateScriptViewModel->Compile();
	if(Emitter)
	{
		Emitter->UpdateScriptProps.InitDataSetAccess();
		if (Emitter->EventHandlerScriptProps.Script)
		{
			FString ErrorMsg;
			LastEventScriptStatus = Cast<UNiagaraScriptSource>(Emitter->EventHandlerScriptProps.Script->Source)->Compile(ErrorMsg);
		}
	}
	OnParameterValueChanged().Broadcast(nullptr);
}

ENiagaraScriptCompileStatus FNiagaraEmitterViewModel::UnionCompileStatus(const ENiagaraScriptCompileStatus& StatusA, const ENiagaraScriptCompileStatus& StatusB)
{
	if (StatusA != StatusB)
	{
		if (StatusA == NCS_Unknown || StatusB == NCS_Unknown)
		{
			return NCS_Unknown;
		}
		else if (StatusA >= NCS_MAX || StatusB >= NCS_MAX)
		{
			return NCS_MAX;
		}
		else if (StatusA == NCS_Dirty || StatusB == NCS_Dirty)
		{
			return NCS_Dirty;
		}
		else if (StatusA == NCS_Error || StatusB == NCS_Error)
		{
			return NCS_Error;
		}
		else if (StatusA == NCS_UpToDateWithWarnings || StatusB == NCS_UpToDateWithWarnings)
		{
			return NCS_UpToDateWithWarnings;
		}
		else if (StatusA == NCS_BeingCreated || StatusB == NCS_BeingCreated)
		{
			return NCS_BeingCreated;
		}
		else if (StatusA == NCS_UpToDate || StatusB == NCS_UpToDate)
		{
			return NCS_UpToDate;
		}
		else
		{
			return NCS_Unknown;
		}
	}
	else
	{
		return StatusA;
	}
}


ENiagaraScriptCompileStatus FNiagaraEmitterViewModel::GetLatestCompileStatus()
{
	ENiagaraScriptCompileStatus SpawnStatus = SpawnScriptViewModel->GetLatestCompileStatus();
	ENiagaraScriptCompileStatus UpdateStatus = UpdateScriptViewModel->GetLatestCompileStatus();
	ENiagaraScriptCompileStatus UnionStatus = UnionCompileStatus(SpawnStatus, UpdateStatus);	

	if (Emitter && Emitter->EventHandlerScriptProps.Script)
	{
		UnionStatus = UnionCompileStatus(UnionStatus, LastEventScriptStatus);
	}
	return UnionStatus;
}


FNiagaraEmitterViewModel::FOnEmitterChanged& FNiagaraEmitterViewModel::OnEmitterChanged()
{
	return OnEmitterChangedDelegate;
}

FNiagaraEmitterViewModel::FOnPropertyChanged& FNiagaraEmitterViewModel::OnPropertyChanged()
{
	return OnPropertyChangedDelegate;
}

FNiagaraEmitterViewModel::FOnParameterValueChanged& FNiagaraEmitterViewModel::OnParameterValueChanged()
{
	return OnParameterValueChangedDelegate;
}

void FNiagaraEmitterViewModel::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	OnPropertyChangedDelegate.Broadcast();
}

void FNiagaraEmitterViewModel::ScriptViewModelSelectedNodesChanged(TSharedRef<FNiagaraScriptGraphViewModel> ClearScriptEditorViewModel)
{
	if (bUpdatingSelectionInternally == false)
	{
		bUpdatingSelectionInternally = true;
		{
			ClearScriptEditorViewModel->GetSelection()->ClearSelectedObjects();
		}
		bUpdatingSelectionInternally = false;
	}
}

void FNiagaraEmitterViewModel::SpawnScriptOutputParametersChanged()
{
	UNiagaraNodeOutput* SpawnOutputNode = SpawnScriptViewModel->GetOutputCollectionViewModel()->GetOutputNode();
	UNiagaraNodeOutput* UpdateOutputNode = UpdateScriptViewModel->GetOutputCollectionViewModel()->GetOutputNode();

	if (SpawnOutputNode != nullptr && UpdateOutputNode != nullptr)
	{
		UpdateOutputNode->Modify();
		UpdateOutputNode->Outputs = SpawnOutputNode->Outputs;
		UpdateOutputNode->NotifyOutputVariablesChanged();
	}
}

void FNiagaraEmitterViewModel::ScriptParameterValueChanged(const FNiagaraVariable* ChangedVariable)
{
	if (ChangedVariable == nullptr || ChangedVariable->GetType().GetClass() != nullptr)
	{
		CompileScripts();
	}
	else
	{
		OnParameterValueChangedDelegate.Broadcast(ChangedVariable);
	}
}

#undef LOCTEXT_NAMESPACE
