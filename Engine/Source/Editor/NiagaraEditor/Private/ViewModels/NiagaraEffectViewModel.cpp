// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectViewModel.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraScriptSource.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraEffectScriptViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraSequence.h"
#include "MovieSceneNiagaraEmitterTrack.h"
#include "MovieSceneNiagaraEmitterSection.h"
#include "NiagaraEffectInstance.h"

#include "Editor.h"

#include "ScopedTransaction.h"
#include "MovieScene.h"
#include "ISequencerModule.h"
#include "EditorSupportDelegates.h"
#include "ModuleManager.h"
#include "TNiagaraViewModelManager.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectViewModel"

template<> TMap<UNiagaraEffect*, TArray<FNiagaraEffectViewModel*>> TNiagaraViewModelManager<UNiagaraEffect, FNiagaraEffectViewModel>::ObjectsToViewModels{};

FNiagaraEffectViewModel::FNiagaraEffectViewModel(UNiagaraEffect& InEffect, FNiagaraEffectViewModelOptions InOptions)
	: Effect(InEffect)
	, ParameterEditMode(InOptions.ParameterEditMode)
	, PreviewComponent(nullptr)
	, EffectScriptViewModel(MakeShareable(new FNiagaraEffectScriptViewModel(Effect)))
	, NiagaraSequence(nullptr)
	, bCanRemoveEmittersFromTimeline(InOptions.bCanRemoveEmittersFromTimeline)
	, bCanRenameEmittersFromTimeline(InOptions.bCanRenameEmittersFromTimeline)
	, bUpdatingFromSequencerDataChange(false)
{
	EffectScriptViewModel->GetInputCollectionViewModel()->OnParameterValueChanged().AddRaw(this, &FNiagaraEffectViewModel::EffectParameterValueChanged);
	EffectScriptViewModel->GetInputCollectionViewModel()->OnCollectionChanged().AddRaw(this, &FNiagaraEffectViewModel::EffectParameterCollectionChanged);
	EffectScriptViewModel->OnParameterBindingsChanged().AddRaw(this, &FNiagaraEffectViewModel::EffectParameterBindingsChanged);
	EffectScriptViewModel->GetInputCollectionViewModel()->GetSelection().OnSelectedObjectsChanged().AddRaw(this, &FNiagaraEffectViewModel::ParameterSelectionChanged);
	SetupPreviewComponentAndInstance();
	SetupSequencer();
	RefreshAll();
	GEditor->RegisterForUndo(this);
	RegisteredHandle = RegisterViewModelWithMap(&InEffect, this);
}

FNiagaraEffectViewModel::~FNiagaraEffectViewModel()
{
	CurveOwner.EmptyCurves();
	if (EffectInstance.IsValid())
	{
		EffectInstance->OnInitialized().RemoveAll(this);
	}

	GEditor->UnregisterForUndo(this);

	// Make sure that we clear out all of our event handlers
	UnregisterViewModelWithMap(RegisteredHandle);
	UE_LOG(LogNiagaraEditor, Warning, TEXT("Deleting effect view model %p"), this);

	EffectScriptViewModel->GetInputCollectionViewModel()->OnParameterValueChanged().RemoveAll(this);
	EffectScriptViewModel->GetInputCollectionViewModel()->OnCollectionChanged().RemoveAll(this);
	EffectScriptViewModel->OnParameterBindingsChanged().RemoveAll(this);
	
	for (TSharedRef<FNiagaraEmitterHandleViewModel>& HandleRef : EmitterHandleViewModels)
	{
		HandleRef->OnPropertyChanged().RemoveAll(this);
		HandleRef->GetEmitterViewModel()->OnPropertyChanged().RemoveAll(this);
		HandleRef->GetEmitterViewModel()->OnParameterValueChanged().RemoveAll(this);
		HandleRef->GetEmitterViewModel()->OnScriptCompiled().RemoveAll(this);
		HandleRef->GetEmitterViewModel()->GetSpawnScriptViewModel()->GetInputCollectionViewModel()->
			GetSelection().OnSelectedObjectsChanged().RemoveAll(this);
		HandleRef->GetEmitterViewModel()->GetUpdateScriptViewModel()->GetInputCollectionViewModel()->
			GetSelection().OnSelectedObjectsChanged().RemoveAll(this);
	}
	EmitterHandleViewModels.Empty();
	EffectInstance.Reset();
}

const TArray<TSharedRef<FNiagaraEmitterHandleViewModel>>& FNiagaraEffectViewModel::GetEmitterHandleViewModels()
{
	return EmitterHandleViewModels;
}

TSharedRef<FNiagaraEffectScriptViewModel> FNiagaraEffectViewModel::GetEffectScriptViewModel()
{
	return EffectScriptViewModel;
}

