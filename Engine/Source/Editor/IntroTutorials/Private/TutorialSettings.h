// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorial.h"
#include "TutorialSettings.generated.h"

/** Per-project tutorial settings */
UCLASS(config=Editor)
class UTutorialSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Categories for tutorials */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials")
	TArray<FTutorialCategory> Categories;

	/** The tutorials available in the editor */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials", meta=(MetaClass="EditorTutorial"))
	TArray<FStringClassReference> Tutorials;

	/** Tutorial to start on project startup */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials", meta=(MetaClass="EditorTutorial"))
	FStringClassReference StartupTutorial;
};