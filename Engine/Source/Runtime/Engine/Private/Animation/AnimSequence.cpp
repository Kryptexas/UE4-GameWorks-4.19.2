// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimSequence.cpp: Skeletal mesh animation functions.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "AnimationCompression.h"
#include "AnimEncoding.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"
#include "TargetPlatform.h"

#define USE_SLERP 0

/////////////////////////////////////////////////////
// FRawAnimSequenceTrackNativeDeprecated

//@deprecated with VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA
struct FRawAnimSequenceTrackNativeDeprecated
{
	TArray<FVector> PosKeys;
	TArray<FQuat> RotKeys;
	friend FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrackNativeDeprecated& T)
	{
		return	Ar << T.PosKeys << T.RotKeys;
	}
};

/////////////////////////////////////////////////////
// FCurveTrack

/** Returns true if valid curve weight exists in the array*/
bool FCurveTrack::IsValidCurveTrack()
{
	bool bValid = false;

	if ( CurveName != NAME_None )
	{
		for (int32 I=0; I<CurveWeights.Num(); ++I)
		{
			// it has valid weight
			if (CurveWeights[I]>KINDA_SMALL_NUMBER)
			{
				bValid = true;
				break;
			}
		}
	}

	return bValid;
}

/** This is very simple cut to 1 key method if all is same since I see so many redundant same value in every frame 
 *  Eventually this can get more complicated 
 *  Will return true if compressed to 1. Return false otherwise
 **/
bool FCurveTrack::CompressCurveWeights()
{
	// if always 1, no reason to do this
	if (CurveWeights.Num() > 1)
	{
		bool bCompress = true;
		// first weight
		float FirstWeight = CurveWeights[0];

		for (int32 I=1; I<CurveWeights.Num(); ++I)
		{
			// see if my key is same as previous
			if (fabs(FirstWeight - CurveWeights[I]) > SMALL_NUMBER)
			{
				// if not same, just get out, you don't like to compress this to 1 key
				bCompress = false;
				break;
			}
		} 

		if (bCompress)
		{
			CurveWeights.Empty();
			CurveWeights.Add(FirstWeight);
			CurveWeights.Shrink();
		}

		return bCompress;
	}

	// nothing changed
	return false;
}

/////////////////////////////////////////////////////
// UAnimSequence

UAnimSequence::UAnimSequence(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	RateScale = 1.0;
	bLoopingInterpolation = true;
}

SIZE_T UAnimSequence::GetResourceSize(EResourceSizeMode::Type Mode)
{
	const int32 ResourceSize = CompressedTrackOffsets.Num() == 0 ? GetApproxRawSize() : GetApproxCompressedSize();
	return ResourceSize;
}

int32 UAnimSequence::GetApproxRawSize() const
{
	int32 Total = sizeof(FRawAnimSequenceTrack) * RawAnimationData.Num();
	for (int32 i=0;i<RawAnimationData.Num();++i)
	{
		const FRawAnimSequenceTrack& RawTrack = RawAnimationData[i];
		Total +=
			sizeof( FVector ) * RawTrack.PosKeys.Num() +
			sizeof( FQuat ) * RawTrack.RotKeys.Num() + 
			sizeof( FVector ) * RawTrack.ScaleKeys.Num(); 
	}
	return Total;
}


int32 UAnimSequence::GetApproxReducedSize() const
{
	int32 Total =
		sizeof(FTranslationTrack) * TranslationData.Num() +
		sizeof(FRotationTrack) * RotationData.Num() + 
		sizeof(FScaleTrack) * ScaleData.Num();

	for (int32 i=0;i<TranslationData.Num();++i)
	{
		const FTranslationTrack& TranslationTrack = TranslationData[i];
		Total +=
			sizeof( FVector ) * TranslationTrack.PosKeys.Num() +
			sizeof( float ) * TranslationTrack.Times.Num();
	}

	for (int32 i=0;i<RotationData.Num();++i)
	{
		const FRotationTrack& RotationTrack = RotationData[i];
		Total +=
			sizeof( FQuat ) * RotationTrack.RotKeys.Num() +
			sizeof( float ) * RotationTrack.Times.Num();
	}

	for (int32 i=0;i<ScaleData.Num();++i)
	{
		const FScaleTrack& ScaleTrack = ScaleData[i];
		Total +=
			sizeof( FVector ) * ScaleTrack.ScaleKeys.Num() +
			sizeof( float ) * ScaleTrack.Times.Num();
	}
	return Total;
}



int32 UAnimSequence::GetApproxCompressedSize() const
{
	const int32 Total = sizeof(int32)*CompressedTrackOffsets.Num() + CompressedByteStream.Num() + CompressedScaleOffsets.GetMemorySize();
	return Total;
}

/**
 * Deserializes old compressed track formats from the specified archive.
 */
static void LoadOldCompressedTrack(FArchive& Ar, FCompressedTrack& Dst, int32 ByteStreamStride)
{
	// Serialize from the archive to a buffer.
	int32 NumBytes = 0;
	Ar << NumBytes;

	TArray<uint8> SerializedData;
	SerializedData.Empty( NumBytes );
	SerializedData.AddUninitialized( NumBytes );
	Ar.Serialize( SerializedData.GetData(), NumBytes );

	// Serialize the key times.
	Ar << Dst.Times;

	// Serialize mins and ranges.
	Ar << Dst.Mins[0] << Dst.Mins[1] << Dst.Mins[2];
	Ar << Dst.Ranges[0] << Dst.Ranges[1] << Dst.Ranges[2];
}

void UAnimSequence::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	FStripDataFlags StripFlags( Ar );
	if( !StripFlags.IsEditorDataStripped() )
	{
		Ar << RawAnimationData;
	}

	if ( Ar.IsLoading() )
	{
		// Serialize the compressed byte stream from the archive to the buffer.
		int32 NumBytes;
		Ar << NumBytes;

		TArray<uint8> SerializedData;
		SerializedData.Empty( NumBytes );
		SerializedData.AddUninitialized( NumBytes );
		Ar.Serialize( SerializedData.GetData(), NumBytes );

		// Swap the buffer into the byte stream.
		FMemoryReader MemoryReader( SerializedData, true );
		MemoryReader.SetByteSwapping( Ar.ForceByteSwapping() );

		// we must know the proper codecs to use
		AnimationFormat_SetInterfaceLinks(*this);

		// and then use the codecs to byte swap
		check( RotationCodec != NULL );
		((AnimEncoding*)RotationCodec)->ByteSwapIn(*this, MemoryReader, Ar.UE3Ver());
	}
	else if( Ar.IsSaving() || Ar.IsCountingMemory() )
	{
		// Swap the byte stream into a buffer.
		TArray<uint8> SerializedData;

		// we must know the proper codecs to use
		AnimationFormat_SetInterfaceLinks(*this);

		// and then use the codecs to byte swap
		check( RotationCodec != NULL );
		((AnimEncoding*)RotationCodec)->ByteSwapOut(*this, SerializedData, Ar.ForceByteSwapping());

		// Make sure the entire byte stream was serialized.
		//check( CompressedByteStream.Num() == SerializedData.Num() );

		// Serialize the buffer to archive.
		int32 Num = SerializedData.Num();
		Ar << Num;
		Ar.Serialize( SerializedData.GetData(), SerializedData.Num() );

		// Count compressed data.
		Ar.CountBytes( SerializedData.Num(), SerializedData.Num() );
	}

#if WITH_EDITORONLY_DATA
	// SourceFilePath and SourceFileTimestamp were moved into a subobject
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_ADDED_FBX_ASSET_IMPORT_DATA )
	{
		if ( AssetImportData == NULL )
		{
			AssetImportData = ConstructObject<UAssetImportData>(UAssetImportData::StaticClass(), this);
		}
		
		AssetImportData->SourceFilePath = SourceFilePath_DEPRECATED;
		AssetImportData->SourceFileTimestamp = SourceFileTimestamp_DEPRECATED;
		SourceFilePath_DEPRECATED = TEXT("");
		SourceFileTimestamp_DEPRECATED = TEXT("");
	}
#endif // WITH_EDITORONLY_DATA
}

int32 UAnimSequence::GetNumberOfTracks() const
{
	// this shows how many tracks are used and stored in this animation
	return TrackToSkeletonMapTable.Num();
}

