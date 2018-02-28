// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSequence.h"
#include "MovieSceneCompiler.h"
#include "Sections/MovieSceneSubSection.h"
#include "Compilation/MovieSceneEvaluationTemplateGenerator.h"

#include "IMovieSceneModule.h"
#include "Algo/Sort.h"

DECLARE_CYCLE_STAT(TEXT("Entire Evaluation Cost"), MovieSceneEval_EntireEvaluationCost, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Gather Entries For Frame"), MovieSceneEval_GatherEntries, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Call Setup() and TearDown()"), MovieSceneEval_CallSetupTearDown, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Evaluate Group"), MovieSceneEval_EvaluateGroup, STATGROUP_MovieSceneEval);

/** Scoped helper class that facilitates the delayed restoration of preanimated state for specific evaluation keys */
struct FDelayedPreAnimatedStateRestore
{
	FDelayedPreAnimatedStateRestore(IMovieScenePlayer& InPlayer)
		: Player(InPlayer)
	{}

	~FDelayedPreAnimatedStateRestore()
	{
		RestoreNow();
	}

	void Add(FMovieSceneEvaluationKey Key)
	{
		KeysToRestore.Add(Key);
	}

	void RestoreNow()
	{
		for (FMovieSceneEvaluationKey Key : KeysToRestore)
		{
			Player.PreAnimatedState.RestorePreAnimatedState(Player, Key);
		}
		KeysToRestore.Reset();
	}

private:
	/** The movie scene player to restore with */
	IMovieScenePlayer& Player;
	/** The array of keys to restore */
	TArray<FMovieSceneEvaluationKey> KeysToRestore;
};

FMovieSceneEvaluationTemplateInstance::FMovieSceneEvaluationTemplateInstance()
	: Sequence(nullptr), Template(nullptr), SubData(nullptr)
{}

FMovieSceneEvaluationTemplateInstance::FMovieSceneEvaluationTemplateInstance(UMovieSceneSequence& InSequence, FMovieSceneEvaluationTemplate* InTemplate)
	: Sequence(&InSequence), Template(InTemplate), SubData(nullptr)
{}

FMovieSceneEvaluationTemplateInstance::FMovieSceneEvaluationTemplateInstance(const FMovieSceneSubSequenceData* InSubData, IMovieSceneSequenceTemplateStore& TemplateStore)
	: Sequence(nullptr), Template(nullptr), SubData(InSubData)
{
	Sequence = InSubData ? InSubData->GetSequence() : nullptr;
	if (Sequence)
	{
		Template = &TemplateStore.AccessTemplate(*Sequence);
	}
}

FMovieSceneRootEvaluationTemplateInstance::FMovieSceneRootEvaluationTemplateInstance()
	: RootSequence(nullptr)
	, TemplateStore(MakeShared<FMovieSceneSequencePrecompiledTemplateStore>())
{
}

FMovieSceneRootEvaluationTemplateInstance::~FMovieSceneRootEvaluationTemplateInstance()
{
}

void FMovieSceneRootEvaluationTemplateInstance::Initialize(UMovieSceneSequence& InRootSequence, IMovieScenePlayer& Player, TSharedRef<IMovieSceneSequenceTemplateStore> InTemplateStore)
{
	if (RootSequence != &InRootSequence)
	{
		Finish(Player);
	}

	TemplateStore = MoveTemp(InTemplateStore);

	Initialize(InRootSequence, Player);
}

void FMovieSceneRootEvaluationTemplateInstance::Initialize(UMovieSceneSequence& InRootSequence, IMovieScenePlayer& Player)
{
	if (RootSequence.Get() != &InRootSequence)
	{
		// Always ensure that there is no persistent data when initializing a new sequence
		// to ensure we don't collide with the previous sequence's entity keys
		Player.State.PersistentEntityData.Reset();
		Player.State.PersistentSharedData.Reset();

		LastFrameMetaData.Reset();
		ThisFrameMetaData.Reset();
		ExecutionTokens = FMovieSceneExecutionTokens();

		TransientInstances.ResetSubInstances();
	}

	RootSequence = &InRootSequence;
	RootTemplate = &TemplateStore->AccessTemplate(InRootSequence);
	TransientInstances.RootInstance = FMovieSceneEvaluationTemplateInstance(InRootSequence, RootTemplate);
}

