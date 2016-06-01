// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneSpawnable.h"
#include "MovieScene.h"


void FMovieSceneSpawnable::CopyObjectTemplate(UObject& InSourceObject, UMovieScene& OwnerMovieScene)
{
	const FName ObjectName = ObjectTemplate ? ObjectTemplate->GetFName() : InSourceObject.GetFName();

	if (ObjectTemplate)
	{
		ObjectTemplate->MarkPendingKill();
		ObjectTemplate = nullptr;
	}

	ObjectTemplate = StaticDuplicateObject(&InSourceObject, &OwnerMovieScene, ObjectName, RF_AllFlags & ~(RF_Transient));
	ObjectTemplate->SetFlags(RF_ArchetypeObject);

	// @todo: this will mark the package as dirty whenever a spawnable is destroyed. We should diff the duplicated object, and only mark dirty where the spawnable has actually changed
	OwnerMovieScene.MarkPackageDirty();
}