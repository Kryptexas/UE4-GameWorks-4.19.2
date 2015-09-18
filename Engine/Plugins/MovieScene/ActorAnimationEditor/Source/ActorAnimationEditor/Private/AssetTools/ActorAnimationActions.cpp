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
	UWorld* WorldContext = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			WorldContext = Context.World();
			break;
		}
	}

	if (!ensure(WorldContext))
	{
		return;
	}

	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UActorAnimation* ActorAnimation = Cast<UActorAnimation>(*ObjIt);

		if (ActorAnimation != nullptr)
		{
			// Legacy upgrade
			ActorAnimation->ConvertPersistentBindingsToDefault(WorldContext);

			// Create an edit instance for this actor animation that can only edit the default bindings in the current world
			UActorAnimationInstance* Instance = NewObject<UActorAnimationInstance>(GetTransientPackage());
			bool bCanInstanceBindings = false;
			Instance->Initialize(ActorAnimation, WorldContext, bCanInstanceBindings);

			TSharedRef<FActorAnimationEditorToolkit> Toolkit = MakeShareable(new FActorAnimationEditorToolkit(Style));
			Toolkit->Initialize(Mode, EditWithinLevelEditor, Instance, true);
		}
	}
}


bool FActorAnimationActions::ShouldForceWorldCentric()
{
	// @todo sequencer: Hack to force world-centric mode for Sequencer
	return true;
}


#undef LOCTEXT_NAMESPACE