void UAnimSequence::PostLoad()
{
	Super::PostLoad();

	// If RAW animation data exists, and needs to be recompressed, do so.
	if( GIsEditor && GetLinkerUE4Version() < VER_UE4_REMOVE_REDUNDANT_KEY && RawAnimationData.Num() > 0 )
	{
		// Recompress Raw Animation data w/ lossless compression.
		// If some keys have been removed, then recompress animsequence with its original compression algorithm
		CompressRawAnimData();
	}

	// Ensure notifies are sorted.
	SortNotifies();

	// No animation data is found. Warn - this should check before we check CompressedTrackOffsets size
	// Otherwise, we'll see empty data set crashing game due to no CompressedTrackOffsets
	// You can't check RawAnimationData size since it gets removed during cooking
	if ( NumFrames == 0 )
	{
		UE_LOG(LogAnimation, Warning, TEXT("No animation data exists for sequence %s (%s)"), *GetName(), (GetOuter() ? *GetOuter()->GetFullName() : *GetFullName()) );
#if WITH_EDITOR
		if( GIsEditor )
		{
			UE_LOG(LogAnimation, Warning, TEXT("Removing bad AnimSequence (%s) from %s."), *GetName(), (GetOuter() ? *GetOuter()->GetFullName() : *GetFullName()) );
		}
#endif
	}
	// @remove temp hack for fixing length
	// @todo need to fix importer/editing feature
	else if ( SequenceLength == 0.f )
	{
		ensure(NumFrames == 1);
		SequenceLength = MINIMUM_ANIMATION_LENGTH;
	}
	// Raw data exists, but missing compress animation data
	else if( CompressedTrackOffsets.Num() == 0 && RawAnimationData.Num() > 0)
	{
		if (!FPlatformProperties::HasEditorOnlyData())
		{
			// Never compress on consoles.
			UE_LOG(LogAnimation, Fatal, TEXT("No animation compression exists for sequence %s (%s)"), *GetName(), (GetOuter() ? *GetOuter()->GetFullName() : *GetFullName()) );
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("No animation compression exists for sequence %s (%s)"), *GetName(), (GetOuter() ? *GetOuter()->GetFullName() : *GetFullName()) );
			// No animation compression, recompress using default settings.
			FAnimationUtils::CompressAnimSequence(this, false, false);
		}
	}

	static bool ForcedRecompressionSetting = FAnimationUtils::GetForcedRecompressionSetting();

	// Recompress the animation if it was encoded with an old package set
	// or we are being forced to do so
	if (EncodingPkgVersion != CURRENT_ANIMATION_ENCODING_PACKAGE_VERSION ||
		ForcedRecompressionSetting)
	{
		if (FPlatformProperties::RequiresCookedData())
		{
			if (EncodingPkgVersion != CURRENT_ANIMATION_ENCODING_PACKAGE_VERSION)
			{
				// Never compress on platforms that don't support it.
				UE_LOG(LogAnimation, Fatal, TEXT("Animation compression method out of date for sequence %s"), *GetName() );
				CompressedTrackOffsets.Empty(0);
				CompressedByteStream.Empty(0);
				CompressedScaleOffsets.Empty(0);
			}
		}
		else
		{
			FAnimationUtils::CompressAnimSequence(this, true, false);
		}
	}

	// If we're in the game and compressed animation data exists, whack the raw data.
	if( IsRunningGame() )
	{
		if( RawAnimationData.Num() > 0  && CompressedTrackOffsets.Num() > 0 )
		{
#if 0//@todo.Cooker/Package...
			// Don't do this on consoles; raw animation data should have been stripped during cook!
			UE_LOG(LogAnimation, Fatal, TEXT("Cooker did not strip raw animation from sequence %s"), *GetName() );
#else
			// Remove raw animation data.
			for ( int32 TrackIndex = 0 ; TrackIndex < RawAnimationData.Num() ; ++TrackIndex )
			{
				FRawAnimSequenceTrack& RawTrack = RawAnimationData[TrackIndex];
				RawTrack.PosKeys.Empty();
				RawTrack.RotKeys.Empty();
				RawTrack.ScaleKeys.Empty();
			}
			
			RawAnimationData.Empty();
#endif
		}
	}

#if WITH_EDITORONLY_DATA
	// swap out the deprecated revert to raw compression scheme with a least destructive compression scheme
	if (GIsEditor && CompressionScheme && CompressionScheme->IsA(UDEPRECATED_AnimCompress_RevertToRaw::StaticClass()))
	{
		UE_LOG(LogAnimation, Warning, TEXT("AnimSequence %s (%s) uses the deprecated revert to RAW compression scheme. Using least destructive compression scheme instead"), *GetName(), *GetFullName());
#if 0 //@todoanim: we won't compress in this case for now 
		USkeletalMesh* DefaultSkeletalMesh = LoadObject<USkeletalMesh>(NULL, *AnimSet->BestRatioSkelMeshName.ToString(), NULL, LOAD_None, NULL);
		UAnimCompress* NewAlgorithm = ConstructObject<UAnimCompress>( UAnimCompress_LeastDestructive::StaticClass() );
		NewAlgorithm->Reduce(this, DefaultSkeletalMesh, false);
#endif
	}

	bWasCompressedWithoutTranslations = false; //@todoanim: @fixmelh : AnimRotationOnly - GetAnimSet()->bAnimRotationOnly;
#endif // WITH_EDITORONLY_DATA

	// setup the Codec interfaces
	AnimationFormat_SetInterfaceLinks(*this);

	// save track data for the case
#if WITH_EDITORONLY_DATA
	USkeleton * MySkeleton = GetSkeleton();
	if( AnimationTrackNames.Num() != TrackToSkeletonMapTable.Num() && MySkeleton!=NULL )
	{
		UE_LOG(LogAnimation, Verbose, TEXT("Fixing track names (%s) from %s."), *GetName(), *MySkeleton->GetName());

		const TArray<FBoneNode> & BoneTree = MySkeleton->GetBoneTree();
		AnimationTrackNames.Empty();
		AnimationTrackNames.AddUninitialized(TrackToSkeletonMapTable.Num());
		for (int32 I=0; I<TrackToSkeletonMapTable.Num(); ++I)
		{
			const FTrackToSkeletonMap & TrackMap = TrackToSkeletonMapTable[I];
			AnimationTrackNames[I] = MySkeleton->GetReferenceSkeleton().GetBoneName(TrackMap.BoneTreeIndex);
		}
	}
#endif

	// save track data for the case
	FixAdditiveType();

	if( IsRunningGame() )
	{
		// this probably will not show newly created animations in PIE but will show them in the game once they have been saved off
		INC_DWORD_STAT_BY( STAT_AnimationMemory, GetResourceSize(EResourceSizeMode::Exclusive) );
	}

	// Convert Notifies to new data
	if( GIsEditor && GetLinkerUE4Version() < VER_UE4_ANIMNOTIFY_NAMECHANGE && Notifies.Num() > 0 )
	{
		LOG_SCOPE_VERBOSITY_OVERRIDE(LogAnimation, ELogVerbosity::Warning);
 		// convert animnotifies
 		for (int32 I=0; I<Notifies.Num(); ++I)
 		{
 			if (Notifies[I].Notify!=NULL)
 			{
				FString Label = Notifies[I].Notify->GetClass()->GetName();
				Label = Label.Replace(TEXT("AnimNotify_"), TEXT(""), ESearchCase::CaseSensitive);
				Notifies[I].NotifyName = FName(*Label);
 			}
 		}
	}
}

void UAnimSequence::FixAdditiveType()
{
	if( GetLinkerUE4Version() < VER_UE4_ADDITIVE_TYPE_CHANGE )
	{
		if ( RefPoseType!=ABPT_None && AdditiveAnimType == AAT_None )
		{
			AdditiveAnimType = AAT_LocalSpaceBase;
			MarkPackageDirty();
		}
	}
}

void UAnimSequence::BeginDestroy()
{
	Super::BeginDestroy();

	// clear any active codec links
	RotationCodec = NULL;
	TranslationCodec = NULL;

	if( IsRunningGame() )
	{
		DEC_DWORD_STAT_BY( STAT_AnimationMemory, GetResourceSize(EResourceSizeMode::Exclusive) );
	}
}

#if WITH_EDITOR
void UAnimSequence::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(!IsTemplate())
	{
		// Make sure package is marked dirty when doing stuff like adding/removing notifies
		MarkPackageDirty();
	}

	if (AdditiveAnimType != AAT_None)
	{
		if (RefPoseType == ABPT_None)
		{
			// slate will take care of change
			RefPoseType = ABPT_RefPose;
		}
	}

	if (RefPoseSeq != NULL)
	{
		if (RefPoseSeq->GetSkeleton() != GetSkeleton()) // @todo this may require to be changed when hierarchy of skeletons is introduced
		{
			RefPoseSeq = NULL;
		}
	}
}
#endif // WITH_EDITOR

// @todo DB: Optimize!
template<typename TimeArray>
static int32 FindKeyIndex(float Time, const TimeArray& Times)
{
	int32 FoundIndex = 0;
	for ( int32 Index = 0 ; Index < Times.Num() ; ++Index )
	{
		const float KeyTime = Times(Index);
		if ( Time >= KeyTime )
		{
			FoundIndex = Index;
		}
		else
		{
			break;
		}
	}
	return FoundIndex;
}

