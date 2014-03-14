// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AnimGraphPrivatePCH.h"
#include "AnimPreviewInstance.h"

UAnimPreviewInstance::UAnimPreviewInstance(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SkeletalControlAlpha = 1.0f;
}

void UAnimPreviewInstance::NativeInitializeAnimation()
{
	// Cache our play state from the previous animation otherwise set to play
	bool bCachedIsPlaying = (CurrentAsset != NULL) ? bPlaying : true;

	Super::NativeInitializeAnimation();

	SetPlaying(bCachedIsPlaying);

	SingleBoneController.TranslationMode = BMM_Replace;
	SingleBoneController.TranslationSpace = BCS_BoneSpace;

	SingleBoneController.RotationMode = BMM_Replace;
	SingleBoneController.RotationSpace = BCS_BoneSpace;
}

bool UAnimPreviewInstance::NativeEvaluateAnimation(FPoseContext& Output)
{
	Super::NativeEvaluateAnimation(Output);

	USkeletalMeshComponent* Component = GetSkelMeshComponent();

	if (Component && CurrentSkeleton)
	{
		SingleBoneController.BoneToModify.BoneIndex = RequiredBones.GetPoseBoneIndexForBoneName(SingleBoneController.BoneToModify.BoneName);
		if (SingleBoneController.BoneToModify.BoneIndex != INDEX_NONE)
		{
			FA2CSPose OutMeshPose;
			OutMeshPose.AllocateLocalPoses(RequiredBones, Output.Pose);

			// now evaluate skelcontrol
			TArray<FBoneTransform> BoneTransforms;
			SingleBoneController.EvaluateBoneTransforms(Component, RequiredBones, OutMeshPose, BoneTransforms);

			if (BoneTransforms.Num() > 0)
			{
				OutMeshPose.LocalBlendCSBoneTransforms(BoneTransforms, 1.0f);
			}

			// convert back to local @todo check this
			OutMeshPose.ConvertToLocalPoses(Output.Pose);
		}
	}

	return true;
}

/** Set SkeletalControl Alpha**/
void UAnimPreviewInstance::SetSkeletalControlAlpha(float InSkeletalControlAlpha)
{
	SkeletalControlAlpha = FMath::Clamp<float>(InSkeletalControlAlpha, 0.f, 1.f);
}

UAnimSequence* UAnimPreviewInstance::GetAnimSequence()
{
	return Cast<UAnimSequence>(CurrentAsset);
}

void UAnimPreviewInstance::RestartMontage(UAnimMontage* Montage, FName FromSection)
{
	if (Montage == CurrentAsset)
	{
		MontagePreviewType = EMPT_Normal;
		Montage_Play(Montage, PlayRate);
		if (FromSection != NAME_None)
		{
			Montage_JumpToSection(FromSection);
		}
		MontagePreview_SetLoopNormal(bLooping, Montage->GetSectionIndex(FromSection));
	}
}

void UAnimPreviewInstance::MontagePreview_SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;

	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		switch (MontagePreviewType)
		{
		case EMPT_AllSections:
			MontagePreview_SetLoopAllSections(bLooping);
			break;
		case EMPT_Normal:
		default:
			MontagePreview_SetLoopNormal(bLooping);
			break;
		}
	}
}

void UAnimPreviewInstance::MontagePreview_SetPlaying(bool bIsPlaying)
{
	bPlaying = bIsPlaying;

	if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance())
	{
		CurMontageInstance->bPlaying = bIsPlaying;
	}
	else if (bPlaying)
	{
		UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset);
		if (Montage)
		{
			switch (MontagePreviewType)
			{
			case EMPT_AllSections:
				MontagePreview_PreviewAllSections();
				break;
			case EMPT_Normal:
			default:
				MontagePreview_PreviewNormal();
				break;
			}
		}
	}
}

void UAnimPreviewInstance::MontagePreview_SetReverse(bool bInReverse)
{
	Super::SetReverse(bInReverse);

	if (FAnimMontageInstance * CurMontageInstance = GetActiveMontageInstance())
	{
		if (bReverse == (CurMontageInstance->PlayRate > 0.f))
		{
			CurMontageInstance->PlayRate *= -1.f;
		}
	}
}

