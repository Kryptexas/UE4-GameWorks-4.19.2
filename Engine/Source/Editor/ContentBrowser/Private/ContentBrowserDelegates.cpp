// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserDelegates.h"
#include "ContentBrowserUtils.h"

FMovedContentFolder::FMovedContentFolder(const FString& InOldPath, const FString& InNewPath)
	: OldPath(InOldPath)
	, NewPath(InNewPath)
	, Flags(ContentBrowserUtils::IsFavoriteFolder(OldPath) ? EMovedContentFolderFlags::Favorite : EMovedContentFolderFlags::None)
{
	OldPath = InOldPath;
	NewPath = InNewPath;
}