void UAnimSequence::GetBoneTransform(FTransform& OutAtom, int32 TrackIndex, float Time, bool bLooping, bool bUseRawData) const
{
	// If the caller didn't request that raw animation data be used . . .
	if ( !bUseRawData )
	{
		if ( CompressedTrackOffsets.Num() > 0 )
		{
			AnimationFormat_GetBoneAtom( OutAtom, *this, TrackIndex, Time, bLooping );
			return;
		}
	}


	// Bail out if the animation data doesn't exists (e.g. was stripped by the cooker).
	if ( RawAnimationData.Num() == 0 )
	{
		UE_LOG(LogAnimation, Log, TEXT("UAnimSequence::GetBoneTransform : No anim data in AnimSequence!") );
		OutAtom.SetIdentity();
		return;
	}

	const FRawAnimSequenceTrack & RawTrack = RawAnimationData[TrackIndex];

	// Bail out (with rather wacky data) if data is empty for some reason.
	if( RawTrack.PosKeys.Num() == 0 || RawTrack.RotKeys.Num() == 0 )
	{
		UE_LOG(LogAnimation, Log, TEXT("UAnimSequence::GetBoneTransform : No anim data in AnimSequence!") );
		OutAtom.SetIdentity();
		return;
	}

	int32 KeyIndex1, KeyIndex2;
	float Alpha;
	FAnimationRuntime::GetKeyIndicesFromTime(KeyIndex1, KeyIndex2, Alpha, Time, bLooping, NumFrames, SequenceLength);
	// @Todo fix me: this change is not good, it has lots of branches. But we'd like to save memory for not saving scale if no scale change exists
	const bool bHasScaleKey = (RawTrack.ScaleKeys.Num() > 0);
	static const FVector DefaultScale3D = FVector(1.f);

 	if( Alpha <= 0.f )
	{
		const int32 PosKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.PosKeys.Num()-1);
		const int32 RotKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.RotKeys.Num()-1);
		if ( bHasScaleKey )
		{
			const int32 ScaleKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.ScaleKeys.Num()-1);
			OutAtom = FTransform(RawTrack.RotKeys[RotKeyIndex1], RawTrack.PosKeys[PosKeyIndex1], RawTrack.ScaleKeys[ScaleKeyIndex1]);		
		}
		else
		{
			OutAtom = FTransform(RawTrack.RotKeys[RotKeyIndex1], RawTrack.PosKeys[PosKeyIndex1], DefaultScale3D);		
		}
		return;
	}
	else if( Alpha >= 1.f )
	{
		const int32 PosKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.PosKeys.Num()-1);
		const int32 RotKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.RotKeys.Num()-1);
		if ( bHasScaleKey )
		{
			const int32 ScaleKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.ScaleKeys.Num()-1);
			OutAtom = FTransform(RawTrack.RotKeys[RotKeyIndex2], RawTrack.PosKeys[PosKeyIndex2], RawTrack.ScaleKeys[ScaleKeyIndex2]);
		}
		else
		{
			OutAtom = FTransform(RawTrack.RotKeys[RotKeyIndex2], RawTrack.PosKeys[PosKeyIndex2], DefaultScale3D);		
		}
		return;
	}

	const int32 PosKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.PosKeys.Num()-1);
	const int32 RotKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.RotKeys.Num()-1);

	const int32 PosKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.PosKeys.Num()-1);
	const int32 RotKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.RotKeys.Num()-1);
	
	FTransform KeyAtom1, KeyAtom2;
	
	if ( bHasScaleKey )
	{
		const int32 ScaleKeyIndex1 = FMath::Min(KeyIndex1, RawTrack.ScaleKeys.Num()-1);
		const int32 ScaleKeyIndex2 = FMath::Min(KeyIndex2, RawTrack.ScaleKeys.Num()-1);
		
		KeyAtom1 = FTransform(RawTrack.RotKeys[RotKeyIndex1], RawTrack.PosKeys[PosKeyIndex1], RawTrack.ScaleKeys[ScaleKeyIndex1]);	
		KeyAtom2 = FTransform(RawTrack.RotKeys[RotKeyIndex2], RawTrack.PosKeys[PosKeyIndex2], RawTrack.ScaleKeys[ScaleKeyIndex2]);
	}
	else
	{
		KeyAtom1 = FTransform(RawTrack.RotKeys[RotKeyIndex1], RawTrack.PosKeys[PosKeyIndex1], DefaultScale3D);	
		KeyAtom2 = FTransform(RawTrack.RotKeys[RotKeyIndex2], RawTrack.PosKeys[PosKeyIndex2], DefaultScale3D);	
	}

// 	UE_LOG(LogAnimation, Log, TEXT(" *  *  *  Position. PosKeyIndex1: %3d, PosKeyIndex2: %3d, Alpha: %f"), PosKeyIndex1, PosKeyIndex2, Alpha);
// 	UE_LOG(LogAnimation, Log, TEXT(" *  *  *  Rotation. RotKeyIndex1: %3d, RotKeyIndex2: %3d, Alpha: %f"), RotKeyIndex1, RotKeyIndex2, Alpha);

	OutAtom.Blend(KeyAtom1, KeyAtom2, Alpha);
	OutAtom.NormalizeRotation();
}

void UAnimSequence::ExtractRootTrack(float Pos, FTransform & RootTransform, const FBoneContainer * RequiredBones) const
{
	// we assume root is in first data if available = SkeletonIndex == 0 && BoneTreeIndex == 0)
	if ((TrackToSkeletonMapTable.Num() > 0) && (TrackToSkeletonMapTable[0].BoneTreeIndex == 0) )
	{
		// if we do have root data, then return root data
		GetBoneTransform(RootTransform, 0, Pos, false, false );
		return;
	}

	// Fallback to root bone from reference skeleton.
	if( RequiredBones )
	{
		const FReferenceSkeleton & RefSkeleton = RequiredBones->GetReferenceSkeleton();
		if( RefSkeleton.GetNum() > 0 )
		{
			RootTransform = RefSkeleton.GetRefBonePose()[0];
			return;
		}
	}

	USkeleton * MySkeleton = GetSkeleton();
	// If we don't have a RequiredBones array, get root bone from default skeleton.
	if( !RequiredBones &&  MySkeleton )
	{
		const FReferenceSkeleton RefSkeleton = MySkeleton->GetReferenceSkeleton();
		if( RefSkeleton.GetNum() > 0 )
		{
			RootTransform = RefSkeleton.GetRefBonePose()[0];
			return;
		}
	}

	// Otherwise, use identity.
	RootTransform = FTransform::Identity;
}

void UAnimSequence::GetAnimationPose(FTransformArrayA2 & OutAtoms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	// @todo anim: if compressed and baked in the future, we don't have to do this 
	if( IsValidAdditive() ) 
	{
		if (AdditiveAnimType == AAT_LocalSpaceBase)
		{
			GetBonePose_Additive(OutAtoms, RequiredBones, ExtractionContext);
		}
		else if ( AdditiveAnimType == AAT_RotationOffsetMeshSpace )
		{
			GetBonePose_AdditiveMeshRotationOnly(OutAtoms, RequiredBones, ExtractionContext);
		}
	}
	else
	{
		GetBonePose(OutAtoms, RequiredBones, ExtractionContext);
	}
}

void UAnimSequence::ResetRootBoneForRootMotion(FTransformArrayA2 & BoneTransforms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	FTransform LockedRootBone;
	switch (ExtractionContext.RootMotionRootLock)
	{
		case ERootMotionRootLock::AnimFirstFrame : ExtractRootTrack(0.f, LockedRootBone, &RequiredBones); break;
		case ERootMotionRootLock::Zero : LockedRootBone = FTransform::Identity; break;
		default:
		case ERootMotionRootLock::RefPose : LockedRootBone = RequiredBones.GetRefPoseArray()[0]; break;
	}

	// If extracting root motion translation, reset translation part to first frame of animation.
	if( ExtractionContext.bExtractRootMotionTranslation )
	{
		BoneTransforms[0].SetTranslation(LockedRootBone.GetTranslation());
	}

	// If extracting root motion rotation, reset rotation part to first frame of animation.
	if( ExtractionContext.bExtractRootMotionRotation )
	{
		BoneTransforms[0].SetRotation(LockedRootBone.GetRotation());
	}
}