void UAnimPreviewInstance::MontagePreview_Restart()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		switch (MontagePreviewType)
		{
		case EMPT_AllSections:
			MontagePreview_PreviewAllSections();
			break;
		case EMPT_Normal:
		default:
			MontagePreview_PreviewNormal();
			break;
		}
	}
}

void UAnimPreviewInstance::MontagePreview_StepForward()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		bool bWasPlaying = IsPlayingMontage() && (bLooping || bPlaying); // we need to handle non-looped case separately, even if paused during playthrough
		MontagePreview_SetReverse(false);
		if (! bWasPlaying)
		{
			if (! bLooping)
			{
				float StoppedAt = CurrentTime;
				if (! bWasPlaying)
				{
					// play montage but at last known location
					MontagePreview_Restart();
					SetPosition(StoppedAt, false);
				}
				int32 LastPreviewSectionIdx = MontagePreview_FindLastSection(MontagePreviewStartSectionIdx);
				if (FMath::Abs(CurrentTime - (Montage->CompositeSections[LastPreviewSectionIdx].StartTime + Montage->GetSectionLength(LastPreviewSectionIdx))) <= MontagePreview_CalculateStepLength())
				{
					// we're at the end, jump right to the end
					Montage_JumpToSectionsEnd(Montage->GetSectionName(LastPreviewSectionIdx));
					if (! bWasPlaying)
					{
						MontagePreview_SetPlaying(false);
					}
					return; // can't go further than beginning of this
				}
			}
			else
			{
				MontagePreview_Restart();
			}
		}
		MontagePreview_SetPlaying(true);

		// Advance a single frame, leaving it paused afterwards
		GetSkelMeshComponent()->GlobalAnimRateScale = 1.0f;
		GetSkelMeshComponent()->TickAnimation(MontagePreview_CalculateStepLength());
		MontagePreview_SetPlaying(false);
	}
}

void UAnimPreviewInstance::MontagePreview_StepBackward()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		bool bWasPlaying = IsPlayingMontage() && (bLooping || bPlaying); // we need to handle non-looped case separately, even if paused during playthrough
		MontagePreview_SetReverse(true);
		if (! bWasPlaying)
		{
			if (! bLooping)
			{
				float StoppedAt = CurrentTime;
				if (! bWasPlaying)
				{
					// play montage but at last known location
					MontagePreview_Restart();
					SetPosition(StoppedAt, false);
				}
				int32 LastPreviewSectionIdx = MontagePreview_FindLastSection(MontagePreviewStartSectionIdx);
				if (FMath::Abs(CurrentTime - (Montage->CompositeSections[LastPreviewSectionIdx].StartTime + Montage->GetSectionLength(LastPreviewSectionIdx))) <= MontagePreview_CalculateStepLength())
				{
					// special case as we could stop at the end of our last section which is also beginning of following section - we don't want to get stuck there, but be inside of our starting section
					Montage_JumpToSection(Montage->GetSectionName(LastPreviewSectionIdx));
				}
				else if (FMath::Abs(CurrentTime - Montage->CompositeSections[MontagePreviewStartSectionIdx].StartTime) <= MontagePreview_CalculateStepLength())
				{
					// we're at the end of playing backward, jump right to the end
					Montage_JumpToSectionsEnd(Montage->GetSectionName(MontagePreviewStartSectionIdx));
					if (! bWasPlaying)
					{
						MontagePreview_SetPlaying(false);
					}
					return; // can't go further than beginning of first section
				}
			}
			else
			{
				MontagePreview_Restart();
			}
		}
		MontagePreview_SetPlaying(true);

		// Advance a single frame, leaving it paused afterwards
		GetSkelMeshComponent()->GlobalAnimRateScale = 1.0f;
		GetSkelMeshComponent()->TickAnimation(MontagePreview_CalculateStepLength());
		MontagePreview_SetPlaying(false);
	}
}

float UAnimPreviewInstance::MontagePreview_CalculateStepLength()
{
	return 1.0f / 30.0f;
}