UNiagaraComponent* FNiagaraEffectViewModel::GetPreviewComponent()
{
	return PreviewComponent;
}

TSharedPtr<ISequencer> FNiagaraEffectViewModel::GetSequencer()
{
	return Sequencer;
}

FNiagaraCurveOwner& FNiagaraEffectViewModel::GetCurveOwner()
{
	return CurveOwner;
}

void FNiagaraEffectViewModel::AddEmitterFromAssetData(const FAssetData& AssetData)
{
	UNiagaraEmitterProperties* Emitter = Cast<UNiagaraEmitterProperties>(AssetData.GetAsset());
	if (Emitter != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddEmitter", "Add emitter"));

		TSet<FName> EmitterHandleNames;
		for (const FNiagaraEmitterHandle& EmitterHandle : Effect.GetEmitterHandles())
		{
			EmitterHandleNames.Add(EmitterHandle.GetName());
		}

		Effect.Modify();
		FNiagaraEmitterHandle EmitterHandle = Effect.AddEmitterHandle(*Emitter, FNiagaraEditorUtilities::GetUniqueName(Emitter->GetFName(), EmitterHandleNames));

		if (Effect.GetNumEmitters() == 1)
		{
			// When adding a new emitter to an empty effect reset the playback and start playing.
			Sequencer->SetGlobalTime(0);
			Sequencer->SetPlaybackStatus(EMovieScenePlayerStatus::Playing);
		}
		
		RefreshAll();
	}
}

void FNiagaraEffectViewModel::DuplicateEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDuplicate)
{
	if (EmitterHandleToDuplicate->GetEmitterHandle() == nullptr)
	{
		return;
	}

	const FScopedTransaction DuplicateTransaction(LOCTEXT("DuplicateEmitter", "Duplicate emitter"));

	TSet<FName> EmitterHandleNames;
	for (const FNiagaraEmitterHandle& EmitterHandle : Effect.GetEmitterHandles())
	{
		EmitterHandleNames.Add(EmitterHandle.GetName());
	}

	Effect.Modify();

	FNiagaraEmitterHandle EmitterHandle = Effect.DuplicateEmitterHandle(*EmitterHandleToDuplicate->GetEmitterHandle(), FNiagaraEditorUtilities::GetUniqueName(EmitterHandleToDuplicate->GetEmitterHandle()->GetName(), EmitterHandleNames));
	
	// When adding a new emitter reset the playback and start playing.
	Sequencer->SetGlobalTime(0);
	Sequencer->SetPlaybackStatus(EMovieScenePlayerStatus::Playing);

	RefreshAll();
}

void FNiagaraEffectViewModel::DeleteEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDelete)
{
	const FScopedTransaction DeleteTransaction(LOCTEXT("DeleteEmitter", "Delete emitter"));

	Effect.Modify();
	if (EmitterHandleToDelete->GetEmitterHandle())
	{
		Effect.RemoveEmitterHandle(*EmitterHandleToDelete->GetEmitterHandle());
	}

	RefreshAll();
}

FNiagaraEffectViewModel::FOnEmitterHandleViewModelsChanged& FNiagaraEffectViewModel::OnEmitterHandleViewModelsChanged()
{
	return OnEmitterHandleViewModelsChangedDelegate;
}

FNiagaraEffectViewModel::FOnCurveOwnerChanged& FNiagaraEffectViewModel::OnCurveOwnerChanged()
{
	return OnCurveOwnerChangedDelegate;
}

void FNiagaraEffectViewModel::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (PreviewComponent != nullptr)
	{
		Collector.AddReferencedObject(PreviewComponent);
	}
	if (NiagaraSequence != nullptr)
	{
		Collector.AddReferencedObject(NiagaraSequence);
	}
}

void FNiagaraEffectViewModel::PostUndo(bool bSuccess)
{
	RefreshAll();
}

void FNiagaraEffectViewModel::SetupPreviewComponentAndInstance()
{
	PreviewComponent = NewObject<UNiagaraComponent>(GetTransientPackage(), NAME_None, RF_Transient);
	PreviewComponent->SetAsset(&Effect);
	EffectInstance = PreviewComponent->GetEffectInstance();
	EffectInstance->SetAgeUpdateMode(FNiagaraEffectInstance::EAgeUpdateMode::DesiredAge);
	EffectInstance->OnInitialized().AddRaw(this, &FNiagaraEffectViewModel::EffectInstanceInitialized);
}

void FNiagaraEffectViewModel::RefreshAll()
{
	ReinitEffectInstances();
	RefreshEmitterHandleViewModels();
	RefreshSequencerTracks();
	EffectScriptViewModel->RefreshEmitterNodes();
}