void FMovieSceneRootEvaluationTemplateInstance::Finish(IMovieScenePlayer& Player)
{
	Swap(ThisFrameMetaData, LastFrameMetaData);
	ThisFrameMetaData.Reset();

	CallSetupTearDown(Player);

	TransientInstances.Reset();
}

void FMovieSceneRootEvaluationTemplateInstance::Evaluate(FMovieSceneContext Context, IMovieScenePlayer& Player, FMovieSceneSequenceID InOverrideRootID)
{
	SCOPE_CYCLE_COUNTER(MovieSceneEval_EntireEvaluationCost);

	Swap(ThisFrameMetaData, LastFrameMetaData);
	ThisFrameMetaData.Reset();

	if (TransientInstances.RootID != InOverrideRootID)
	{
		// Tear everything down if we're evaluating a different root sequence
		CallSetupTearDown(Player);
		LastFrameMetaData.Reset();
	}

	RootOverridePath.Set(InOverrideRootID, GetHierarchy());

	const FMovieSceneEvaluationGroup* GroupToEvaluate = SetupFrame(Player, InOverrideRootID, Context);
	if (!GroupToEvaluate)
	{
		CallSetupTearDown(Player);
		return;
	}

	// Cause stale tracks to not restore until after evaluation. This fixes issues when tracks that are set to 'Restore State' are regenerated, causing the state to be restored then re-animated by the new track
	FDelayedPreAnimatedStateRestore DelayedRestore(Player);

	// Run the post root evaluate steps which invoke tear downs for anything no longer evaluated.
	// Do this now to ensure they don't undo any of the current frame's execution tokens 
	CallSetupTearDown(Player, &DelayedRestore);

	// Ensure any null objects are not cached
	Player.State.InvalidateExpiredObjects();

	// Accumulate execution tokens into this structure
	EvaluateGroup(*GroupToEvaluate, Context, Player);

	// Process execution tokens
	ExecutionTokens.Apply(Context, Player);
}

const FMovieSceneEvaluationGroup* FMovieSceneRootEvaluationTemplateInstance::SetupFrame(IMovieScenePlayer& Player, FMovieSceneSequenceID InOverrideRootID, FMovieSceneContext Context)
{
	UMovieSceneSequence* RootSequencePtr = RootSequence.Get();
	if (!RootSequencePtr)
	{
		return nullptr;
	}

	// Create the instance for the current root override
	FMovieSceneEvaluationTemplateInstance RootOverrideInstance;

	if (InOverrideRootID != MovieSceneSequenceID::Root)
	{
		if (const FMovieSceneSubSequenceData* OverrideSubData = GetHierarchy().FindSubData(InOverrideRootID))
		{
			RootOverrideInstance = FMovieSceneEvaluationTemplateInstance(OverrideSubData, *TemplateStore);
		}
	}
	else
	{
		RootOverrideInstance = FMovieSceneEvaluationTemplateInstance(*RootSequencePtr, RootTemplate);
	}

	if (!ensureMsgf(RootOverrideInstance.IsValid(), TEXT("Could not find valid sequence or template for supplied sequence ID.")))
	{
		return nullptr;
	}

	// Ensure the root is up to date
	if (RootOverrideInstance.Template->SequenceSignature != RootOverrideInstance.Sequence->GetSignature())
	{
		FMovieSceneEvaluationTemplateGenerator(*RootOverrideInstance.Sequence, *RootOverrideInstance.Template).Generate();
	}

	// First off, attempt to find the evaluation group in the existing evaluation field data from the template
	int32 TemplateFieldIndex = RootOverrideInstance.Template->EvaluationField.GetSegmentFromTime(Context.GetTime());

	if (TemplateFieldIndex != INDEX_NONE)
	{
		const FMovieSceneEvaluationMetaData& FieldMetaData = RootOverrideInstance.Template->EvaluationField.GetMetaData(TemplateFieldIndex);

		// Verify that this field entry is still valid (all its cached signatures are still the same)
		TRange<float> InvalidatedSubSequenceRange = TRange<float>::Empty();
		if (FieldMetaData.IsDirty(*RootOverrideInstance.Sequence, RootOverrideInstance.Template->Hierarchy, *TemplateStore, &InvalidatedSubSequenceRange))
		{
			TemplateFieldIndex = INDEX_NONE;

			if (!InvalidatedSubSequenceRange.IsEmpty())
			{
				// Invalidate the evaluation field for the root template (it may not exist until we compile below)
				RootOverrideInstance.Template->EvaluationField.Invalidate(InvalidatedSubSequenceRange);
			}
		}
	}

	if (TemplateFieldIndex == INDEX_NONE)
	{
		// We need to compile an entry in the evaluation field
		static bool bFullCompile = false;
		if (bFullCompile)
		{
			FMovieSceneCompiler::Compile(*RootOverrideInstance.Sequence, *TemplateStore);
			TemplateFieldIndex = RootOverrideInstance.Template->EvaluationField.GetSegmentFromTime(Context.GetTime());
		}
		else
		{
			float CurrentTime = Context.GetTime();
			TOptional<FCompiledGroupResult> CompileResult = FMovieSceneCompiler::CompileTime(CurrentTime, *RootOverrideInstance.Sequence, *TemplateStore);

			if (CompileResult.IsSet())
			{
				TRange<float> FieldRange = CompileResult->Range;
				TemplateFieldIndex = RootOverrideInstance.Template->EvaluationField.Insert(
					CurrentTime,
					FieldRange,
					MoveTemp(CompileResult->Group),
					MoveTemp(CompileResult->MetaData)
					);
			}
		}
	}

	if (TemplateFieldIndex != INDEX_NONE)
	{
		// Set meta-data
		ThisFrameMetaData = RootOverrideInstance.Template->EvaluationField.GetMetaData(TemplateFieldIndex);

		RecreateInstances(RootOverrideInstance, InOverrideRootID, Player);

		return &RootOverrideInstance.Template->EvaluationField.GetGroup(TemplateFieldIndex);
	}

	return nullptr;
}