void UAnimPreviewInstance::MontagePreview_JumpToStart()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 SectionIdx = 0;
		if (MontagePreviewType == EMPT_Normal)
		{
			SectionIdx = MontagePreviewStartSectionIdx;
		}
		// TODO hack - Montage_JumpToSection requires montage being played
		bool bWasPlaying = IsPlayingMontage();
		if (! bWasPlaying)
		{
			MontagePreview_Restart();
		}
		if (bReverse)
		{
			Montage_JumpToSectionsEnd(Montage->GetSectionName(SectionIdx));
		}
		else
		{
			Montage_JumpToSection(Montage->GetSectionName(SectionIdx));
		}
		if (! bWasPlaying)
		{
			MontagePreview_SetPlaying(false);
		}
	}
}

void UAnimPreviewInstance::MontagePreview_JumpToEnd()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 SectionIdx = 0;
		if (MontagePreviewType == EMPT_Normal)
		{
			SectionIdx = MontagePreviewStartSectionIdx;
		}
		// TODO hack - Montage_JumpToSectionsEnd requires montage being played
		bool bWasPlaying = IsPlayingMontage();
		if (! bWasPlaying)
		{
			MontagePreview_Restart();
		}
		if (bReverse)
		{
			Montage_JumpToSection(Montage->GetSectionName(MontagePreview_FindLastSection(SectionIdx)));
		}
		else
		{
			Montage_JumpToSectionsEnd(Montage->GetSectionName(MontagePreview_FindLastSection(SectionIdx)));
		}
		if (! bWasPlaying)
		{
			MontagePreview_SetPlaying(false);
		}
	}
}

void UAnimPreviewInstance::MontagePreview_JumpToPreviewStart()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 SectionIdx = 0;
		if (MontagePreviewType == EMPT_Normal)
		{
			SectionIdx = MontagePreviewStartSectionIdx;
		}
		// TODO hack - Montage_JumpToSectionsEnd requires montage being played
		bool bWasPlaying = IsPlayingMontage();
		if (! bWasPlaying)
		{
			MontagePreview_Restart();
		}
		Montage_JumpToSection(Montage->GetSectionName(! bReverse? SectionIdx : MontagePreview_FindLastSection(SectionIdx)));
		if (! bWasPlaying)
		{
			MontagePreview_SetPlaying(false);
		}
	}
}

void UAnimPreviewInstance::MontagePreview_JumpToPosition(float NewPosition)
{
	SetPosition(NewPosition, false);
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		// this section will be first
		int32 NewMontagePreviewStartSectionIdx = MontagePreview_FindFirstSectionAsInMontage(Montage->GetSectionIndexFromPosition(NewPosition));
		if (MontagePreviewStartSectionIdx != NewMontagePreviewStartSectionIdx &&
			MontagePreviewType == EMPT_Normal)
		{
			MontagePreviewStartSectionIdx = NewMontagePreviewStartSectionIdx;
		}
		// setup looping to match normal playback
		MontagePreview_SetLooping(bLooping);
	}
}

void UAnimPreviewInstance::MontagePreview_RemoveBlendOut()
{
	if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance())
	{
		CurMontageInstance->DefaultBlendTimeMultiplier = 0.0f;
	}
}

void UAnimPreviewInstance::MontagePreview_PreviewNormal(int32 FromSectionIdx)
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 PreviewFromSection = FromSectionIdx;
		if (FromSectionIdx != INDEX_NONE)
		{
			MontagePreviewStartSectionIdx = MontagePreview_FindFirstSectionAsInMontage(FromSectionIdx);
		}
		else
		{
			FromSectionIdx = MontagePreviewStartSectionIdx;
			PreviewFromSection = MontagePreviewStartSectionIdx;
		}
		MontagePreviewType = EMPT_Normal;
		Montage_Play(Montage, (bReverse? -1.0f : 1.0f) * PlayRate);
		MontagePreview_SetLoopNormal(bLooping, FromSectionIdx);
		Montage_JumpToSection(Montage->GetSectionName(PreviewFromSection));
		MontagePreview_RemoveBlendOut();
		bPlaying = true;
	}
}