void FNiagaraEffectViewModel::ReinitEffectInstances()
{
	// Make sure the emitters are compiled and then setup the effect instance.
	for (int32 i = 0; i < Effect.GetNumEmitters(); ++i)
	{
		FNiagaraEmitterHandle& EmitterHandle = Effect.GetEmitterHandle(i);
		FString ErrorMessages;
		//Cast<UNiagaraScriptSource>(EmitterHandle.GetInstance()->SpawnScriptProps.Script->Source)->Compile(ErrorMessages);
		//Cast<UNiagaraScriptSource>(EmitterHandle.GetInstance()->UpdateScriptProps.Script->Source)->Compile(ErrorMessages);
		EmitterHandle.GetInstance()->UpdateScriptProps.InitDataSetAccess();
	}

	for (TObjectIterator<UNiagaraComponent> ComponentIt; ComponentIt; ++ComponentIt)
	{
		UNiagaraComponent* Component = *ComponentIt;
		if (Component->GetAsset() == &Effect)
		{
			Component->GetEffectInstance()->Init(Component->GetAsset());
		}
	}
}

void FNiagaraEffectViewModel::RefreshEmitterHandleViewModels()
{
	TArray<TSharedRef<FNiagaraEmitterHandleViewModel>> OldViewModels = EmitterHandleViewModels;
	EmitterHandleViewModels.Empty();

	// Map existing view models to the real instances that now exist. Reuse if we can. Create a new one if we cannot.
	int32 i;
	for (i = 0; i < Effect.GetNumEmitters(); ++i)
	{
		FNiagaraEmitterHandle* EmitterHandle = &Effect.GetEmitterHandle(i);
		FNiagaraSimulation* Simulation = EffectInstance->GetSimulationForHandle(*EmitterHandle);

		bool bAdd = OldViewModels.Num() <= i;
		if (bAdd)
		{
			TSharedRef<FNiagaraEmitterHandleViewModel> ViewModel = MakeShareable(new FNiagaraEmitterHandleViewModel(EmitterHandle, Simulation, Effect, ParameterEditMode));
			// Since we're adding fresh, we need to register all the event handlers.
			ViewModel->OnPropertyChanged().AddRaw(this, &FNiagaraEffectViewModel::EmitterHandlePropertyChanged, ViewModel);
			ViewModel->GetEmitterViewModel()->OnPropertyChanged().AddRaw(this, &FNiagaraEffectViewModel::EmitterPropertyChanged, ViewModel);
			ViewModel->GetEmitterViewModel()->OnParameterValueChanged().AddRaw(this, &FNiagaraEffectViewModel::EmitterParameterValueChanged);
			ViewModel->GetEmitterViewModel()->OnScriptCompiled().AddRaw(this, &FNiagaraEffectViewModel::ScriptCompiled);
			ViewModel->GetEmitterViewModel()->GetSpawnScriptViewModel()->GetInputCollectionViewModel()->
				GetSelection().OnSelectedObjectsChanged().AddRaw(this, &FNiagaraEffectViewModel::ParameterSelectionChanged);
			ViewModel->GetEmitterViewModel()->GetUpdateScriptViewModel()->GetInputCollectionViewModel()->
				GetSelection().OnSelectedObjectsChanged().AddRaw(this, &FNiagaraEffectViewModel::ParameterSelectionChanged);
			EmitterHandleViewModels.Add(ViewModel);
		}
		else
		{
			TSharedRef<FNiagaraEmitterHandleViewModel> ViewModel = OldViewModels[i];
			ViewModel->Set(EmitterHandle, Simulation, Effect, ParameterEditMode);
			EmitterHandleViewModels.Add(ViewModel);
		}

	}

	check(EmitterHandleViewModels.Num() == Effect.GetNumEmitters());

	// Clear out any old view models that may still be left around.
	for (; i < OldViewModels.Num(); i++)
	{
		TSharedRef<FNiagaraEmitterHandleViewModel> ViewModel = OldViewModels[i];
		ViewModel->OnPropertyChanged().RemoveAll(this);
		ViewModel->GetEmitterViewModel()->OnPropertyChanged().RemoveAll(this);
		ViewModel->GetEmitterViewModel()->OnParameterValueChanged().RemoveAll(this);
		ViewModel->GetEmitterViewModel()->OnScriptCompiled().RemoveAll(this);
		ViewModel->GetEmitterViewModel()->GetSpawnScriptViewModel()->GetInputCollectionViewModel()->
			GetSelection().OnSelectedObjectsChanged().RemoveAll(this);
		ViewModel->GetEmitterViewModel()->GetUpdateScriptViewModel()->GetInputCollectionViewModel()->
			GetSelection().OnSelectedObjectsChanged().RemoveAll(this);
		ViewModel->Set(nullptr, nullptr, Effect, ParameterEditMode);
	}

	OnEmitterHandleViewModelsChangedDelegate.Broadcast();
}

