// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSequenceInstance.h"
#include "MovieSceneExecutionTokens.h"

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

FMovieSceneEvaluationTemplateInstance::FMovieSceneEvaluationTemplateInstance(UMovieSceneSequence& InSequence, const FMovieSceneEvaluationTemplate& InTemplate)
	: Sequence(&InSequence)
	, Template(&InTemplate)
	, PreRollRange(TRange<float>::Empty())
	, PostRollRange(TRange<float>::Empty())
{
	if (Template->bHasLegacyTrackInstances)
	{
		// Initialize the legacy sequence instance
		LegacySequenceInstance = MakeShareable(new FMovieSceneSequenceInstance(InSequence, MovieSceneSequenceID::Root));
	}
}

FMovieSceneEvaluationTemplateInstance::FMovieSceneEvaluationTemplateInstance(const FMovieSceneSubSequenceData& InSubData, const FMovieSceneEvaluationTemplate& InTemplate, FMovieSceneSequenceIDRef InSequenceID)
	: Sequence(InSubData.Sequence)
	, RootToSequenceTransform(InSubData.RootToSequenceTransform)
	, Template(&InTemplate)
	, PreRollRange(InSubData.PreRollRange)
	, PostRollRange(InSubData.PostRollRange)
{
	if (Template->bHasLegacyTrackInstances)
	{
		// Initialize the legacy sequence instance
		LegacySequenceInstance = MakeShareable(new FMovieSceneSequenceInstance(*InSubData.Sequence, InSequenceID));
	}
}

FMovieSceneRootEvaluationTemplateInstance::FMovieSceneRootEvaluationTemplateInstance()
	: RootSequence(nullptr)
	, TemplateStore(MakeShareable(new FMovieSceneSequenceTemplateStore))
	, bIsDirty(false)
{

}

FMovieSceneRootEvaluationTemplateInstance::~FMovieSceneRootEvaluationTemplateInstance()
{
	Reset();
}

void FMovieSceneRootEvaluationTemplateInstance::Reset()
{
	if (TemplateStore->AreTemplatesVolatile())
	{
		if (UMovieSceneSequence* Sequence = RootInstance.Sequence.Get())
		{
			Sequence->OnSignatureChanged().RemoveAll(this);
		}

		for (auto& Pair : SubInstances)
		{
			if (UMovieSceneSequence* Sequence = Pair.Value.Sequence.Get())
			{
				Sequence->OnSignatureChanged().RemoveAll(this);
			}
		}
	}

	SubInstances.Reset();
}

void FMovieSceneRootEvaluationTemplateInstance::Initialize(UMovieSceneSequence& InRootSequence, IMovieScenePlayer& Player, TSharedRef<FMovieSceneSequenceTemplateStore> InTemplateStore)
{
	if (RootSequence != &InRootSequence)
	{
		Finish(Player);
	}

	// Ensure we reset everything before we overwrite the template store (which potentially owns templates we've previously referenced)
	Reset();

	TemplateStore = MoveTemp(InTemplateStore);

	Initialize(InRootSequence, Player);
}

void FMovieSceneRootEvaluationTemplateInstance::Initialize(UMovieSceneSequence& InRootSequence, IMovieScenePlayer& Player)
{
	Reset();

	if (RootSequence.Get() != &InRootSequence)
	{
		// Always ensure that there is no persistent data when initializing a new sequence
		// to ensure we don't collide with the previous sequence's entity keys
		Player.State.PersistentEntityData.Reset();
		Player.State.PersistentSharedData.Reset();
	}

	const bool bAddEvents = TemplateStore->AreTemplatesVolatile();
	
	RootSequence = &InRootSequence;

	const FMovieSceneEvaluationTemplate& RootTemplate = TemplateStore->GetCompiledTemplate(InRootSequence);
	
	Player.State.AssignSequence(MovieSceneSequenceID::Root, InRootSequence);
	RootInstance = FMovieSceneEvaluationTemplateInstance(InRootSequence, RootTemplate);

	if (bAddEvents)
	{
		InRootSequence.OnSignatureChanged().AddRaw(this, &FMovieSceneRootEvaluationTemplateInstance::OnSequenceChanged);
	}

	for (auto& Pair : RootTemplate.Hierarchy.AllSubSequenceData())
	{
		const FMovieSceneSubSequenceData& Data = Pair.Value;
		FMovieSceneSequenceID SequenceID = Pair.Key;

		if (!Data.Sequence)
		{
			continue;
		}

		Player.State.AssignSequence(SequenceID, *Data.Sequence);

		const FMovieSceneEvaluationTemplate& ChildTemplate = TemplateStore->GetCompiledTemplate(*Data.Sequence, FObjectKey(Data.SequenceKeyObject));
		FMovieSceneEvaluationTemplateInstance& Instance = SubInstances.Add(SequenceID, FMovieSceneEvaluationTemplateInstance(Data, ChildTemplate, SequenceID));

		if (bAddEvents)
		{
			Data.Sequence->OnSignatureChanged().AddRaw(this, &FMovieSceneRootEvaluationTemplateInstance::OnSequenceChanged);
		}
	}

	bIsDirty = false;

	OnUpdatedEvent.Broadcast();
}

