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

template<> TMap<UNiagaraEmitterProperties*, TArray<FNiagaraEmitterViewModel*>> TNiagaraViewModelManager<UNiagaraEmitterProperties, FNiagaraEmitterViewModel>::ObjectsToViewModels{};

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
	, LastEventScriptStatus(ENiagaraScriptCompileStatus::NCS_Unknown)
	, bEmitterDirty(false)
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

	if (Emitter && Emitter->EventHandlerScriptProps.Script && Emitter->EventHandlerScriptProps.Script->ByteCode.Num() != 0)
	{
		LastEventScriptStatus = ENiagaraScriptCompileStatus::NCS_UpToDate;
	}

	RegisteredHandle = RegisterViewModelWithMap(Emitter, this);
}


bool FNiagaraEmitterViewModel::Set(UNiagaraEmitterProperties* InEmitter, FNiagaraSimulation* InSimulation, ENiagaraParameterEditMode ParameterEditMode)
{
	SetEmitter(InEmitter);
	SetSimulation(InSimulation);
	return true;
}

FNiagaraEmitterViewModel::~FNiagaraEmitterViewModel()
{
	UnregisterViewModelWithMap(RegisteredHandle);
}


void FNiagaraEmitterViewModel::SetEmitter(UNiagaraEmitterProperties* InEmitter)
{
	UnregisterViewModelWithMap(RegisteredHandle);

	Emitter = InEmitter;

	RegisteredHandle = RegisterViewModelWithMap(Emitter, this);

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
	OnParameterValueChangedDelegate.Broadcast(FGuid());
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

bool FNiagaraEmitterViewModel::GetDirty() const
{
	bool bDirty = SpawnScriptViewModel->GetScriptDirty() || UpdateScriptViewModel->GetScriptDirty() || bEmitterDirty;
	return bDirty;
}

void FNiagaraEmitterViewModel::SetDirty(bool bDirty)
{
	SpawnScriptViewModel->SetScriptDirty(bDirty);
	UpdateScriptViewModel->SetScriptDirty(bDirty);
	bEmitterDirty = bDirty;
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
	if(Emitter)
	{
		TArray<ENiagaraScriptCompileStatus> CompileStatuses;
		TArray<FString> CompileErrors;
		TArray<FString> CompilePaths;
		Emitter->CompileScripts(CompileStatuses, CompileErrors, CompilePaths);
		if (CompileStatuses.Num() > (int32)EScriptCompileIndices::UpdateScript)
		{
			SpawnScriptViewModel->UpdateCompileStatus(CompileStatuses[(int32)EScriptCompileIndices::SpawnScript], CompileErrors[(int32)EScriptCompileIndices::SpawnScript]);
			UpdateScriptViewModel->UpdateCompileStatus(CompileStatuses[(int32)EScriptCompileIndices::UpdateScript], CompileErrors[(int32)EScriptCompileIndices::UpdateScript]);
		}

		if (CompileStatuses.Num() > (int32)EScriptCompileIndices::EventScript)
		{
			FString ErrorMsg;
			LastEventScriptStatus = CompileStatuses[(int32)EScriptCompileIndices::EventScript];
		}
	}
	OnScriptCompiled().Broadcast();
}

ENiagaraScriptCompileStatus FNiagaraEmitterViewModel::UnionCompileStatus(const ENiagaraScriptCompileStatus& StatusA, const ENiagaraScriptCompileStatus& StatusB)
{
	if (StatusA != StatusB)
	{
		if (StatusA == ENiagaraScriptCompileStatus::NCS_Unknown || StatusB == ENiagaraScriptCompileStatus::NCS_Unknown)
		{
			return ENiagaraScriptCompileStatus::NCS_Unknown;
		}
		else if (StatusA >= ENiagaraScriptCompileStatus::NCS_MAX || StatusB >= ENiagaraScriptCompileStatus::NCS_MAX)
		{
			return ENiagaraScriptCompileStatus::NCS_MAX;
		}
		else if (StatusA == ENiagaraScriptCompileStatus::NCS_Dirty || StatusB == ENiagaraScriptCompileStatus::NCS_Dirty)
		{
			return ENiagaraScriptCompileStatus::NCS_Dirty;
		}
		else if (StatusA == ENiagaraScriptCompileStatus::NCS_Error || StatusB == ENiagaraScriptCompileStatus::NCS_Error)
		{
			return ENiagaraScriptCompileStatus::NCS_Error;
		}
		else if (StatusA == ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings || StatusB == ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings)
		{
			return ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings;
		}
		else if (StatusA == ENiagaraScriptCompileStatus::NCS_BeingCreated || StatusB == ENiagaraScriptCompileStatus::NCS_BeingCreated)
		{
			return ENiagaraScriptCompileStatus::NCS_BeingCreated;
		}
		else if (StatusA == ENiagaraScriptCompileStatus::NCS_UpToDate || StatusB == ENiagaraScriptCompileStatus::NCS_UpToDate)
		{
			return ENiagaraScriptCompileStatus::NCS_UpToDate;
		}
		else
		{
			return ENiagaraScriptCompileStatus::NCS_Unknown;
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

FNiagaraEmitterViewModel::FOnScriptCompiled& FNiagaraEmitterViewModel::OnScriptCompiled()
{
	return OnScriptCompiledDelegate;
}

void FNiagaraEmitterViewModel::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged)
{
	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraEmitterProperties, bInterpolatedSpawning))
	{
		//Recompile spawn script if we've altered the interpolated spawn property.
		Emitter->SpawnScriptProps.Script->Usage = Emitter->bInterpolatedSpawning ? ENiagaraScriptUsage::SpawnScriptInterpolated : ENiagaraScriptUsage::SpawnScript;

		FString GraphLevelErrorMessages;
		ENiagaraScriptCompileStatus CompileStatus = Emitter->CompileScript(EScriptCompileIndices::SpawnScript, GraphLevelErrorMessages);
		SpawnScriptViewModel->UpdateCompileStatus(CompileStatus, GraphLevelErrorMessages);
	}

	bEmitterDirty = true;

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

void FNiagaraEmitterViewModel::ScriptParameterValueChanged(FGuid ParameterId)
{
	OnParameterValueChangedDelegate.Broadcast(ParameterId);
}

#undef LOCTEXT_NAMESPACE