void UAnimPreviewInstance::MontagePreview_PreviewAllSections()
{
	UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset);
	if (Montage && Montage->SequenceLength > 0.f)
	{
		MontagePreviewType = EMPT_AllSections;
		Montage_Play(Montage, (bReverse? -1.0f : 1.0f) * PlayRate);
		MontagePreview_SetLoopAllSections(bLooping);
		MontagePreview_JumpToPreviewStart();
		MontagePreview_RemoveBlendOut();
		bPlaying = true;
	}
}

void UAnimPreviewInstance::MontagePreview_SetLoopNormal(bool bIsLooping, int32 PreferSectionIdx)
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		MontagePreview_ResetSectionsOrder();

		if (PreferSectionIdx == INDEX_NONE)
		{
			PreferSectionIdx = Montage->GetSectionIndexFromPosition(CurrentTime);
		}
		int32 TotalSection = Montage->CompositeSections.Num();
		if (TotalSection > 0)
		{
			int PreferedInChain = TotalSection;
			TArray<bool> AlreadyUsed;
			AlreadyUsed.AddZeroed(TotalSection);
			while (true)
			{
				// find first not already used section
				int32 NotUsedIdx = 0;
				while (NotUsedIdx < TotalSection)
				{
					if (! AlreadyUsed[NotUsedIdx])
					{
						break;
					}
					++ NotUsedIdx;
				}
				if (NotUsedIdx >= TotalSection)
				{
					break;
				}
				// find if this is one we're looking for closest to starting one
				int32 CurSectionIdx = NotUsedIdx;
				int32 InChain = 0;
				while (true)
				{
					// find first that contains this
					if (CurSectionIdx == PreferSectionIdx &&
						InChain < PreferedInChain)
					{
						PreferedInChain = InChain;
						PreferSectionIdx = NotUsedIdx;
					}
					AlreadyUsed[CurSectionIdx] = true;
					FName NextSection = Montage->CompositeSections[CurSectionIdx].NextSectionName;
					CurSectionIdx = Montage->GetSectionIndex(NextSection);
					if (CurSectionIdx == INDEX_NONE || AlreadyUsed[CurSectionIdx]) // break loops
					{
						break;
					}
					++ InChain;
				}
				// loop this section
				SetMontageLoop(Montage, bIsLooping, Montage->CompositeSections[NotUsedIdx].SectionName);
			}
			if (Montage->CompositeSections.IsValidIndex(PreferSectionIdx))
			{
				SetMontageLoop(Montage, bIsLooping, Montage->CompositeSections[PreferSectionIdx].SectionName);
			}
		}
	}
}

void UAnimPreviewInstance::MontagePreview_SetLoopAllSetupSections(bool bIsLooping)
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		MontagePreview_ResetSectionsOrder();

		int32 TotalSection = Montage->CompositeSections.Num();
		if (TotalSection > 0)
		{
			FName FirstSection = Montage->CompositeSections[0].SectionName;
			FName PreviousSection = FirstSection;
			TArray<bool> AlreadyUsed;
			AlreadyUsed.AddZeroed(TotalSection);
			while (true)
			{
				// find first not already used section
				int32 NotUsedIdx = 0;
				while (NotUsedIdx < TotalSection)
				{
					if (! AlreadyUsed[NotUsedIdx])
					{
						break;
					}
					++ NotUsedIdx;
				}
				if (NotUsedIdx >= TotalSection)
				{
					break;
				}
				// go through all connected to join them into one big chain
				int CurSectionIdx = NotUsedIdx;
				while (true)
				{
					AlreadyUsed[CurSectionIdx] = true;
					FName CurrentSection = Montage->CompositeSections[CurSectionIdx].SectionName;
					Montage_SetNextSection(PreviousSection, CurrentSection);
					PreviousSection = CurrentSection;

					FName NextSection = Montage->CompositeSections[CurSectionIdx].NextSectionName;
					CurSectionIdx = Montage->GetSectionIndex(NextSection);
					if (CurSectionIdx == INDEX_NONE || AlreadyUsed[CurSectionIdx]) // break loops
					{
						break;
					}
				}
			}
			if (bIsLooping)
			{
				// and loop all
				Montage_SetNextSection(PreviousSection, FirstSection);
			}
		}
	}
}