void FMovieSceneRootEvaluationTemplateInstance::Finish(IMovieScenePlayer& Player)
{
	Swap(EntitiesEvaluatedThisFrame, EntitiesEvaluatedLastFrame);
	EntitiesEvaluatedThisFrame.Reset();

	CallSetupTearDown(Player);
}

void FMovieSceneRootEvaluationTemplateInstance::Evaluate(FMovieSceneContext Context, IMovieScenePlayer& Player, FMovieSceneSequenceID OverrideRootID)
{
	if (bIsDirty && RootSequence.IsValid())
	{
		Initialize(*RootSequence.Get(), Player);
	}

	SCOPE_CYCLE_COUNTER(MovieSceneEval_EntireEvaluationCost);

	Swap(EntitiesEvaluatedThisFrame, EntitiesEvaluatedLastFrame);
	EntitiesEvaluatedThisFrame.Reset();

	Swap(ActiveSequencesThisFrame, ActiveSequencesLastFrame);
	ActiveSequencesThisFrame.Reset();

	const FMovieSceneEvaluationTemplateInstance* Instance = GetInstance(OverrideRootID);

	ensureMsgf(Instance, TEXT("Could not find instance for supplied sequence ID."));

	const int32 FieldIndex = Instance ? Instance->Template->EvaluationField.GetSegmentFromTime(Context.GetTime() * Instance->RootToSequenceTransform) : INDEX_NONE;
	if (FieldIndex == INDEX_NONE)
	{
		CallSetupTearDown(Player);
		return;
	}

	// Construct a path that allows us to remap sequence IDs from the local (OverrideRootID) template, to the master template
	ReverseOverrideRootPath.Reset();
	{
		FMovieSceneSequenceID CurrentSequenceID = OverrideRootID;
		const FMovieSceneSequenceHierarchy& Hierarchy = GetHierarchy();
		while (CurrentSequenceID != MovieSceneSequenceID::Root)
		{
			const FMovieSceneSequenceHierarchyNode* CurrentNode = Hierarchy.FindNode(CurrentSequenceID);
			const FMovieSceneSubSequenceData* SubData = Hierarchy.FindSubData(CurrentSequenceID);
			if (!ensureAlwaysMsgf(CurrentNode && SubData, TEXT("Malformed sequence hierarchy")))
			{
				return;
			}

			ReverseOverrideRootPath.Add(SubData->DeterministicSequenceID);
			CurrentSequenceID = CurrentNode->ParentID;
		}
	}

	const FMovieSceneEvaluationGroup& Group = Instance->Template->EvaluationField.Groups[FieldIndex];

	GatherEntities(Group, Player);

	// Gather the active sequences for this frame, remapping them to the root if necessary
	ActiveSequencesThisFrame = Instance->Template->EvaluationField.MetaData[FieldIndex].ActiveSequences;
	if (OverrideRootID != MovieSceneSequenceID::Root)
	{
		for (FMovieSceneSequenceID& ID : ActiveSequencesThisFrame)
		{
			ID = GetSequenceIdForRoot(ID);
		}
	}

	// Cause stale tracks to not restore until after evaluation. This fixes issues when tracks that are set to 'Restore State' are regenerated, causing the state to be restored then re-animated by the new track
	FDelayedPreAnimatedStateRestore DelayedRestore(Player);

	// Run the post root evaluate steps which invoke tear downs for anything no longer evaluated.
	// Do this now to ensure they don't undo any of the current frame's execution tokens 
	CallSetupTearDown(Player, &DelayedRestore);

	// Ensure any null objects are not cached
	Player.State.InvalidateExpiredObjects();

	// Accumulate execution tokens into this structure
	FMovieSceneExecutionTokens ExecutionTokens;
	EvaluateGroup(Group, Context, Player, ExecutionTokens);

	// Process execution tokens
	ExecutionTokens.Apply(Player);
}