void UAnimSequence::GetBonePose(FTransformArrayA2 & OutAtoms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	// Allow 'bLoopingInterpolation' flag on the AnimSequence to disable the looping interpolation.
	const bool bDoLoopingInterpolation = ExtractionContext.bLooping && bLoopingInterpolation;

	// initialize with refpose
	FAnimationRuntime::FillWithRefPose(OutAtoms, RequiredBones);

	const int32 NumTracks = GetNumberOfTracks();
	USkeleton * MySkeleton = GetSkeleton();
	if ((NumTracks == 0) || !MySkeleton)
	{
		return;
	}

	// Remap RequiredBones array to Source Skeleton.
	const FSkeletonRemappedBoneArray * RemappedBoneArray = RequiredBones.GetRemappedArrayForSkeleton(*MySkeleton);
	if( !RemappedBoneArray )
	{
		return;
	}

	const TArray<int32> & SkeletonToPoseBoneIndexArray = RemappedBoneArray->SkeletonToPoseBoneIndexArray;

	// Slower path for disable retargeting, that's only used in editor and for debugging.
	if( RequiredBones.ShouldUseRawData() || RequiredBones.GetDisableRetargeting() )
	{
		for(int32 TrackIndex=0; TrackIndex<NumTracks; TrackIndex++)
		{
			const int32 SkeletonBoneIndex = GetSkeletonIndexFromTrackIndex(TrackIndex);
			// not sure it's safe to assume that SkeletonBoneIndex can never be INDEX_NONE
			check( (SkeletonBoneIndex != INDEX_NONE) && (SkeletonBoneIndex < MAX_BONES) );
			const int32 PoseBoneIndex = SkeletonToPoseBoneIndexArray[SkeletonBoneIndex];
			if( PoseBoneIndex != INDEX_NONE )
			{
				// extract animation
				GetBoneTransform(OutAtoms[PoseBoneIndex], TrackIndex, ExtractionContext.CurrentTime, bDoLoopingInterpolation, true);

				// retarget
				// @laurent - we should look into splitting rotation and translation tracks, so we don't have to process translation twice.
				if( !RequiredBones.GetDisableRetargeting() )
				{
					RetargetBoneTransform(OutAtoms[PoseBoneIndex], SkeletonBoneIndex, PoseBoneIndex, RequiredBones);
				}
			}
		}

		if( ExtractionContext.bExtractRootMotionTranslation || ExtractionContext.bExtractRootMotionRotation )
		{
			ResetRootBoneForRootMotion(OutAtoms, RequiredBones, ExtractionContext);
		}
		return;
	}

	const TArray<FBoneNode> & BoneTree = MySkeleton->GetBoneTree();

	//@TODO:@ANIMATION: These should be memstack allocated - very heavy
	BoneTrackArray RotationScalePairs;
	BoneTrackArray TranslationPairs;
	BoneTrackArray AnimScaleRetargetingPairs;

	// build a list of desired bones
	RotationScalePairs.Empty(NumTracks);
	TranslationPairs.Empty(NumTracks);
	AnimScaleRetargetingPairs.Empty(NumTracks);

	// Optimization: assuming first index is root bone. That should always be the case in Skeletons.
	checkSlow( (SkeletonToPoseBoneIndexArray[0] == 0) );
	// this is not guaranteed for AnimSequences though... If Root is not animated, Track will not exist.
	const bool bFirstTrackIsRootBone = (GetSkeletonIndexFromTrackIndex(0) == 0);

	// Handle root bone separately if it is track 0. so we start w/ Index 1.
	for(int32 TrackIndex=(bFirstTrackIsRootBone ? 1 : 0); TrackIndex<NumTracks; TrackIndex++)
	{
		const int32 SkeletonBoneIndex = GetSkeletonIndexFromTrackIndex(TrackIndex);
		// not sure it's safe to assume that SkeletonBoneIndex can never be INDEX_NONE
		checkSlow( SkeletonBoneIndex != INDEX_NONE );
		const int32 PoseBoneIndex = SkeletonToPoseBoneIndexArray[SkeletonBoneIndex];
		if( PoseBoneIndex != INDEX_NONE )
		{
			checkSlow( PoseBoneIndex < RequiredBones.GetNumBones() );
			RotationScalePairs.Add(BoneTrackPair(PoseBoneIndex, TrackIndex));

			// Skip extracting translation component for EBoneTranslationRetargetingMode::Skeleton.
			switch( BoneTree[SkeletonBoneIndex].TranslationRetargetingMode )
			{
				case EBoneTranslationRetargetingMode::Animation : 
					TranslationPairs.Add(BoneTrackPair(PoseBoneIndex, TrackIndex));
					break;
				case EBoneTranslationRetargetingMode::AnimationScaled :
					TranslationPairs.Add(BoneTrackPair(PoseBoneIndex, TrackIndex));
					AnimScaleRetargetingPairs.Add(BoneTrackPair(PoseBoneIndex, SkeletonBoneIndex));
					break;
			}
		}
	}

	// Handle Root Bone separately
	if( bFirstTrackIsRootBone )
	{
		const int32 TrackIndex = 0;
		FTransform & RootAtom = OutAtoms[0];

		AnimationFormat_GetBoneAtom(	
			RootAtom,
			*this,
			TrackIndex,
			ExtractionContext.CurrentTime,
			bDoLoopingInterpolation );

		// @laurent - we should look into splitting rotation and translation tracks, so we don't have to process translation twice.
		RetargetBoneTransform(RootAtom, 0, 0, RequiredBones);
	}

	// get the remaining bone atoms
	AnimationFormat_GetAnimationPose(	
		*((FTransformArray*)&OutAtoms), //@TODO:@ANIMATION: Nasty hack
		RotationScalePairs,
		TranslationPairs,
		RotationScalePairs, 
		*this,
		ExtractionContext.CurrentTime,
		bDoLoopingInterpolation);

	if( ExtractionContext.bExtractRootMotionTranslation || ExtractionContext.bExtractRootMotionRotation )
	{
		ResetRootBoneForRootMotion(OutAtoms, RequiredBones, ExtractionContext);
	}

	// Anim Scale Retargeting
	const TArray<FTransform> & TargetRefPoseArray = RequiredBones.GetRefPoseArray();
	const TArray<FTransform> & SkeletonRefPoseArray = GetSkeleton()->GetRefLocalPoses(RetargetSource);
	const int32 NumBonesToRetarget = AnimScaleRetargetingPairs.Num();
	for(int32 Index=0; Index<NumBonesToRetarget; Index++)
	{
		const BoneTrackPair & BonePair = AnimScaleRetargetingPairs[Index];
		const int32 & PoseBoneIndex = BonePair.AtomIndex;
		const int32 & SkeletonBoneIndex = BonePair.TrackIndex;

		// @todo - precache that in FBoneContainer when we have SkeletonIndex->TrackIndex mapping. So we can just apply scale right away.
		const float SourceTranslationLength = SkeletonRefPoseArray[SkeletonBoneIndex].GetTranslation().Size();
		if( FMath::Abs(SourceTranslationLength) > KINDA_SMALL_NUMBER )
		{
			const float TargetTranslationLength = TargetRefPoseArray[PoseBoneIndex].GetTranslation().Size();
			OutAtoms[PoseBoneIndex].ScaleTranslation(TargetTranslationLength / SourceTranslationLength);
		}
	}
}

void UAnimSequence::GetBonePose_Additive(FTransformArrayA2 & OutAtoms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	if( !IsValidAdditive() )
	{
		FAnimationRuntime::InitializeTransform(RequiredBones, OutAtoms);
		return;
	}

	// Extract target pose
	GetBonePose(OutAtoms, RequiredBones, ExtractionContext);

	// Extract base pose
	FTransformArrayA2 BasePoseAtoms;
	GetAdditiveBasePose(BasePoseAtoms, RequiredBones, ExtractionContext);

	// Create Additive animation
	FAnimationRuntime::ConvertPoseToAdditive(OutAtoms, BasePoseAtoms, RequiredBones);
}

void UAnimSequence::GetAdditiveBasePose(FTransformArrayA2 & OutAtoms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	switch (RefPoseType)
	{
		// use whole animation as a base pose. Need BasePoseSeq.
	case ABPT_AnimScaled:	
		{
			// normalize time to fit base seq
			const float Fraction = FMath::Clamp<float>(ExtractionContext.CurrentTime / SequenceLength, 0.f, 1.f);
			const float BasePoseTime = RefPoseSeq->SequenceLength * Fraction;

			FAnimExtractContext BasePoseExtractionContext(ExtractionContext);
			BasePoseExtractionContext.CurrentTime = BasePoseTime;
			RefPoseSeq->GetBonePose(OutAtoms, RequiredBones, BasePoseExtractionContext);
			break;
		}
		// use animation as a base pose. Need BasePoseSeq and RefFrameIndex (will clamp if outside).
	case ABPT_AnimFrame:		
		{
			const float Fraction = (RefPoseSeq->NumFrames > 0) ? FMath::Clamp<float>((float)RefFrameIndex/(float)RefPoseSeq->NumFrames, 0.f, 1.f) : 0.f;
			const float BasePoseTime = RefPoseSeq->SequenceLength * Fraction;

			FAnimExtractContext BasePoseExtractionContext(ExtractionContext);
			BasePoseExtractionContext.CurrentTime = BasePoseTime;
			RefPoseSeq->GetBonePose(OutAtoms, RequiredBones, BasePoseExtractionContext);
			break;
		}
		// use ref pose of Skeleton as base
	case ABPT_RefPose:		
	default:
		FAnimationRuntime::FillWithRefPose(OutAtoms, RequiredBones);
		break;
	}
}