void FMovieSceneRootEvaluationTemplateInstance::EvaluateGroup(const FMovieSceneEvaluationGroup& Group, const FMovieSceneContext& RootContext, IMovieScenePlayer& Player)
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_EvaluateGroup);

	FPersistentEvaluationData PersistentDataProxy(Player);

	FMovieSceneEvaluationOperand Operand;

	FMovieSceneContext Context = RootContext;
	FMovieSceneContext SubContext = Context;

	for (const FMovieSceneEvaluationGroupLUTIndex& Index : Group.LUTIndices)
	{
		int32 TrackIndex = Index.LUTOffset;
		
		// Initialize anything that wants to be initialized first
		for ( ; TrackIndex < Index.LUTOffset + Index.NumInitPtrs; ++TrackIndex)
		{
			FMovieSceneEvaluationFieldSegmentPtr SegmentPtr = Group.SegmentPtrLUT[TrackIndex];

			// Ensure we're able to find the sequence instance in our root if we've overridden
			SegmentPtr.SequenceID = RootOverridePath.Remap(SegmentPtr.SequenceID);

			const FMovieSceneEvaluationTemplateInstance& Instance = TransientInstances.GetChecked(SegmentPtr.SequenceID);
			const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(SegmentPtr.TrackIdentifier);

			if (Track)
			{
				Operand.ObjectBindingID = Track->GetObjectBindingID();
				Operand.SequenceID = SegmentPtr.SequenceID;
				
				FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);

				PersistentDataProxy.SetTrackKey(TrackKey);
				Player.PreAnimatedState.SetCaptureEntity(TrackKey, EMovieSceneCompletionMode::KeepState);

				SubContext = Context;
				if (Instance.SubData)
				{
					SubContext = Context.Transform(Instance.SubData->RootToSequenceTransform);

					// Hittest against the sequence's pre and postroll ranges
					SubContext.ReportOuterSectionRanges(Instance.SubData->PreRollRange, Instance.SubData->PostRollRange);
					SubContext.SetHierarchicalBias(Instance.SubData->HierarchicalBias);
				}

				Track->Initialize(SegmentPtr.SegmentID, Operand, SubContext, PersistentDataProxy, Player);
			}
		}

		// Then evaluate

		// *Threading candidate*
		// @todo: if we want to make this threaded, we need to:
		//  - Make the execution tokens threadsafe, and sortable (one container per thread + append?)
		//  - Do the above in a lockless manner
		for (; TrackIndex < Index.LUTOffset + Index.NumInitPtrs + Index.NumEvalPtrs; ++TrackIndex)
		{
			FMovieSceneEvaluationFieldSegmentPtr SegmentPtr = Group.SegmentPtrLUT[TrackIndex];

			// Ensure we're able to find the sequence instance in our root if we've overridden
			SegmentPtr.SequenceID = RootOverridePath.Remap(SegmentPtr.SequenceID);

			const FMovieSceneEvaluationTemplateInstance& Instance = TransientInstances.GetChecked(SegmentPtr.SequenceID);
			const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(SegmentPtr.TrackIdentifier);

			if (Track)
			{
				Operand.ObjectBindingID = Track->GetObjectBindingID();
				Operand.SequenceID = SegmentPtr.SequenceID;

				FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);

				PersistentDataProxy.SetTrackKey(TrackKey);

				ExecutionTokens.SetOperand(Operand);
				ExecutionTokens.SetCurrentScope(FMovieSceneEvaluationScope(TrackKey, EMovieSceneCompletionMode::KeepState));

				SubContext = Context;
				if (Instance.SubData)
				{
					SubContext = Context.Transform(Instance.SubData->RootToSequenceTransform);

					// Hittest against the sequence's pre and postroll ranges
					SubContext.ReportOuterSectionRanges(Instance.SubData->PreRollRange, Instance.SubData->PostRollRange);
					SubContext.SetHierarchicalBias(Instance.SubData->HierarchicalBias);
				}

				Track->Evaluate(
					SegmentPtr.SegmentID,
					Operand,
					SubContext,
					PersistentDataProxy,
					ExecutionTokens);
			}
		}

		ExecutionTokens.Apply(Context, Player);
	}
}