float SequencerTimeRangePadding = 0.1f;
float SequencerEmitterEndPadding = 5.0f;

void FNiagaraEffectViewModel::RefreshSequencerTracks()
{
	TArray<UMovieSceneTrack*> MasterTracks = NiagaraSequence->GetMovieScene()->GetMasterTracks();
	for (UMovieSceneTrack* MasterTrack : MasterTracks)
	{
		if (MasterTrack != nullptr)
		{
			NiagaraSequence->GetMovieScene()->RemoveMasterTrack(*MasterTrack);
		}
	}

	float MaxEmitterTime = 0;
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EmitterHandleViewModels)
	{
		UMovieSceneNiagaraEmitterTrack* EmitterTrack = Cast<UMovieSceneNiagaraEmitterTrack>(NiagaraSequence->GetMovieScene()->AddMasterTrack(UMovieSceneNiagaraEmitterTrack::StaticClass()));
		EmitterTrack->SetEmitterHandle(EmitterHandleViewModel);
		RefreshSequencerTrack(EmitterTrack);
		MaxEmitterTime = FMath::Max(MaxEmitterTime, EmitterHandleViewModel->GetEmitterViewModel()->GetEndTime());
	}

	TRange<float> CurrentPlaybackRange = NiagaraSequence->MovieScene->GetPlaybackRange();
	TRange<float> CurrentWorkingRange = NiagaraSequence->MovieScene->GetEditorData().WorkingRange;

	NiagaraSequence->MovieScene->SetPlaybackRange(0, FMath::Max(MaxEmitterTime + SequencerEmitterEndPadding, CurrentPlaybackRange.GetUpperBoundValue()));
	NiagaraSequence->MovieScene->GetEditorData().WorkingRange = TRange<float>(
		FMath::Min(-SequencerTimeRangePadding, CurrentWorkingRange.GetLowerBoundValue()),
		FMath::Max(NiagaraSequence->MovieScene->GetPlaybackRange().GetUpperBoundValue() + SequencerTimeRangePadding, CurrentWorkingRange.GetUpperBoundValue()));

	Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
}

UMovieSceneNiagaraEmitterTrack* FNiagaraEffectViewModel::GetTrackForHandleViewModel(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel)
{
	for (UMovieSceneTrack* Track : NiagaraSequence->GetMovieScene()->GetMasterTracks())
	{
		UMovieSceneNiagaraEmitterTrack* EmitterTrack = CastChecked<UMovieSceneNiagaraEmitterTrack>(Track);
		if (EmitterTrack->GetEmitterHandle() == EmitterHandleViewModel)
		{
			return EmitterTrack;
		}
	}
	return nullptr;
}

void FNiagaraEffectViewModel::RefreshSequencerTrack(UMovieSceneNiagaraEmitterTrack* EmitterTrack)
{
	if(EmitterTrack != nullptr)
	{
		TSharedPtr<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel = EmitterTrack->GetEmitterHandle();
		UNiagaraEmitterProperties* Emitter = EmitterHandleViewModel->GetEmitterViewModel()->GetEmitter();

		TArray<UMovieSceneSection*> Sections = EmitterTrack->GetAllSections();
		UMovieSceneNiagaraEmitterSection* EmitterSection = Cast<UMovieSceneNiagaraEmitterSection>(Sections.Num() == 1 ? Sections[0] : nullptr);

		if (EmitterSection != nullptr && Emitter != nullptr)
		{
			EmitterTrack->SetDisplayName(EmitterHandleViewModel->GetNameText());

			bool bIsInfinite = EmitterHandleViewModel->GetEmitterViewModel()->GetStartTime() == 0
				&& EmitterHandleViewModel->GetEmitterViewModel()->GetEndTime() == 0;
			float StartTime;
			float EndTime;

			if (bIsInfinite && Emitter->Bursts.Num() > 0)
			{
				StartTime = TNumericLimits<float>::Max();
				EndTime = TNumericLimits<float>::Lowest();
				for (const FNiagaraEmitterBurst& Burst : Emitter->Bursts)
				{
					StartTime = FMath::Min(StartTime, Burst.Time);
					EndTime = FMath::Max(EndTime, Burst.Time);
				}
			}
			else
			{
				StartTime = EmitterHandleViewModel->GetEmitterViewModel()->GetStartTime();
				EndTime = EmitterHandleViewModel->GetEmitterViewModel()->GetEndTime();
			}

			EmitterSection->SetEmitterHandle(EmitterHandleViewModel.ToSharedRef());
			EmitterSection->SetIsActive(EmitterHandleViewModel->GetIsEnabled());
			EmitterSection->SetStartTime(StartTime);
			EmitterSection->SetEndTime(EndTime);
			EmitterSection->SetIsInfinite(bIsInfinite);

			TSharedPtr<FBurstCurve> BurstCurve = EmitterSection->GetBurstCurve();
			if (BurstCurve.IsValid())
			{
				BurstCurve->Reset();
				for (const FNiagaraEmitterBurst& Burst : Emitter->Bursts)
				{
					FMovieSceneBurstKey BurstKey;
					BurstKey.TimeRange = Burst.TimeRange;
					BurstKey.SpawnMinimum = Burst.SpawnMinimum;
					BurstKey.SpawnMaximum = Burst.SpawnMaximum;
					BurstCurve->AddKeyValue(Burst.Time, BurstKey);
				}
			}
		}
	}
}

