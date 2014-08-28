// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorial.h"
#include "EditorTutorialSettings.generated.h"

/** Named context that corresponds to a particular tutorial */
USTRUCT()
struct FTutorialContext
{
	GENERATED_USTRUCT_BODY()

	/** The context that this tutorial is used in */
	UPROPERTY(EditAnywhere, Category = "Tutorials")
	FName Context;

	/** The tutorial to use in this context to let the user know there is a tutorial available */
	UPROPERTY(EditAnywhere, Category = "Tutorials", meta = (MetaClass = "EditorTutorial"))
	FStringClassReference AttractTutorial;

	/** The tutorial to use in this context when the user chooses to launch */
	UPROPERTY(EditAnywhere, Category = "Tutorials", meta = (MetaClass = "EditorTutorial"))
	FStringClassReference LaunchTutorial;
};

/** Editor-wide tutorial settings */
UCLASS(config=EditorGameAgnostic)
class UEditorTutorialSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Categories for tutorials */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials")
	TArray<FTutorialCategory> Categories;

	/** Tutorial to start on Editor startup */
	UPROPERTY(Config, EditAnywhere, Category="Tutorials", meta=(MetaClass="EditorTutorial"))
	FStringClassReference StartupTutorial;

	/** Tutorials used in various contexts - e.g. the various asset editors */
	UPROPERTY(Config, EditAnywhere, Category = "Tutorials")
	TArray<FTutorialContext> TutorialContexts;

	/** Get the tutorial for the specified context */
	void FindTutorialsForContext(FName InContext, UEditorTutorial*& OutAttractTutorial, UEditorTutorial*& OutLaunchTutorial) const;
};