void UAnimSequence::GetBonePose_AdditiveMeshRotationOnly(FTransformArrayA2& OutAtoms, const FBoneContainer & RequiredBones, const FAnimExtractContext & ExtractionContext) const
{
	if( !IsValidAdditive() )
	{
		// since this is additive, need to initialize to identity
		FAnimationRuntime::InitializeTransform(RequiredBones, OutAtoms);
		return;
	}

	// Get target pose
	GetBonePose(OutAtoms, RequiredBones, ExtractionContext);

	// get base pose
	FTransformArrayA2 BasePoseAtoms;
	GetAdditiveBasePose(BasePoseAtoms, RequiredBones, ExtractionContext);

	// Convert them to mesh rotation.
	FAnimationRuntime::ConvertPoseToMeshRotation(OutAtoms, RequiredBones);
	FAnimationRuntime::ConvertPoseToMeshRotation(BasePoseAtoms, RequiredBones);

	// Turn into Additive
	FAnimationRuntime::ConvertPoseToAdditive(OutAtoms, BasePoseAtoms, RequiredBones);
}

void UAnimSequence::RetargetBoneTransform(FTransform & BoneTransform, const int32 & SkeletonBoneIndex, const int32 & PoseBoneIndex, const FBoneContainer & RequiredBones) const
{
	const USkeleton * MySkeleton = GetSkeleton();
	const TArray<FBoneNode> & BoneTree = MySkeleton->GetBoneTree();
	if( BoneTree[SkeletonBoneIndex].TranslationRetargetingMode == EBoneTranslationRetargetingMode::Skeleton )
	{
		const TArray<FTransform> & TargetRefPoseArray = RequiredBones.GetRefPoseArray();
		BoneTransform.SetTranslation( TargetRefPoseArray[PoseBoneIndex].GetTranslation() );
	}
	else if( BoneTree[SkeletonBoneIndex].TranslationRetargetingMode == EBoneTranslationRetargetingMode::AnimationScaled )
	{
		// @todo - precache that in FBoneContainer when we have SkeletonIndex->TrackIndex mapping. So we can just apply scale right away.
		const TArray<FTransform> & SkeletonRefPoseArray = GetSkeleton()->GetRefLocalPoses(RetargetSource);
		const float SourceTranslationLength = SkeletonRefPoseArray[SkeletonBoneIndex].GetTranslation().Size();
		if( FMath::Abs(SourceTranslationLength) > KINDA_SMALL_NUMBER )
		{
			const TArray<FTransform> & TargetRefPoseArray = RequiredBones.GetRefPoseArray();
			const float TargetTranslationLength = TargetRefPoseArray[PoseBoneIndex].GetTranslation().Size();
			BoneTransform.ScaleTranslation(TargetTranslationLength / SourceTranslationLength);
		}
	}
}

/** Utility function to crop data from a RawAnimSequenceTrack */
static int32 CropRawTrack(FRawAnimSequenceTrack& RawTrack, int32 StartKey, int32 NumKeys, int32 TotalNumOfFrames)
{
	check(RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == TotalNumOfFrames);
	check(RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == TotalNumOfFrames);
	// scale key can be empty
	check(RawTrack.ScaleKeys.Num() == 0 || RawTrack.ScaleKeys.Num() == 1 || RawTrack.ScaleKeys.Num() == TotalNumOfFrames);

	if( RawTrack.PosKeys.Num() > 1 )
	{
		RawTrack.PosKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.PosKeys.Num() > 0);
		RawTrack.PosKeys.Shrink();
	}

	if( RawTrack.RotKeys.Num() > 1 )
	{
		RawTrack.RotKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.RotKeys.Num() > 0);
		RawTrack.RotKeys.Shrink();
	}

	if( RawTrack.ScaleKeys.Num() > 1 )
	{
		RawTrack.ScaleKeys.RemoveAt(StartKey, NumKeys);
		check(RawTrack.ScaleKeys.Num() > 0);
		RawTrack.ScaleKeys.Shrink();
	}

	// Update NumFrames below to reflect actual number of keys.
	return FMath::Max<int32>( RawTrack.PosKeys.Num(), FMath::Max<int32>(RawTrack.RotKeys.Num(), RawTrack.ScaleKeys.Num()) );
}


bool UAnimSequence::CropRawAnimData( float CurrentTime, bool bFromStart )
{
	// Length of one frame.
	float const FrameTime = SequenceLength / ((float)NumFrames);
	// Save Total Number of Frames before crop
	int32 TotalNumOfFrames = NumFrames;

	// if current frame is 1, do not try crop. There is nothing to crop
	if ( NumFrames <= 1 )
	{
		return false;
	}
	
	// If you're end or beginning, you can't cut all nor nothing. 
	// Avoiding ambiguous situation what exactly we would like to cut 
	// Below it clamps range to 1, TotalNumOfFrames-1
	// causing if you were in below position, it will still crop 1 frame. 
	// To be clearer, it seems better if we reject those inputs. 
	// If you're a bit before/after, we assume that you'd like to crop
	if ( CurrentTime == 0.f || CurrentTime == SequenceLength )
	{
		return false;
	}

	// Find the right key to cut at.
	// This assumes that all keys are equally spaced (ie. won't work if we have dropped unimportant frames etc).
	// The reason I'm changing to TotalNumOfFrames is CT/SL = KeyIndexWithFraction/TotalNumOfFrames
	// To play TotalNumOfFrames, it takes SequenceLength. Each key will take SequenceLength/TotalNumOfFrames
	float const KeyIndexWithFraction = (CurrentTime * (float)(TotalNumOfFrames)) / SequenceLength;
	int32 KeyIndex = bFromStart ? FMath::Floor(KeyIndexWithFraction) : FMath::Ceil(KeyIndexWithFraction);
	// Ensure KeyIndex is in range.
	KeyIndex = FMath::Clamp<int32>(KeyIndex, 1, TotalNumOfFrames-1); 
	// determine which keys need to be removed.
	int32 const StartKey = bFromStart ? 0 : KeyIndex;
	int32 const NumKeys = bFromStart ? KeyIndex : TotalNumOfFrames - KeyIndex ;

	// Recalculate NumFrames
	NumFrames = TotalNumOfFrames - NumKeys;

	UE_LOG(LogAnimation, Log, TEXT("UAnimSequence::CropRawAnimData %s - CurrentTime: %f, bFromStart: %d, TotalNumOfFrames: %d, KeyIndex: %d, StartKey: %d, NumKeys: %d"), *GetName(), CurrentTime, bFromStart, TotalNumOfFrames, KeyIndex, StartKey, NumKeys);

	// Iterate over tracks removing keys from each one.
	for(int32 i=0; i<RawAnimationData.Num(); i++)
	{
		// Update NumFrames below to reflect actual number of keys while we crop the anim data
		CropRawTrack(RawAnimationData[i], StartKey, NumKeys, TotalNumOfFrames);
	}

	// Double check that everything is fine
	for(int32 i=0; i<RawAnimationData.Num(); i++)
	{
		FRawAnimSequenceTrack& RawTrack = RawAnimationData[i];
		check(RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == NumFrames);
		check(RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == NumFrames);
	}

	// Crop curve data
	{
		float CroppedStartTime = (bFromStart)? (NumKeys + StartKey)*FrameTime : 0.f;
		float CroppedEndTime = (bFromStart)? SequenceLength : (StartKey-1)*FrameTime;
		
		// make sure it doesn't go negative
		CroppedStartTime = FMath::Max<float>(CroppedStartTime, 0.f);
		CroppedEndTime = FMath::Max<float>(CroppedEndTime, 0.f);

		for ( auto CurveIter = RawCurveData.FloatCurves.CreateIterator(); CurveIter; ++CurveIter )
		{
			FFloatCurve & Curve = *CurveIter;
			// fix up curve before deleting
			// add these two values to keep the previous curve shape
			// it's possible I'm adding same values in the same location
			if (bFromStart)
			{
				// start from beginning, just add at the start time
				float EvaluationTime = CroppedStartTime; 
				float Value = Curve.FloatCurve.Eval(EvaluationTime);
				Curve.FloatCurve.AddKey(EvaluationTime, Value);
			}
			else
			{
				float EvaluationTime = CroppedEndTime; 
				float Value = Curve.FloatCurve.Eval(EvaluationTime);
				Curve.FloatCurve.AddKey(EvaluationTime, Value);
			}

			// now delete anything outside of range
			TArray<FKeyHandle> KeysToDelete;
			for (auto It(Curve.FloatCurve.GetKeyHandleIterator()); It; ++It)
			{
				FKeyHandle KeyHandle = It.Key();
				float KeyTime = Curve.FloatCurve.GetKeyTime(KeyHandle);
				// if outside of range, just delete it. 
				if ( KeyTime<CroppedStartTime || KeyTime>CroppedEndTime)
				{
					KeysToDelete.Add(KeyHandle);
				}
				// if starting from beginning, 
				// fix this up to apply delta
				else if (bFromStart)
				{
					Curve.FloatCurve.SetKeyTime(KeyHandle, KeyTime - CroppedStartTime);
				}
			}
			for (int32 i = 0; i < KeysToDelete.Num(); ++i)
			{
				Curve.FloatCurve.DeleteKey(KeysToDelete[i]);
			}
		}
	}

	// Update sequence length to match new number of frames.
	SequenceLength = (float)NumFrames * FrameTime;

	UE_LOG(LogAnimation, Log, TEXT("\tSequenceLength: %f, NumFrames: %d"), SequenceLength, NumFrames);

	MarkPackageDirty();
	return true;
}