void UAnimPreviewInstance::MontagePreview_SetLoopAllSections(bool bIsLooping)
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 TotalSection = Montage->CompositeSections.Num();
		if (TotalSection > 0)
		{
			if (bIsLooping)
			{
				for (int i = 0; i < TotalSection; ++ i)
				{
					Montage_SetNextSection(Montage->CompositeSections[i].SectionName, Montage->CompositeSections[(i+1) % TotalSection].SectionName);
				}
			}
			else
			{
				for (int i = 0; i < TotalSection - 1; ++ i)
				{
					Montage_SetNextSection(Montage->CompositeSections[i].SectionName, Montage->CompositeSections[i+1].SectionName);
				}
				Montage_SetNextSection(Montage->CompositeSections[TotalSection - 1].SectionName, NAME_None);
			}
		}
	}
}

void UAnimPreviewInstance::MontagePreview_ResetSectionsOrder()
{
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		int32 TotalSection = Montage->CompositeSections.Num();
		// restore to default
		for (int i = 0; i < TotalSection; ++ i)
		{
			Montage_SetNextSection(Montage->CompositeSections[i].SectionName, Montage->CompositeSections[i].NextSectionName);
		}
	}
}

int32 UAnimPreviewInstance::MontagePreview_FindFirstSectionAsInMontage(int32 ForSectionIdx)
{
	int32 ResultIdx = ForSectionIdx;
	// Montage does not have looping set up, so it should be valid and it gets
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		TArray<bool> AlreadyVisited;
		AlreadyVisited.AddZeroed(Montage->CompositeSections.Num());
		bool bFoundResult = false;
		while (! bFoundResult)
		{
			int32 UnusedSectionIdx = INDEX_NONE;
			for (int32 Idx = 0; Idx < Montage->CompositeSections.Num(); ++ Idx)
			{
				if (! AlreadyVisited[Idx])
				{
					UnusedSectionIdx = Idx;
					break;
				}
			}
			if (UnusedSectionIdx == INDEX_NONE)
			{
				break;
			}
			// check if this has ForSectionIdx
			int32 CurrentSectionIdx = UnusedSectionIdx;
			while (CurrentSectionIdx != INDEX_NONE && ! AlreadyVisited[CurrentSectionIdx])
			{
				if (CurrentSectionIdx == ForSectionIdx)
				{
					ResultIdx = UnusedSectionIdx;
					bFoundResult = true;
					break;
				}
				AlreadyVisited[CurrentSectionIdx] = true;
				FName NextSection = Montage->CompositeSections[CurrentSectionIdx].NextSectionName;
				CurrentSectionIdx = Montage->GetSectionIndex(NextSection);
			}
		}
	}
	return ResultIdx;
}

int32 UAnimPreviewInstance::MontagePreview_FindLastSection(int32 StartSectionIdx)
{
	int32 ResultIdx = StartSectionIdx;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(CurrentAsset))
	{
		if (FAnimMontageInstance* CurMontageInstance = GetActiveMontageInstance())
		{
			int32 TotalSection = Montage->CompositeSections.Num();
			if (TotalSection > 0)
			{
				TArray<bool> AlreadyVisited;
				AlreadyVisited.AddZeroed(TotalSection);
				int32 CurrentSectionIdx = StartSectionIdx;
				while (CurrentSectionIdx != INDEX_NONE && ! AlreadyVisited[CurrentSectionIdx])
				{
					AlreadyVisited[CurrentSectionIdx] = true;
					ResultIdx = CurrentSectionIdx;
					if (CurMontageInstance->NextSections.IsValidIndex(CurrentSectionIdx))
					{
						CurrentSectionIdx = CurMontageInstance->NextSections[CurrentSectionIdx];
					}
				}
			}
		}
	}
	return ResultIdx;
}