TRange<float> SequencerDefaultTimeRange(0, 10);

void FNiagaraEffectViewModel::SetupSequencer()
{
	NiagaraSequence = NewObject<UNiagaraSequence>(GetTransientPackage());
	NiagaraSequence->MovieScene = NewObject<UMovieScene>(NiagaraSequence, FName("Niagara Effect MovieScene"), RF_Transactional);
	NiagaraSequence->MovieScene->SetPlaybackRange(SequencerDefaultTimeRange.GetLowerBoundValue(), SequencerDefaultTimeRange.GetUpperBoundValue());
	NiagaraSequence->MovieScene->GetEditorData().ViewRange =TRange<float>(
		SequencerDefaultTimeRange.GetLowerBoundValue() - SequencerTimeRangePadding,
		SequencerDefaultTimeRange.GetUpperBoundValue() + SequencerTimeRangePadding);
	NiagaraSequence->MovieScene->GetEditorData().WorkingRange = NiagaraSequence->MovieScene->GetEditorData().ViewRange;

	FSequencerViewParams ViewParams(TEXT("NiagaraSequencerSettings"));
	{
		ViewParams.InitialScrubPosition = 0;
		ViewParams.UniqueName = "NiagaraSequenceEditor";
	}

	FSequencerInitParams SequencerInitParams;
	{
		SequencerInitParams.ViewParams = ViewParams;
		SequencerInitParams.RootSequence = NiagaraSequence;
		SequencerInitParams.bEditWithinLevelEditor = false;
		SequencerInitParams.ToolkitHost = nullptr;
	}

	ISequencerModule &SequencerModule = FModuleManager::LoadModuleChecked< ISequencerModule >("Sequencer");
	Sequencer = SequencerModule.CreateSequencer(SequencerInitParams);

	Sequencer->OnMovieSceneDataChanged().AddRaw(this, &FNiagaraEffectViewModel::SequencerDataChanged);
	Sequencer->OnGlobalTimeChanged().AddRaw(this, &FNiagaraEffectViewModel::SequencerTimeChanged);
	Sequencer->SetPlaybackStatus(Effect.GetNumEmitters() > 0 
		? EMovieScenePlayerStatus::Playing
		: EMovieScenePlayerStatus::Stopped);
}

void FNiagaraEffectViewModel::ResetEffect()
{
	EffectInstance->Reset(FNiagaraEffectInstance::EResetMode::DeferredReset);
	if (Sequencer->GetPlaybackStatus() == EMovieScenePlayerStatus::Playing)
	{
		Sequencer->SetGlobalTime(0.0f);
	}

	for (TObjectIterator<UNiagaraComponent> ComponentIt; ComponentIt; ++ComponentIt)
	{
		UNiagaraComponent* Component = *ComponentIt;
		if (Component->GetAsset() == &Effect)
		{
			Component->SynchronizeWithSourceEffect();
			Component->GetEffectInstance()->Reset(FNiagaraEffectInstance::EResetMode::DeferredReset);
		}
	}
	SynchronizeEffectDrivenParametersUI();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}