void FMovieSceneRootEvaluationTemplateInstance::GatherEntities(const FMovieSceneEvaluationGroup& Group, IMovieScenePlayer& Player)
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_GatherEntries);

	// See what's happening this frame
	for (const FMovieSceneEvaluationGroupLUTIndex& Index : Group.LUTIndices)
	{
		for (int32 PreEvalTrackIndex = Index.LUTOffset; PreEvalTrackIndex < Index.LUTOffset + Index.NumInitPtrs + Index.NumEvalPtrs; ++PreEvalTrackIndex)
		{
			FMovieSceneEvaluationFieldSegmentPtr SegmentPtr = Group.SegmentPtrLUT[PreEvalTrackIndex];
			
			// Ensure we're able to find the sequence instance in our root if we've overridden
			SegmentPtr.SequenceID = GetSequenceIdForRoot(SegmentPtr.SequenceID);

			const FMovieSceneEvaluationTemplateInstance& Instance = GetInstanceChecked(SegmentPtr.SequenceID);

			if (const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(SegmentPtr.TrackIdentifier))
			{
				const FMovieSceneSegment& Segment = Track->GetSegment(SegmentPtr.SegmentIndex);

				FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);

				// Important: Add the track key first. When tearing down unevaluated entites, we do so in a strictly reverse order.
				// This is necessary as sections may depend on track data, but tracks should have no knowledge of their inner section data
				EntitiesEvaluatedThisFrame.Add(TrackKey);

				// Add all the sections
				for (const FSectionEvaluationData& EvalData : Segment.Impls)
				{
					EntitiesEvaluatedThisFrame.Add(TrackKey.AsSection(EvalData.ImplIndex));
				}
			}
		}
	}
}

void FMovieSceneRootEvaluationTemplateInstance::EvaluateGroup(const FMovieSceneEvaluationGroup& Group, const FMovieSceneContext& RootContext, IMovieScenePlayer& Player, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_EvaluateGroup);

	FPersistentEvaluationData PersistentDataProxy(Player.State.PersistentEntityData, Player.State.PersistentSharedData);
	
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
			SegmentPtr.SequenceID = GetSequenceIdForRoot(SegmentPtr.SequenceID);

			const FMovieSceneEvaluationTemplateInstance& Instance = GetInstanceChecked(SegmentPtr.SequenceID);
			const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(SegmentPtr.TrackIdentifier);

			if (Track)
			{
				Operand.ObjectBindingID = Track->GetObjectBindingID();
				Operand.SequenceID = SegmentPtr.SequenceID;
				
				FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);

				PersistentDataProxy.SetTrackKey(TrackKey);
				Player.PreAnimatedState.SetCaptureEntity(TrackKey, EMovieSceneCompletionMode::KeepState);

				SubContext = Context;
				if (SegmentPtr.SequenceID != MovieSceneSequenceID::Root)
				{
					SubContext = Context.Transform(Instance.RootToSequenceTransform);

					// Hittest against the sequence's pre and postroll ranges
					SubContext.ReportOuterSectionRanges(Instance.PreRollRange, Instance.PostRollRange);
				}

				Track->Initialize(SegmentPtr.SegmentIndex, Operand, SubContext, PersistentDataProxy, Player);
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
			SegmentPtr.SequenceID = GetSequenceIdForRoot(SegmentPtr.SequenceID);

			const FMovieSceneEvaluationTemplateInstance& Instance = GetInstanceChecked(SegmentPtr.SequenceID);
			const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(SegmentPtr.TrackIdentifier);

			if (Track)
			{
				Operand.ObjectBindingID = Track->GetObjectBindingID();
				Operand.SequenceID = SegmentPtr.SequenceID;

				FMovieSceneEvaluationKey TrackKey(SegmentPtr.SequenceID, SegmentPtr.TrackIdentifier);

				PersistentDataProxy.SetTrackKey(TrackKey);

				ExecutionTokens.SetOperand(Operand);
				ExecutionTokens.SetTrack(TrackKey);

				Player.PreAnimatedState.SetCaptureEntity(TrackKey, EMovieSceneCompletionMode::KeepState);

				SubContext = Context;
				if (SegmentPtr.SequenceID != MovieSceneSequenceID::Root)
				{
					SubContext = Context.Transform(Instance.RootToSequenceTransform);

					// Hittest against the sequence's pre and postroll ranges
					SubContext.ReportOuterSectionRanges(Instance.PreRollRange, Instance.PostRollRange);
				}

				Track->Evaluate(
					SegmentPtr.SegmentIndex,
					Operand,
					SubContext,
					PersistentDataProxy,
					ExecutionTokens);
			}
		}

		if (Index.bRequiresImmediateFlush)
		{
			ExecutionTokens.Apply(Player);
		}
	}
}

