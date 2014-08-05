// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorial.h"
#include "EditorTutorialSettings.generated.h"

/** Track the progress of an individual tutorial */
USTRUCT()
struct FTutorialProgress
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FStringClassReference Tutorial;

	UPROPERTY()
	int32 CurrentStage;
};

/** Editor-wide tutorial settings */
UCLASS(config=EditorGameAgnostic)
class UEditorTutorialSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Categories for tutorials */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials")
	TArray<FTutorialCategory> Categories;

	/** The tutorials available in the editor */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials", meta=(MetaClass="EditorTutorial"))
	TArray<FStringClassReference> Tutorials;

	UPROPERTY(Config)
	TArray<FTutorialProgress> TutorialsProgress;

	/** Tutorial to start on Editor startup */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials", meta=(MetaClass="EditorTutorial"))
	FStringClassReference StartupTutorial;

	/** UObject interface */
	virtual void PostInitProperties() override;

	/** Get the recorded progress of the pass-in tutorial */
	int32 GetProgress(UEditorTutorial* InTutorial, bool& bOutHaveSeenTutorial) const;

	/** Check if we have seen the passed-in tutorial before */
	bool HaveSeenTutorial(UEditorTutorial* InTutorial) const;

	/** Record the progress of the passed-in tutorial */
	void RecordProgress(UEditorTutorial* InTutorial, int32 CurrentStage);

	/** Save the progress of all our tutorials */
	void SaveProgress();

	/** Recorded progress */
	TMap<UEditorTutorial*, FTutorialProgress> ProgressMap;
};