void FNiagaraEffectViewModel::ReInitializeEffect()
{
	if (EffectInstance.IsValid())
	{
		EffectInstance->Reset(FNiagaraEffectInstance::EResetMode::DeferredReInit);
	}

	if (Sequencer.IsValid() && Sequencer->GetPlaybackStatus() == EMovieScenePlayerStatus::Playing)
	{
		Sequencer->SetGlobalTime(0.0f);
	}

	for (TObjectIterator<UNiagaraComponent> ComponentIt; ComponentIt; ++ComponentIt)
	{
		UNiagaraComponent* Component = *ComponentIt;
		if (Component->GetAsset() == &Effect)
		{
			Component->SynchronizeWithSourceEffect();
			Component->GetEffectInstance()->Reset(FNiagaraEffectInstance::EResetMode::DeferredReInit);
		}
	}
	SynchronizeEffectDrivenParametersUI();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

struct FParameterCurveData
{
	FRichCurve* Curve;
	FName Name;
	FLinearColor Color;
	UObject* Owner;
	FGuid ParameterId;
	TSharedPtr<INiagaraParameterCollectionViewModel> ParameterCollection;
};

void GetParameterCurveData(FName EmitterName, FString ScriptName, TSharedPtr<INiagaraParameterCollectionViewModel> ParameterCollection,	TArray<FParameterCurveData>& OutParameterCurveData)
{
	for (TSharedRef<INiagaraParameterViewModel> SelectedParameterViewModel : ParameterCollection->GetSelection().GetSelectedObjects())
	{
		UNiagaraDataInterfaceCurveBase* CurveDataInterface = Cast<UNiagaraDataInterfaceCurveBase>(SelectedParameterViewModel->GetDefaultValueObject());
		if (CurveDataInterface != nullptr)
		{
			TArray<UNiagaraDataInterfaceCurveBase::FCurveData> CurveData;
			CurveDataInterface->GetCurveData(CurveData);
			for (UNiagaraDataInterfaceCurveBase::FCurveData CurveDataItem : CurveData)
			{
				FParameterCurveData ParameterCurveData;
				ParameterCurveData.Curve = CurveDataItem.Curve;
				ParameterCurveData.Color = CurveDataItem.Color;
				ParameterCurveData.Owner = CurveDataInterface;
				FString EmitterPrefix = EmitterName == NAME_None
					? FString()
					: EmitterName.ToString() + ".";
				FString ScriptPrefix = ScriptName + ".";
				FString ParameterName = CurveDataItem.Name == NAME_None
					? SelectedParameterViewModel->GetName().ToString()
					: SelectedParameterViewModel->GetName().ToString() + ".";
				FString DataName = CurveDataItem.Name == NAME_None
					? FString()
					: CurveDataItem.Name.ToString();
				ParameterCurveData.Name = *(EmitterPrefix + ScriptPrefix + ParameterName + DataName);
				ParameterCurveData.ParameterId = SelectedParameterViewModel->GetId();
				ParameterCurveData.ParameterCollection = ParameterCollection;
				OutParameterCurveData.Add(ParameterCurveData);
			}
		}
	}
}

void FNiagaraEffectViewModel::ResetCurveData()
{
	CurveOwner.EmptyCurves();

	TArray<FParameterCurveData> ParameterCurveData;

	GetParameterCurveData(
		NAME_None,
		TEXT("Effect"),
		EffectScriptViewModel->GetInputCollectionViewModel(),
		ParameterCurveData);

	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EmitterHandleViewModels)
	{
		GetParameterCurveData(
			EmitterHandleViewModel->GetName(),
			TEXT("Spawn"),
			EmitterHandleViewModel->GetEmitterViewModel()->GetSpawnScriptViewModel()->GetInputCollectionViewModel(),
			ParameterCurveData);

		GetParameterCurveData(
			EmitterHandleViewModel->GetName(),
			TEXT("Update"),
			EmitterHandleViewModel->GetEmitterViewModel()->GetUpdateScriptViewModel()->GetInputCollectionViewModel(),
			ParameterCurveData);
	}

	for (FParameterCurveData& ParameterCurveDataItem : ParameterCurveData)
	{
		CurveOwner.AddCurve(
			*ParameterCurveDataItem.Curve,
			ParameterCurveDataItem.Name,
			ParameterCurveDataItem.Color,
			*ParameterCurveDataItem.Owner,
			FNiagaraCurveOwner::FNotifyCurveChanged::CreateRaw(this, &FNiagaraEffectViewModel::CurveChanged, ParameterCurveDataItem.ParameterCollection, ParameterCurveDataItem.ParameterId));
	}

	OnCurveOwnerChangedDelegate.Broadcast();
}

void FNiagaraEffectViewModel::EmitterHandlePropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel)
{
	// When the emitter handle changes, refresh the effect scripts emitter nodes just in cast the
	// property that changed was the handles emitter.
	EffectScriptViewModel->RefreshEmitterNodes();
	if (bUpdatingFromSequencerDataChange == false)
	{
		RefreshSequencerTrack(GetTrackForHandleViewModel(EmitterHandleViewModel));
	}
	ReInitializeEffect();
}

void FNiagaraEffectViewModel::EffectParameterCollectionChanged()
{
	ResetEffect();
}

void FNiagaraEffectViewModel::EmitterPropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel)
{
	if (bUpdatingFromSequencerDataChange == false)
	{
		RefreshSequencerTrack(GetTrackForHandleViewModel(EmitterHandleViewModel));
	}
	ResetEffect();
}

void FNiagaraEffectViewModel::EmitterParameterValueChanged(FGuid ParamterId)
{
	ResetEffect();
}