bool UAnimSequence::CompressRawAnimSequenceTrack(FRawAnimSequenceTrack& RawTrack, float MaxPosDiff, float MaxAngleDiff)
{
	bool bRemovedKeys = false;

	// First part is to make sure we have valid input
	bool const bPosTrackIsValid = (RawTrack.PosKeys.Num() == 1 || RawTrack.PosKeys.Num() == NumFrames);
	if( !bPosTrackIsValid )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid position track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.PosKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.PosKeys.RemoveAt(1, RawTrack.PosKeys.Num()- 1);
		RawTrack.PosKeys.Shrink();
		check( RawTrack.PosKeys.Num() == 1);
	}

	bool const bRotTrackIsValid = (RawTrack.RotKeys.Num() == 1 || RawTrack.RotKeys.Num() == NumFrames);
	if( !bRotTrackIsValid )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid rotation track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.RotKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.RotKeys.RemoveAt(1, RawTrack.RotKeys.Num()- 1);
		RawTrack.RotKeys.Shrink();
		check( RawTrack.RotKeys.Num() == 1);
	}

	// scale keys can be empty, and that is valid 
	bool const bScaleTrackIsValid = (RawTrack.ScaleKeys.Num() == 0 || RawTrack.ScaleKeys.Num() == 1 || RawTrack.ScaleKeys.Num() == NumFrames);
	if( !bScaleTrackIsValid )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Found non valid Scaleation track for %s, %d frames, instead of %d. Chopping!"), *GetName(), RawTrack.ScaleKeys.Num(), NumFrames);
		bRemovedKeys = true;
		RawTrack.ScaleKeys.RemoveAt(1, RawTrack.ScaleKeys.Num()- 1);
		RawTrack.ScaleKeys.Shrink();
		check( RawTrack.ScaleKeys.Num() == 1);
	}

	// Second part is actual compression.

	// Check variation of position keys
	if( (RawTrack.PosKeys.Num() > 1) && (MaxPosDiff >= 0.0f) )
	{
		FVector FirstPos = RawTrack.PosKeys[0];
		bool bFramesIdentical = true;
		for(int32 j=1; j<RawTrack.PosKeys.Num() && bFramesIdentical; j++)
		{
			if( (FirstPos - RawTrack.PosKeys[j]).Size() > MaxPosDiff )
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if( bFramesIdentical )
		{
			bRemovedKeys = true;
			RawTrack.PosKeys.RemoveAt(1, RawTrack.PosKeys.Num()- 1);
			RawTrack.PosKeys.Shrink();
			check( RawTrack.PosKeys.Num() == 1);
		}
	}

	// Check variation of rotational keys
	if( (RawTrack.RotKeys.Num() > 1) && (MaxAngleDiff >= 0.0f) )
	{
		FQuat FirstRot = RawTrack.RotKeys[0];
		bool bFramesIdentical = true;
		for(int32 j=1; j<RawTrack.RotKeys.Num() && bFramesIdentical; j++)
		{
			if( FQuat::Error(FirstRot, RawTrack.RotKeys[j]) > MaxAngleDiff )
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if( bFramesIdentical )
		{
			bRemovedKeys = true;
			RawTrack.RotKeys.RemoveAt(1, RawTrack.RotKeys.Num()- 1);
			RawTrack.RotKeys.Shrink();
			check( RawTrack.RotKeys.Num() == 1);
		}			
	}
	
	float MaxScaleDiff = 0.0001f;

	// Check variation of Scaleition keys
	if( (RawTrack.ScaleKeys.Num() > 1) && (MaxScaleDiff >= 0.0f) )
	{
		FVector FirstScale = RawTrack.ScaleKeys[0];
		bool bFramesIdentical = true;
		for(int32 j=1; j<RawTrack.ScaleKeys.Num() && bFramesIdentical; j++)
		{
			if( (FirstScale - RawTrack.ScaleKeys[j]).Size() > MaxScaleDiff )
			{
				bFramesIdentical = false;
			}
		}

		// If all keys are the same, remove all but first frame
		if( bFramesIdentical )
		{
			bRemovedKeys = true;
			RawTrack.ScaleKeys.RemoveAt(1, RawTrack.ScaleKeys.Num()- 1);
			RawTrack.ScaleKeys.Shrink();
			check( RawTrack.ScaleKeys.Num() == 1);
		}
	}

	return bRemovedKeys;
}

bool UAnimSequence::CompressRawAnimData(float MaxPosDiff, float MaxAngleDiff)
{
	bool bRemovedKeys = false;
#if WITH_EDITORONLY_DATA

	// This removes trivial keys, and this has to happen before the removing tracks
	for(int32 i=0; i<RawAnimationData.Num(); i++)
	{
		bRemovedKeys = CompressRawAnimSequenceTrack( RawAnimationData[i], MaxPosDiff, MaxAngleDiff ) || bRemovedKeys;
	}

	const USkeleton * MySkeleton = GetSkeleton();

	if (MySkeleton)
	{
		const TArray<FTransform> & LocalRefPoses = MySkeleton->GetRefLocalPoses();
		bool bComopressScaleKeys = false;
		// go through remove keys if not needed
		for ( int32 I=0; I<RawAnimationData.Num(); ++I )
		{
			FRawAnimSequenceTrack & RawData = RawAnimationData[I];
			if ( RawData.PosKeys.Num() == 1 && RawData.RotKeys.Num() == 1 && RawData.ScaleKeys.Num() == 1 )
			{
				// if I only have 1 key, and if that is same as ref pose
				// we don't have to save those keys
				int32 BoneIndex = GetSkeletonIndexFromTrackIndex(I);
				FTransform OneKeyTransform(RawData.RotKeys[0], RawData.PosKeys[0], RawData.ScaleKeys[0]);
				if (OneKeyTransform.Equals(LocalRefPoses[BoneIndex]))
				{
					// remove this key
					RawAnimationData.RemoveAt(I);
					AnimationTrackNames.RemoveAt(I);
					TrackToSkeletonMapTable.RemoveAt(I);
					--I;
					bRemovedKeys = true;
				}
			}
			else if ( RawData.ScaleKeys.Num() > 0 )
			{
				// if scale key exists, see if we can just empty it
				bComopressScaleKeys |= (RawData.ScaleKeys.Num() > 1 || RawData.ScaleKeys[0].Equals(FVector(1.f)) == false);
			}
		}

		// if we don't have scale, we should delete all scale keys
		// if you have one track that has scale, we still should support scale, so compress scale
		if (!bComopressScaleKeys)
		{
			// then remove all scale keys
			for ( int32 I=0; I<RawAnimationData.Num(); ++I )
			{
				FRawAnimSequenceTrack & RawData = RawAnimationData[I];
				RawData.ScaleKeys.Empty();
			}
		}
	}

	// Only bother doing anything if we have some keys!
	if( NumFrames == 1 )
	{
		return bRemovedKeys;
	}

	CompressedTrackOffsets.Empty();
	CompressedScaleOffsets.Empty();
#endif
	return bRemovedKeys;
}

bool UAnimSequence::CompressRawAnimData()
{
	const float MaxPosDiff = 0.0001f;
	const float MaxAngleDiff = 0.0003f;
	return CompressRawAnimData(MaxPosDiff, MaxAngleDiff);
}

/** 
 * Flip Rotation W for the RawTrack
 */
void FlipRotationW(FRawAnimSequenceTrack& RawTrack)
{
	int32 TotalNumOfRotKey = RawTrack.RotKeys.Num();

	for (int32 I=0; I<TotalNumOfRotKey; ++I)
	{
		FQuat & RotKey = RawTrack.RotKeys[I];
		RotKey.W *= -1.f;
	}
}


void UAnimSequence::FlipRotationWForNonRoot(USkeletalMesh * SkelMesh)
{
	if (!GetSkeleton())
	{
		return;
	}

	// Now add additive animation to destination.
	for(int32 TrackIdx=0; TrackIdx<TrackToSkeletonMapTable.Num(); TrackIdx++)
	{
		// Figure out which bone this track is mapped to
		const int32 BoneIndex = TrackToSkeletonMapTable[TrackIdx].BoneTreeIndex;
		if ( BoneIndex > 0 )
		{
			FlipRotationW( RawAnimationData[TrackIdx] );

		}
	}

	// Apply compression
	FAnimationUtils::CompressAnimSequence(this, false, false);
}

void UAnimSequence::RecycleAnimSequence()
{
#if WITH_EDITORONLY_DATA
	// Clear RawAnimData
	RawAnimationData.Empty();
	TrackToSkeletonMapTable.Empty();

#endif // WITH_EDITORONLY_DATA
}
bool UAnimSequence::CopyAnimSequenceProperties(UAnimSequence* SourceAnimSeq, UAnimSequence* DestAnimSeq, bool bSkipCopyingNotifies)
{
#if WITH_EDITORONLY_DATA
	// Copy parameters
	DestAnimSeq->SequenceLength				= SourceAnimSeq->SequenceLength;
	DestAnimSeq->NumFrames					= SourceAnimSeq->NumFrames;
	DestAnimSeq->RateScale					= SourceAnimSeq->RateScale;
	DestAnimSeq->bLoopingInterpolation		= SourceAnimSeq->bLoopingInterpolation;
	DestAnimSeq->bDoNotOverrideCompression	= SourceAnimSeq->bDoNotOverrideCompression;

	// Copy Compression Settings
	DestAnimSeq->CompressionScheme				= static_cast<UAnimCompress*>( StaticDuplicateObject( SourceAnimSeq->CompressionScheme, DestAnimSeq, TEXT("None"), ~RF_RootSet ) );
	DestAnimSeq->TranslationCompressionFormat	= SourceAnimSeq->TranslationCompressionFormat;
	DestAnimSeq->RotationCompressionFormat		= SourceAnimSeq->RotationCompressionFormat;
	DestAnimSeq->AdditiveAnimType				= SourceAnimSeq->AdditiveAnimType;
	DestAnimSeq->RefPoseType					= SourceAnimSeq->RefPoseType;
	DestAnimSeq->RefPoseSeq						= SourceAnimSeq->RefPoseSeq;
	DestAnimSeq->RefFrameIndex					= SourceAnimSeq->RefFrameIndex;

	if( !bSkipCopyingNotifies )
	{
		// Copy Metadata information
		CopyNotifies(SourceAnimSeq, DestAnimSeq);
	}

	DestAnimSeq->MarkPackageDirty();

	// Copy Curve Data
	DestAnimSeq->RawCurveData = SourceAnimSeq->RawCurveData;
#endif // WITH_EDITORONLY_DATA

	return true;
}


bool UAnimSequence::CopyNotifies(UAnimSequence* SourceAnimSeq, UAnimSequence* DestAnimSeq)
{
#if WITH_EDITORONLY_DATA
	// Abort if source == destination.
	if( SourceAnimSeq == DestAnimSeq )
	{
		return true;
	}

	// If the destination sequence is shorter than the source sequence, we'll be dropping notifies that
	// occur at later times than the dest sequence is long.  Give the user a chance to abort if we
	// find any notifies that won't be copied over.
	if( DestAnimSeq->SequenceLength < SourceAnimSeq->SequenceLength )
	{
		for(int32 NotifyIndex=0; NotifyIndex<SourceAnimSeq->Notifies.Num(); ++NotifyIndex)
		{
			// If a notify is found which occurs off the end of the destination sequence, prompt the user to continue.
			const FAnimNotifyEvent& SrcNotifyEvent = SourceAnimSeq->Notifies[NotifyIndex];
			if( SrcNotifyEvent.DisplayTime > DestAnimSeq->SequenceLength )
			{
				const bool bProceed = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "SomeNotifiesWillNotBeCopiedQ", "Some notifies will not be copied because the destination sequence is not long enough.  Proceed?") );
				if( !bProceed )
				{
					return false;
				}
				else
				{
					break;
				}
			}
		}
	}

	// If the destination sequence contains any notifies, ask the user if they'd like
	// to delete the existing notifies before copying over from the source sequence.
	if( DestAnimSeq->Notifies.Num() > 0 )
	{
		const bool bDeleteExistingNotifies = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
			NSLOCTEXT("UnrealEd", "DestSeqAlreadyContainsNotifiesMergeQ", "The destination sequence already contains {0} notifies.  Delete these before copying?"), FText::AsNumber(DestAnimSeq->Notifies.Num())) );
		if( bDeleteExistingNotifies )
		{
			DestAnimSeq->Notifies.Empty();
			DestAnimSeq->MarkPackageDirty();
		}
	}

	// Do the copy.
	TArray<int32> NewNotifyIndices;
	int32 NumNotifiesThatWereNotCopied = 0;

	for(int32 NotifyIndex=0; NotifyIndex<SourceAnimSeq->Notifies.Num(); ++NotifyIndex)
	{
		const FAnimNotifyEvent& SrcNotifyEvent = SourceAnimSeq->Notifies[NotifyIndex];

		// Skip notifies which occur at times later than the destination sequence is long.
		if( SrcNotifyEvent.DisplayTime > DestAnimSeq->SequenceLength )
		{
			continue;
		}

		// Do a linear-search through existing notifies to determine where
		// to insert the new notify.
		int32 NewNotifyIndex = 0;
		while( NewNotifyIndex < DestAnimSeq->Notifies.Num()
			&& DestAnimSeq->Notifies[NewNotifyIndex].DisplayTime <= SrcNotifyEvent.DisplayTime )
		{
			++NewNotifyIndex;
		}

		// Track the location of the new notify.
		NewNotifyIndices.Add(NewNotifyIndex);

		// Create a new empty on in the array.
		DestAnimSeq->Notifies.InsertZeroed(NewNotifyIndex);

		// Copy time and comment.
		FAnimNotifyEvent& Notify = DestAnimSeq->Notifies[NewNotifyIndex];
		Notify.DisplayTime = SrcNotifyEvent.DisplayTime;
		Notify.TriggerTimeOffset = GetTriggerTimeOffsetForType( DestAnimSeq->CalculateOffsetForNotify(Notify.DisplayTime));
		Notify.NotifyName = SrcNotifyEvent.NotifyName;
		Notify.Duration = SrcNotifyEvent.Duration;

		// Copy the notify itself, and point the new one at it.
		if( SrcNotifyEvent.Notify )
		{
			DestAnimSeq->Notifies[NewNotifyIndex].Notify = static_cast<UAnimNotify*>( StaticDuplicateObject(SrcNotifyEvent.Notify, DestAnimSeq, TEXT("None"), ~RF_RootSet ) );
		}
		else
		{
			DestAnimSeq->Notifies[NewNotifyIndex].Notify = NULL;
		}

		if( SrcNotifyEvent.NotifyStateClass )
		{
			DestAnimSeq->Notifies[NewNotifyIndex].NotifyStateClass = static_cast<UAnimNotifyState*>( StaticDuplicateObject(SrcNotifyEvent.NotifyStateClass, DestAnimSeq, TEXT("None"), ~RF_RootSet ) );
		}
		else
		{
			DestAnimSeq->Notifies[NewNotifyIndex].NotifyStateClass = NULL;
		}

		// Make sure editor knows we've changed something.
		DestAnimSeq->MarkPackageDirty();
	}

	// Inform the user if some notifies weren't copied.
	if( SourceAnimSeq->Notifies.Num() > NewNotifyIndices.Num() )
	{
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format(
			NSLOCTEXT("UnrealEd", "SomeNotifiesWereNotCopiedF", "Because the destination sequence was shorter, {0} notifies were not copied."), FText::AsNumber(SourceAnimSeq->Notifies.Num() - NewNotifyIndices.Num())) );
	}