void FMovieSceneRootEvaluationTemplateInstance::CallSetupTearDown(IMovieScenePlayer& Player, FDelayedPreAnimatedStateRestore* DelayedRestore)
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_CallSetupTearDown);

	FPersistentEvaluationData PersistentDataProxy(Player.State.PersistentEntityData, Player.State.PersistentSharedData);
	
	// Always tear down entites in reverse order
	Algo::Reverse(EntitiesEvaluatedLastFrame.OrderedKeys);
	for (const FMovieSceneEvaluationKey& Key : EntitiesEvaluatedLastFrame.OrderedKeys)
	{
		if (EntitiesEvaluatedThisFrame.Contains(Key))
		{
			continue;
		}

		const FMovieSceneEvaluationTemplateInstance* Instance = FindInstance(Key.SequenceID);
		if (!Instance)
		{
			continue;
		}

		const FMovieSceneEvaluationTrack* Track = Instance->Template->FindTrack(Key.TrackIdentifier);
		const bool bStaleTrack = Instance->Template->IsTrackStale(Key.TrackIdentifier);

		// Track data key may be required by both tracks and sections
		PersistentDataProxy.SetTrackKey(Key.AsTrack());

		if (Key.SectionIdentifier == uint32(-1))
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
			if (Track && Track->HasChildTemplate(Key.SectionIdentifier))
			{
				Track->GetChildTemplate(Key.SectionIdentifier).OnEndEvaluation(PersistentDataProxy, Player);
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

	for (const FMovieSceneEvaluationKey& Key : EntitiesEvaluatedThisFrame.OrderedKeys)
	{
		if (EntitiesEvaluatedLastFrame.Contains(Key))
		{
			continue;
		}

		const FMovieSceneEvaluationTemplateInstance& Instance = GetInstanceChecked(Key.SequenceID);

		if (const FMovieSceneEvaluationTrack* Track = Instance.Template->FindTrack(Key.TrackIdentifier))
		{
			PersistentDataProxy.SetTrackKey(Key.AsTrack());

			if (Key.SectionIdentifier == uint32(-1))
			{
				Track->OnBeginEvaluation(PersistentDataProxy, Player);
			}
			else
			{
				PersistentDataProxy.SetSectionKey(Key);
				Track->GetChildTemplate(Key.SectionIdentifier).OnBeginEvaluation(PersistentDataProxy, Player);
			}
		}
	}

	// Determine which sub sequences are no longer evaluated this frame.
	// This algorithm works on the premise that each array is sorted, and each ID can only appear once
	FMovieSceneSpawnRegister& Register = Player.GetSpawnRegister();

	auto ThisFrameIDs = ActiveSequencesThisFrame.CreateConstIterator();
	for (FMovieSceneSequenceID LastID : ActiveSequencesLastFrame)
	{
		if (ThisFrameIDs)
		{
			FMovieSceneSequenceID ThisID = *ThisFrameIDs;
			if (LastID == ThisID)
			{
				continue;
			}
			else if (LastID < ThisID)
			{
				Register.OnSequenceExpired(LastID, Player);
			}
			else
			{
				++ThisFrameIDs;
			}
		}
		else
		{
			Register.OnSequenceExpired(LastID, Player);
		}
	}
}

void FMovieSceneRootEvaluationTemplateInstance::OnSequenceChanged()
{
	bIsDirty = true;
}