void FNiagaraEffectViewModel::ParameterSelectionChanged()
{
	ResetCurveData();
}

void FNiagaraEffectViewModel::EffectParameterValueChanged(FGuid ParamterId)
{
	ResetEffect();
}

void FNiagaraEffectViewModel::EffectParameterBindingsChanged()
{
	ReInitializeEffect();
}

void FNiagaraEffectViewModel::ScriptCompiled()
{
	ReInitializeEffect();
}

void FNiagaraEffectViewModel::CurveChanged(TSharedPtr<INiagaraParameterCollectionViewModel> CollectionViewModel, FGuid ParameterId)
{
	CollectionViewModel->NotifyParameterChangedExternally(ParameterId);
}

void FNiagaraEffectViewModel::SynchronizeEffectDrivenParametersUI()
{
	for (TSharedRef<FNiagaraEmitterHandleViewModel>& EmitterHandleVM : EmitterHandleViewModels)
	{
		TSharedRef<FNiagaraEmitterViewModel> EmitterVM = EmitterHandleVM->GetEmitterViewModel();

		TSharedRef<FNiagaraScriptViewModel> SpawnVM = EmitterVM->GetSpawnScriptViewModel();
		TSharedRef<FNiagaraScriptViewModel> UpdateVM = EmitterVM->GetUpdateScriptViewModel();

		TSharedRef<FNiagaraScriptInputCollectionViewModel> SpawnInputCollectionVM = SpawnVM->GetInputCollectionViewModel();
		TSharedRef<FNiagaraScriptInputCollectionViewModel> UpdateInputCollectionVM = UpdateVM->GetInputCollectionViewModel();

		FNiagaraEmitterHandle* EmitterHandle = EmitterHandleVM->GetEmitterHandle();
		if (EmitterHandle != nullptr)
		{
			SpawnInputCollectionVM->SetAllParametersEditingEnabled(true);
			UpdateInputCollectionVM->SetAllParametersEditingEnabled(true);
			SpawnInputCollectionVM->SetAllParametersTooltipOverrides(FText::FromString(FString()));
			UpdateInputCollectionVM->SetAllParametersTooltipOverrides(FText::FromString(FString()));

			FText OverrideTextFormat = LOCTEXT("NiagaraEffectTooltipOverride", "Parameter {0} is driven by the effect graph.");
			const TArray<FNiagaraParameterBinding> Bindings = Effect.GetParameterBindings();
			for (const FNiagaraParameterBinding& Binding : Bindings)
			{
				if (Binding.GetDestinationEmitterId() == EmitterHandle->GetId())
				{
					TSharedPtr<INiagaraParameterViewModel> ParamVM = SpawnInputCollectionVM->GetParameterViewModel(Binding.GetDestinationParameterId());
					if (ParamVM.IsValid())
					{
						ParamVM->SetEditingEnabled(false);
						ParamVM->SetTooltipOverride(FText::Format(OverrideTextFormat, FText::FromName(ParamVM->GetName())));
					}

					ParamVM = UpdateInputCollectionVM->GetParameterViewModel(Binding.GetDestinationParameterId());
					if (ParamVM.IsValid())
					{
						ParamVM->SetEditingEnabled(false);
						ParamVM->SetTooltipOverride(FText::Format(OverrideTextFormat, FText::FromName(ParamVM->GetName())));
					}
				}
			}

			const TArray<FNiagaraParameterBinding> DataInterfaceBindings = Effect.GetDataInterfaceBindings();
			for (const FNiagaraParameterBinding& Binding : DataInterfaceBindings)
			{
				if (Binding.GetDestinationEmitterId() == EmitterHandle->GetId())
				{
					TSharedPtr<INiagaraParameterViewModel> ParamVM = SpawnInputCollectionVM->GetParameterViewModel(Binding.GetDestinationParameterId());
					if (ParamVM.IsValid())
					{
						ParamVM->SetEditingEnabled(false);
						ParamVM->SetTooltipOverride(FText::Format(OverrideTextFormat, FText::FromName(ParamVM->GetName())));
					}

					ParamVM = UpdateInputCollectionVM->GetParameterViewModel(Binding.GetDestinationParameterId());
					if (ParamVM.IsValid())
					{
						ParamVM->SetEditingEnabled(false);
						ParamVM->SetTooltipOverride(FText::Format(OverrideTextFormat, FText::FromName(ParamVM->GetName())));
					}
				}
			}
		}
	}
}

