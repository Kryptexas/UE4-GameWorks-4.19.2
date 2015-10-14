// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSkeletalAnimationTrack.h"
#include "ScopedTransaction.h"
#include "ISequencerObjectChangeListener.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneTrackEditor.h"
#include "SkeletalAnimationTrackEditor.h"
#include "MovieSceneSkeletalAnimationSection.h"
#include "CommonMovieSceneTools.h"
#include "AssetRegistryModule.h"
#include "Animation/SkeletalMeshActor.h"
#include "ContentBrowserModule.h"


namespace SkeletalAnimationEditorConstants
{
	// @todo Sequencer Allow this to be customizable
	const uint32 AnimationTrackHeight = 20;
}


FSkeletalAnimationSection::FSkeletalAnimationSection( UMovieSceneSection& InSection )
	: Section( InSection )
{
}


FSkeletalAnimationSection::~FSkeletalAnimationSection()
{
}


UMovieSceneSection* FSkeletalAnimationSection::GetSectionObject()
{ 
	return &Section;
}


FText FSkeletalAnimationSection::GetDisplayName() const
{
	return NSLOCTEXT("FAnimationSection", "AnimationSection", "Animation");
}


FText FSkeletalAnimationSection::GetSectionTitle() const
{
	return FText::FromString( Cast<UMovieSceneSkeletalAnimationSection>(&Section)->GetAnimSequence()->GetName() );
}


float FSkeletalAnimationSection::GetSectionHeight() const
{
	return (float)SkeletalAnimationEditorConstants::AnimationTrackHeight;
}


int32 FSkeletalAnimationSection::OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const
{
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	UMovieSceneSkeletalAnimationSection* AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(&Section);
	
	FTimeToPixel TimeToPixelConverter( AllottedGeometry, TRange<float>( Section.GetStartTime(), Section.GetEndTime() ) );

	// Add a box for the section
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
		SectionClippingRect,
		DrawEffects,
		FLinearColor(0.7f, 0.4f, 0.7f, 1.f)
	);

	// Darken the part that doesn't have animation
	if (AnimSection->GetAnimationStartTime() > AnimSection->GetStartTime())
	{
		float StartDarkening = AnimSection->GetStartTime();
		float EndDarkening = FMath::Min(AnimSection->GetAnimationStartTime(), AnimSection->GetEndTime());
		
		float StartPixels = TimeToPixelConverter.TimeToPixel(StartDarkening);
		float EndPixels = TimeToPixelConverter.TimeToPixel(EndDarkening);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(StartPixels, 0), FVector2D(EndPixels - StartPixels, AllottedGeometry.Size.Y)),
			FEditorStyle::GetBrush("WhiteTexture"),
			SectionClippingRect,
			DrawEffects,
			FLinearColor(0.f, 0.f, 0.f, 0.3f)
		);
	}

	// Add lines where the animation starts and ends/loops
	float CurrentTime = AnimSection->GetAnimationStartTime();
	while (CurrentTime < AnimSection->GetEndTime() && !FMath::IsNearlyZero(AnimSection->GetAnimationDuration()))
	{
		if (CurrentTime > AnimSection->GetStartTime())
		{
			float CurrentPixels = TimeToPixelConverter.TimeToPixel(CurrentTime);

			TArray<FVector2D> Points;
			Points.Add(FVector2D(CurrentPixels, 0));
			Points.Add(FVector2D(CurrentPixels, AllottedGeometry.Size.Y));

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(),
				Points,
				SectionClippingRect,
				DrawEffects
			);
		}
		CurrentTime += AnimSection->GetAnimationDuration();
	}

	return LayerId+3;
}


FSkeletalAnimationTrackEditor::FSkeletalAnimationTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
}


FSkeletalAnimationTrackEditor::~FSkeletalAnimationTrackEditor()
{
}


TSharedRef<ISequencerTrackEditor> FSkeletalAnimationTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FSkeletalAnimationTrackEditor( InSequencer ) );
}


bool FSkeletalAnimationTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneSkeletalAnimationTrack::StaticClass();
}


TSharedRef<ISequencerSection> FSkeletalAnimationTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );
	
	return MakeShareable( new FSkeletalAnimationSection(SectionObject) );
}


void FSkeletalAnimationTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	UAnimSequence* AnimSequence = Cast<UAnimSequence>(AdditionalAsset);

	if (AnimSequence)
	{
		TArray<UObject*> OutObjects;
		GetSequencer()->GetRuntimeObjects( GetSequencer()->GetFocusedMovieSceneSequenceInstance(), ObjectGuid, OutObjects);

		AnimatablePropertyChanged( UMovieSceneSkeletalAnimationTrack::StaticClass(),
			FOnKeyProperty::CreateRaw( this, &FSkeletalAnimationTrackEditor::AddKeyInternal, OutObjects, AnimSequence) );
	}
}


