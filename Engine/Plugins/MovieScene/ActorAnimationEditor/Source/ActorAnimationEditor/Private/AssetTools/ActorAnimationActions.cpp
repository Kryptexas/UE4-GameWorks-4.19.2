// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationEditorPrivatePCH.h"
#include "IAssetTools.h"
#include "ISequencerModule.h"
#include "Toolkits/IToolkit.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


/* FActorAnimationActions constructors
 *****************************************************************************/

FActorAnimationActions::FActorAnimationActions( const TSharedRef<ISlateStyle>& InStyle )
	: Style(InStyle)
{ }


/* IAssetTypeActions interface
 *****************************************************************************/

uint32 FActorAnimationActions::GetCategories()
{
	return EAssetTypeCategories::Animation;
}


FText FActorAnimationActions::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ActorAnimation", "Actor Animation");
}


UClass* FActorAnimationActions::GetSupportedClass() const
{
	return UActorAnimation::StaticClass(); 
}


FColor FActorAnimationActions::GetTypeColor() const
{
	return FColor(200, 80, 80);
}


void FActorAnimationActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UActorAnimation* ActorAnimation = Cast<UActorAnimation>(*ObjIt);

		if (ActorAnimation != nullptr)
		{
			TSharedRef<FActorAnimationEditorToolkit> Toolkit = MakeShareable(new FActorAnimationEditorToolkit(Style));
			Toolkit->Initialize(Mode, EditWithinLevelEditor, ActorAnimation, true);
		}
	}
}


bool FActorAnimationActions::ShouldForceWorldCentric()
{
	// @todo sequencer: Hack to force world-centric mode for Sequencer
	return true;
}


#undef LOCTEXT_NAMESPACE