void FMovieSceneRootEvaluationTemplateInstance::CallSetupTearDown(IMovieScenePlayer& Player, FDelayedPreAnimatedStateRestore* DelayedRestore)
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_CallSetupTearDown);

	FPersistentEvaluationData PersistentDataProxy(Player);

	TArray<FMovieSceneOrderedEvaluationKey> ExpiredEntities;
	TArray<FMovieSceneOrderedEvaluationKey> NewEntities;
	ThisFrameMetaData.DiffEntities(LastFrameMetaData, &NewEntities, &ExpiredEntities);
	
	for (const FMovieSceneOrderedEvaluationKey& OrderedKey : ExpiredEntities)
	{
		FMovieSceneEvaluationKey Key = OrderedKey.Key;

		// Ensure we're able to find the sequence instance in our root if we've overridden
		Key.SequenceID = RootOverridePath.Remap(Key.SequenceID);

		const FMovieSceneEvaluationTemplateInstance* Instance = TransientInstances.Find(Key.SequenceID);
		if (Instance && Instance->IsValid())
		{
			const FMovieSceneEvaluationTrack* Track = Instance->Template->FindTrack(Key.TrackIdentifier);
			const bool bStaleTrack = Instance->Template->IsTrackStale(Key.TrackIdentifier);

			// Track data key may be required by both tracks and sections
			PersistentDataProxy.SetTrackKey(Key.AsTrack());

			if (Key.SectionIndex == uint32(-1))
			{
				if (Track)
				{
					Track->OnEndEvaluation(PersistentDataProxy, Player);
				}
				
				PersistentDataProxy.ResetTrackData();
			}
			else
			{
				PersistentDataProxy.SetSectionKey(Key);
				if (Track && Track->HasChildTemplate(Key.SectionIndex))
				{
					Track->GetChildTemplate(Key.SectionIndex).OnEndEvaluation(PersistentDataProxy, Player);
				}

				PersistentDataProxy.ResetSectionData();
			}

			if (bStaleTrack && DelayedRestore)
			{
				DelayedRestore->Add(Key);
			}
			else
			{
				Player.PreAnimatedState.RestorePreAnimatedState(Player, Key);
			}
		}
	}

	for (const FMovieSceneOrderedEvaluationKey& OrderedKey : NewEntities)
	{
		FMovieSceneEvaluationKey Key = OrderedKey.Key;

		// Ensure we're able to find the sequence instance in our root if we've overridden
		Key.SequenceID = RootOverridePath.Remap(Key.SequenceID);

		const FMovieSceneEvaluationTemplateInstance& Instance = TransientInstances.GetChecked(Key.SequenceID);
		check(Instance.IsValid());

		if (const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(Key.TrackIdentifier))
		{
			PersistentDataProxy.SetTrackKey(Key.AsTrack());

			if (Key.SectionIndex == uint32(-1))
			{
				Track->OnBeginEvaluation(PersistentDataProxy, Player);
			}
			else
			{
				PersistentDataProxy.SetSectionKey(Key);
				Track->GetChildTemplate(Key.SectionIndex).OnBeginEvaluation(PersistentDataProxy, Player);
			}
		}
	}

	// Tear down spawned objects
	FMovieSceneSpawnRegister& Register = Player.GetSpawnRegister();

	TArray<FMovieSceneSequenceID> ExpiredSequenceIDs;
	ThisFrameMetaData.DiffSequences(LastFrameMetaData, nullptr, &ExpiredSequenceIDs);
	for (FMovieSceneSequenceID ExpiredID : ExpiredSequenceIDs)
	{
		Register.OnSequenceExpired(RootOverridePath.Remap(ExpiredID), Player);
	}
}