void FNiagaraEffectViewModel::SequencerDataChanged(EMovieSceneDataChangeType DataChangeType)
{
	bUpdatingFromSequencerDataChange = true;
	TSet<FGuid> VaildTrackEmitterHandleIds;
	for (UMovieSceneTrack* Track : NiagaraSequence->GetMovieScene()->GetMasterTracks())
	{
		UMovieSceneNiagaraEmitterTrack* EmitterTrack = CastChecked<UMovieSceneNiagaraEmitterTrack>(Track);
		TArray<UMovieSceneSection*> Sections = EmitterTrack->GetAllSections();
		UMovieSceneNiagaraEmitterSection* EmitterSection = Cast<UMovieSceneNiagaraEmitterSection>(Sections.Num() == 1 ? Sections[0] : nullptr);
		if (EmitterSection != nullptr)
		{
			VaildTrackEmitterHandleIds.Add(EmitterTrack->GetEmitterHandle()->GetId());

			TSharedPtr<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel = EmitterTrack->GetEmitterHandle();
			if (bCanRenameEmittersFromTimeline)
			{
				EmitterHandleViewModel->SetName(*EmitterTrack->GetDisplayName().ToString());
			}
			else
			{
				EmitterTrack->SetDisplayName(EmitterHandleViewModel->GetNameText());
			}
			EmitterHandleViewModel->SetIsEnabled(EmitterSection->IsActive());

			TSharedPtr<FNiagaraEmitterViewModel> EmitterViewModel = EmitterTrack->GetEmitterHandle()->GetEmitterViewModel();
			if (EmitterSection->IsInfinite() == false)
			{
				EmitterViewModel->SetStartTime(EmitterSection->GetStartTime());
				EmitterViewModel->SetEndTime(EmitterSection->GetEndTime());
			}

			TSharedPtr<FBurstCurve> BurstCurve = EmitterSection->GetBurstCurve();
			if (BurstCurve.IsValid())
			{
				UNiagaraEmitterProperties* Emitter = EmitterViewModel->GetEmitter();
				if (Emitter)
				{
					Emitter->Bursts.Empty();
				}
				TArray<FKeyHandle> KeyHandles;
				for (TKeyTimeIterator<float> KeyIterator = BurstCurve->IterateKeys(); KeyIterator; ++KeyIterator)
				{
					KeyHandles.Add(KeyIterator.GetKeyHandle());
				}

				for (const FKeyHandle& KeyHandle : KeyHandles)
				{
					auto Key = BurstCurve->GetKey(KeyHandle).GetValue();
					FNiagaraEmitterBurst Burst;
					Burst.Time = Key.Time;
					Burst.TimeRange = Key.Value.TimeRange;
					Burst.SpawnMinimum = Key.Value.SpawnMinimum;
					Burst.SpawnMaximum = Key.Value.SpawnMaximum;
					if (Emitter)
					{
						Emitter->Bursts.Add(Burst);
					}
				}
			}
		}
		bUpdatingFromSequencerDataChange = false;
	}

	TSet<FGuid> AllEmitterHandleIds;
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EmitterHandleViewModels)
	{
		AllEmitterHandleIds.Add(EmitterHandleViewModel->GetId());
	}

	TSet<FGuid> RemovedHandles = AllEmitterHandleIds.Difference(VaildTrackEmitterHandleIds);
	if (RemovedHandles.Num() > 0)
	{
		if (bCanRemoveEmittersFromTimeline)
		{
			Effect.RemoveEmitterHandlesById(RemovedHandles);
			RefreshAll();
		}
		else
		{
			RefreshSequencerTracks();
		}
	}

	EffectInstance->Reset(FNiagaraEffectInstance::EResetMode::DeferredReset);
}

void FNiagaraEffectViewModel::SequencerTimeChanged()
{
	// Skip the first update after going from stopped to playing because snapping in sequencer may have made the time
	// reverse by a small amount, and sending that update to the effect will reset it unnecessarily. *
	EMovieScenePlayerStatus::Type CurrentStatus = Sequencer->GetPlaybackStatus();
	bool bStartedPlaying = PreviousStatus == EMovieScenePlayerStatus::Stopped && CurrentStatus == EMovieScenePlayerStatus::Playing;
	if (CurrentStatus != EMovieScenePlayerStatus::Stopped &&
		bStartedPlaying == false)
	{
		EffectInstance->SetDesiredAge(FMath::Max(Sequencer->GetGlobalTime(), 0.0f));
	}
	PreviousStatus = CurrentStatus;
}

void FNiagaraEffectViewModel::EffectInstanceInitialized()
{
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EmitterHandleViewModels)
	{
		if (EmitterHandleViewModel->GetEmitterHandle())
		{
			EmitterHandleViewModel->SetSimulation(EffectInstance->GetSimulationForHandle(*EmitterHandleViewModel->GetEmitterHandle()));
		}
	}
}

void FNiagaraEffectViewModel::ResynchronizeAllHandles()
{
	Effect.ResynchronizeAllHandles();
	RefreshAll();
}

#undef LOCTEXT_NAMESPACE // NiagaraEffectViewModel