#endif // WITH_EDITORONLY_DATA

	return true;
}

void UAnimSequence::UpgradeMorphTargetCurves()
{
	// call super first, it will convert currently existing curve first
	Super::UpgradeMorphTargetCurves();

	// this also gets called for the imported data, so don't check version number
	// until we fix importing data
	for (auto CurveIter = CurveData_DEPRECATED.CreateConstIterator(); CurveIter; ++CurveIter)
	{
		const FCurveTrack & CurveTrack = (*CurveIter);

		{

			// add it first, it won't add if duplicate
			RawCurveData.AddCurveData(CurveTrack.CurveName);

			// get the last one I just added
			FFloatCurve * NewCurve = RawCurveData.GetCurveData(CurveTrack.CurveName);
			check (NewCurve);

			// since the import code change, this might not match all the time
			// but the current import should cover 0-(NumFrames-1) for whole SequenceLength
			//ensure (CurveTrack.CurveWeights.Num() == NumFrames);

			// make sure this matches
			// the way we import animation should cover 0 - (NumFrames-1) for SequenceLength;
			// if only 1 frame, it will be written in the first frame
			float TimeInterval = (NumFrames==1) ? 0.f : SequenceLength / (NumFrames-1);

			// copy all weights back
			for (int32 Iter = 0 ; Iter<CurveTrack.CurveWeights.Num(); ++Iter)
			{
				// add the new value
				NewCurve->FloatCurve.AddKey(TimeInterval*Iter, CurveTrack.CurveWeights[Iter]);
			}

			// set morph target flags on them, not editable by default
			NewCurve->SetCurveTypeFlags(ACF_DrivesMorphTarget | ACF_TriggerEvent);
		}
	}

	// now conversion is done, clear all previous data
	CurveData_DEPRECATED.Empty();
}