bool FMovieSceneRootEvaluationTemplateInstance::IsDirty() const
{
	if (TransientInstances.RootInstance.IsValid())
	{
		return LastFrameMetaData.IsDirty(*TransientInstances.RootInstance.Sequence, TransientInstances.RootInstance.Template->Hierarchy, *TemplateStore);
	}

	return true;
}

void FMovieSceneRootEvaluationTemplateInstance::CopyActuators(FMovieSceneBlendingAccumulator& Accumulator) const
{
	Accumulator.Actuators = ExecutionTokens.GetBlendingAccumulator().Actuators;
}

UMovieSceneSequence* FMovieSceneRootEvaluationTemplateInstance::GetSequence(FMovieSceneSequenceIDRef SequenceID) const
{
	if (SequenceID == MovieSceneSequenceID::Root)
	{
		return RootSequence.Get();
	}
	
	const FMovieSceneSubSequenceData* SubData = GetHierarchy().FindSubData(SequenceID);
	return SubData ? SubData->GetSequence() : nullptr;
}

FMovieSceneEvaluationTemplate* FMovieSceneRootEvaluationTemplateInstance::FindTemplate(FMovieSceneSequenceIDRef SequenceID)
{
	if (SequenceID == MovieSceneSequenceID::Root)
	{
		return RootTemplate;
	}

	const FMovieSceneSubSequenceData* SubData = GetHierarchy().FindSubData(SequenceID);
	UMovieSceneSequence* Sequence = SubData ? SubData->GetSequence() : nullptr;

	return Sequence ? &TemplateStore->AccessTemplate(*Sequence) : nullptr;
}

void FMovieSceneRootEvaluationTemplateInstance::RecreateInstances(const FMovieSceneEvaluationTemplateInstance& RootOverrideInstance, FMovieSceneSequenceID InOverrideRootID, IMovieScenePlayer& Player)
{
	check(RootOverrideInstance.IsValid());

	TransientInstances.RootID = InOverrideRootID;
	TransientInstances.RootInstance = RootOverrideInstance;
	
	Player.State.AssignSequence(TransientInstances.RootID, *RootOverrideInstance.Sequence, Player);

	TransientInstances.ResetSubInstances();

	// We recreate all necessary sequence instances for the current and previous frames by diffing the sequences active last frame, with this frame
	TArray<FMovieSceneSequenceID> PreviousAndCurrentFrameSequenceIDs = LastFrameMetaData.ActiveSequences;
	ThisFrameMetaData.DiffSequences(LastFrameMetaData, &PreviousAndCurrentFrameSequenceIDs, nullptr);

	for (FMovieSceneSequenceID SequenceID : PreviousAndCurrentFrameSequenceIDs)
	{
		const FMovieSceneSubSequenceData* SubData = RootOverrideInstance.Template->Hierarchy.FindSubData(SequenceID);
		if (SubData)
		{
			FMovieSceneEvaluationTemplateInstance NewInstance(SubData, *TemplateStore);
			
			// Always add sequence IDs as the root
			SequenceID = RootOverridePath.Remap(SequenceID);
			TransientInstances.Add(SequenceID, NewInstance);

			if (NewInstance.Sequence)
			{
				Player.State.AssignSequence(SequenceID, *NewInstance.Sequence, Player);
			}
		}
	}
}