bool FSkeletalAnimationTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	if (Asset->IsA<UAnimSequence>())
	{
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(Asset);
		
		if (TargetObjectGuid.IsValid())
		{
			USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(TargetObjectGuid);

			if (Skeleton && Skeleton == AnimSequence->GetSkeleton())
			{
				TArray<UObject*> OutObjects;
				GetSequencer()->GetRuntimeObjects(GetSequencer()->GetFocusedMovieSceneSequenceInstance(), TargetObjectGuid, OutObjects);

				AnimatablePropertyChanged(UMovieSceneSkeletalAnimationTrack::StaticClass(),
					FOnKeyProperty::CreateRaw(this, &FSkeletalAnimationTrackEditor::AddKeyInternal, OutObjects, AnimSequence));

				return true;
			}
		}
	}
	return false;
}


void FSkeletalAnimationTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	if (ObjectClass->IsChildOf(ASkeletalMeshActor::StaticClass()))
	{
		const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();

		USkeleton* Skeleton = AcquireSkeletonFromObjectGuid(ObjectBinding);

		if (Skeleton)
		{
			// Load the asset registry module
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			// Collect a full list of assets with the specified class
			TArray<FAssetData> AssetDataList;
			AssetRegistryModule.Get().GetAssetsByClass(UAnimSequence::StaticClass()->GetFName(), AssetDataList);

			if (AssetDataList.Num())
			{
				MenuBuilder.AddSubMenu(
					NSLOCTEXT("Sequencer", "AddAnimation", "Animation"), NSLOCTEXT("Sequencer", "AddAnimationTooltip", "Adds an animation track."),
					FNewMenuDelegate::CreateRaw(this, &FSkeletalAnimationTrackEditor::BuildAnimationSubMenu, ObjectBinding, Skeleton));
			}
		}
	}
}

void FSkeletalAnimationTrackEditor::BuildAnimationSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, USkeleton* Skeleton)
{
	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw( this, &FSkeletalAnimationTrackEditor::OnAnimationAssetSelected, ObjectBinding);
	AssetPickerConfig.bAllowNullSelection = false;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;

	// Filter config
	AssetPickerConfig.Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
	AssetPickerConfig.Filter.TagsAndValues.Add(TEXT("Skeleton"), FAssetData(Skeleton).GetExportTextName());

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	TSharedPtr<SBox> MenuEntry = SNew(SBox)
	.WidthOverride(300.0f)
	.HeightOverride(300.f)
	[
		ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
	];
	MenuBuilder.AddWidget(MenuEntry.ToSharedRef(), FText::GetEmpty(), true);
}

void FSkeletalAnimationTrackEditor::OnAnimationAssetSelected(const FAssetData& AssetData, FGuid ObjectBinding)
{
	UObject* SelectedObject = AssetData.GetAsset();
	if (SelectedObject && SelectedObject->IsA(UAnimSequence::StaticClass()))
	{
		UAnimSequence* AnimSequence = CastChecked<UAnimSequence>(AssetData.GetAsset());

		AddKey(ObjectBinding, AnimSequence);
	}
}

void FSkeletalAnimationTrackEditor::AddKeyInternal( float KeyTime, const TArray<UObject*> Objects, class UAnimSequence* AnimSequence )
{
	for( int32 ObjectIndex = 0; ObjectIndex < Objects.Num(); ++ObjectIndex )
	{
		UObject* Object = Objects[ObjectIndex];

		FGuid ObjectHandle = FindOrCreateHandleToObject( Object );
		if (ObjectHandle.IsValid())
		{
			UMovieSceneTrack* Track = GetTrackForObject( ObjectHandle, UMovieSceneSkeletalAnimationTrack::StaticClass(), FName("Animation"));

			if (ensure(Track))
			{
				Cast<UMovieSceneSkeletalAnimationTrack>(Track)->AddNewAnimation( KeyTime, AnimSequence );
			}
		}
	}
}


USkeleton* FSkeletalAnimationTrackEditor::AcquireSkeletonFromObjectGuid(const FGuid& Guid)
{
	TArray<UObject*> OutObjects;
	GetSequencer()->GetRuntimeObjects(GetSequencer()->GetFocusedMovieSceneSequenceInstance(), Guid, OutObjects);

	USkeleton* Skeleton = NULL;
	for (int32 i = 0; i < OutObjects.Num(); ++i)
	{
		AActor* Actor = Cast<AActor>(OutObjects[i]);

		if (Actor != NULL)
		{
			TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			Actor->GetComponents(SkeletalMeshComponents);

			for (int32 j = 0; j <SkeletalMeshComponents.Num(); ++j)
			{
				USkeletalMeshComponent* SkeletalMeshComp = SkeletalMeshComponents[j];
				if (SkeletalMeshComp->SkeletalMesh && SkeletalMeshComp->SkeletalMesh->Skeleton)
				{
					// @todo Multiple actors, multiple components
					check(!Skeleton);
					Skeleton = SkeletalMeshComp->SkeletalMesh->Skeleton;
				}
			}
		}
	}

	return Skeleton;
}