bool UAnimSequence::IsValidAdditive() const		
{ 
	if (AdditiveAnimType != AAT_None)
	{
		switch (RefPoseType)
		{
		case ABPT_RefPose:
			return true;
		case ABPT_AnimScaled:
			return (RefPoseSeq != NULL);
		case ABPT_AnimFrame:
			return (RefPoseSeq != NULL) && (RefFrameIndex >= 0);
		default:
			return false;
		}
	}

	return false;
}

#if WITH_EDITOR
void UAnimSequence::RemapTracksToNewSkeleton( USkeleton * NewSkeleton )
{
	// this only replaces the primary one, it doesn't replace old ones
	TArray<struct FTrackToSkeletonMap> NewTrackToSkeletonMapTable;
	NewTrackToSkeletonMapTable.Empty(AnimationTrackNames.Num());
	NewTrackToSkeletonMapTable.AddUninitialized(AnimationTrackNames.Num());
	for ( int32 Track = 0; Track < AnimationTrackNames.Num(); ++Track )
	{
		int32 BoneIndex = NewSkeleton->GetReferenceSkeleton().FindBoneIndex(AnimationTrackNames[Track]);
		NewTrackToSkeletonMapTable[Track].BoneTreeIndex = BoneIndex;
	}

	// now I have all NewTrack To Skeleton Map Table
	// I'll need to compare with old tracks and copy over if SkeletonIndex == 0
	// if SkeletonIndex != 0, we need to see if we can 
	for (int32 TableId = 0; TableId <NewTrackToSkeletonMapTable.Num(); ++TableId)
	{
		if ( ensure(TrackToSkeletonMapTable.IsValidIndex(TableId)) )
		{
			if ( NewTrackToSkeletonMapTable[TableId].BoneTreeIndex != INDEX_NONE )
			{
				TrackToSkeletonMapTable[TableId].BoneTreeIndex = NewTrackToSkeletonMapTable[TableId].BoneTreeIndex;
			}
			else
			{
				// if not found, delete the track data
				RawAnimationData.RemoveAt(TableId);
				AnimationTrackNames.RemoveAt(TableId);
				TrackToSkeletonMapTable.RemoveAt(TableId);
				NewTrackToSkeletonMapTable.RemoveAt(TableId);
				--TableId;
			}
		}
	}

	// I have to set this here in order for compression
	// that has to happen outside of this after Skeleton changes
	SetSkeleton(NewSkeleton);

	PostProcessSequence();
}

void UAnimSequence::PostProcessSequence()
{
	// if scale is too small, zero it out. Cause it hard to retarget when compress
	// inverse scale is applied to translation, and causing translation to be huge to retarget, but
	// compression can't handle that much precision. 
	for (auto Iter = RawAnimationData.CreateIterator(); Iter; ++Iter)
	{
		FRawAnimSequenceTrack& RawAnim = (*Iter);

		for (auto ScaleIter = RawAnim.ScaleKeys.CreateIterator(); ScaleIter; ++ScaleIter)
		{
			FVector & Scale3D = *ScaleIter;
			if ( FMath::IsNearlyZero(Scale3D.X) )
			{
				Scale3D.X = 0.f;
			}
			if ( FMath::IsNearlyZero(Scale3D.Y) )
			{
				Scale3D.Y = 0.f;
			}
			if ( FMath::IsNearlyZero(Scale3D.Z) )
			{
				Scale3D.Z = 0.f;
			}
		}
	}

	CompressRawAnimData();
	// Apply compression
	FAnimationUtils::CompressAnimSequence(this, false, false);
	// initialize notify track
	InitializeNotifyTrack();
	//Make sure we dont have any notifies off the end of the sequence
#if WITH_EDITOR
	ClampNotifiesAtEndOfSequence();
#endif
}

bool UAnimSequence::GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences)
{
	AnimationSequences.Add(this);

	return true;
}
#endif
/*-----------------------------------------------------------------------------
	AnimNotify & subclasses
-----------------------------------------------------------------------------*/




////////////
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

void GatherAnimSequenceStats(FOutputDevice& Ar)
{
	int32 AnimationKeyFormatNum[AKF_MAX];
	int32 TranslationCompressionFormatNum[ACF_MAX];
	int32 RotationCompressionFormatNum[ACF_MAX];
	int32 ScaleCompressionFormatNum[ACF_MAX];
	FMemory::Memzero( AnimationKeyFormatNum, AKF_MAX * sizeof(int32) );
	FMemory::Memzero( TranslationCompressionFormatNum, ACF_MAX * sizeof(int32) );
	FMemory::Memzero( RotationCompressionFormatNum, ACF_MAX * sizeof(int32) );
	FMemory::Memzero( ScaleCompressionFormatNum, ACF_MAX * sizeof(int32) );

	Ar.Logf( TEXT(" %60s, Frames,NTT,NRT, NT1,NR1, TotTrnKys,TotRotKys,Codec,ResBytes"), TEXT("Sequence Name") );
	int32 GlobalNumTransTracks = 0;
	int32 GlobalNumRotTracks = 0;
	int32 GlobalNumScaleTracks = 0;
	int32 GlobalNumTransTracksWithOneKey = 0;
	int32 GlobalNumRotTracksWithOneKey = 0;
	int32 GlobalNumScaleTracksWithOneKey = 0;
	int32 GlobalApproxCompressedSize = 0;
	int32 GlobalApproxKeyDataSize = 0;
	int32 GlobalNumTransKeys = 0;
	int32 GlobalNumRotKeys = 0;
	int32 GlobalNumScaleKeys = 0;

	for( TObjectIterator<UAnimSequence> It; It; ++It )
	{
		UAnimSequence* Seq = *It;

		int32 NumTransTracks = 0;
		int32 NumRotTracks = 0;
		int32 NumScaleTracks = 0;
		int32 TotalNumTransKeys = 0;
		int32 TotalNumRotKeys = 0;
		int32 TotalNumScaleKeys = 0;
		float TranslationKeySize = 0.0f;
		float RotationKeySize = 0.0f;
		float ScaleKeySize = 0.0f;
		int32 OverheadSize = 0;
		int32 NumTransTracksWithOneKey = 0;
		int32 NumRotTracksWithOneKey = 0;
		int32 NumScaleTracksWithOneKey = 0;

		AnimationFormat_GetStats(
			Seq, 
			NumTransTracks,
			NumRotTracks,
			NumScaleTracks,
			TotalNumTransKeys,
			TotalNumRotKeys,
			TotalNumScaleKeys,
			TranslationKeySize,
			RotationKeySize,
			ScaleKeySize, 
			OverheadSize,
			NumTransTracksWithOneKey,
			NumRotTracksWithOneKey,
			NumScaleTracksWithOneKey);

		GlobalNumTransTracks += NumTransTracks;
		GlobalNumRotTracks += NumRotTracks;
		GlobalNumScaleTracks += NumScaleTracks;
		GlobalNumTransTracksWithOneKey += NumTransTracksWithOneKey;
		GlobalNumRotTracksWithOneKey += NumRotTracksWithOneKey;
		GlobalNumScaleTracksWithOneKey += NumScaleTracksWithOneKey;

		GlobalApproxCompressedSize += Seq->GetApproxCompressedSize();
		GlobalApproxKeyDataSize += (int32)((TotalNumTransKeys * TranslationKeySize) + (TotalNumRotKeys * RotationKeySize) + (TotalNumScaleKeys * ScaleKeySize));

		GlobalNumTransKeys += TotalNumTransKeys;
		GlobalNumRotKeys += TotalNumRotKeys;
		GlobalNumScaleKeys += TotalNumScaleKeys;

		Ar.Logf(TEXT(" %60s, %3i, %3i,%3i,%3i, %3i,%3i,%3i, %10i,%10i,%10i, %s, %d"),
			*Seq->GetName(),
			Seq->NumFrames,
			NumTransTracks, NumRotTracks, NumScaleTracks,
			NumTransTracksWithOneKey, NumRotTracksWithOneKey, NumScaleTracksWithOneKey,
			TotalNumTransKeys, TotalNumRotKeys, TotalNumScaleKeys,
			*FAnimationUtils::GetAnimationKeyFormatString(static_cast<AnimationKeyFormat>(Seq->KeyEncodingFormat)),
			(int32)Seq->GetResourceSize(EResourceSizeMode::Exclusive) );
	}
	Ar.Logf( TEXT("======================================================================") );
	Ar.Logf( TEXT("Total Num Tracks: %i trans, %i rot, %i scale, %i trans1, %i rot1, %i scale1"), GlobalNumTransTracks, GlobalNumRotTracks, GlobalNumScaleTracks, GlobalNumTransTracksWithOneKey, GlobalNumRotTracksWithOneKey, GlobalNumScaleTracksWithOneKey  );
	Ar.Logf( TEXT("Total Num Keys: %i trans, %i rot, %i scale"), GlobalNumTransKeys, GlobalNumRotKeys, GlobalNumScaleKeys );

	Ar.Logf( TEXT("Approx Compressed Memory: %i bytes"), GlobalApproxCompressedSize);
	Ar.Logf( TEXT("Approx Key Data Memory: %i bytes"), GlobalApproxKeyDataSize);
}
#endif

