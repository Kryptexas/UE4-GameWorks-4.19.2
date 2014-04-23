// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_World::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor )
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto World = Cast<UWorld>(*ObjIt);
		if (World != NULL)
		{
			const FString FileToOpen = FPackageName::LongPackageNameToFilename(World->GetOutermost()->GetName(), FPackageName::GetMapPackageExtension());
			const bool bLoadAsTemplate = false;
			const bool bShowProgress = false;
			const bool bWorldComposition = false;
			FEditorFileUtils::LoadMap( FileToOpen, bLoadAsTemplate, bShowProgress, bWorldComposition );

			// We can only edit one world at a time... so just break after the first valid world to load
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
