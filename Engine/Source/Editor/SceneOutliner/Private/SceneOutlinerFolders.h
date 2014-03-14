// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once


namespace SceneOutlinerFilters
{
	class SceneOutlinerFolderMap : public TMap<FName, TSharedPtr<TOutlinerFolderTreeItem>>
	{
		void AddFolderPath(FName Name);
	};
}