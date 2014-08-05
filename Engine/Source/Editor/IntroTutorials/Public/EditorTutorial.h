// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorial.generated.h"

/** The type of tutorial content to display */
UENUM()
namespace ETutorialContent
{
	enum Type
	{
		/** Blank - displays no content */
		None,

		/** Text content */
		Text,

		/** Content from a UDN excerpt */
		UDNExcerpt,
	};
}

/** The type of tutorial content to display */
UENUM()
namespace ETutorialAnchorIdentifier
{
	enum Type
	{
		/** No anchor */
		None,

		/** Uses a tutorial wrapper widget */
		NamedWidget,

		/** An asset accessible via the content browser */
		Asset,
	};
}

/** Category description */
USTRUCT()
struct INTROTUTORIALS_API FTutorialCategory
{
	GENERATED_USTRUCT_BODY()

	/** Period-separated category name, e.g. "Editor Quickstart.Level Editor" */
	UPROPERTY(EditAnywhere, Category="Content")
	FString Identifier;

	/** Title of the category */
	UPROPERTY(EditAnywhere, Category="Content")
	FText Title;

	/** Localized text to use to describe this category */
	UPROPERTY(EditAnywhere, Category="Content", meta=(MultiLine=true))
	FText Description;

	/** Icon for this tutorial, used when presented to the user */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	FString Icon;
};

/** Content wrapper */
USTRUCT()
struct INTROTUTORIALS_API FTutorialContent
{
	GENERATED_USTRUCT_BODY()

	/** The type of this content */
	UPROPERTY(EditAnywhere, Category="Content")
	TEnumAsByte<ETutorialContent::Type> Type;

	/** Content reference string, path etc. */
	UPROPERTY(EditAnywhere, Category="Content")
	FString Content;

	/** Excerpt name for UDN excerpt */
	UPROPERTY(EditAnywhere, Category="Content")
	FString ExcerptName;

	/** Localized text to use with this content */
	UPROPERTY(EditAnywhere, Category="Content", meta=(MultiLine=true))
	FText Text;
};

/** A way of identifying something to be highlighted by a tutorial */
USTRUCT()
struct INTROTUTORIALS_API FTutorialContentAnchor
{
	GENERATED_USTRUCT_BODY()
	
	FTutorialContentAnchor()
	{
		 Type = ETutorialAnchorIdentifier::None;
	}

	UPROPERTY(EditAnywhere, Category="Anchor")
	TEnumAsByte<ETutorialAnchorIdentifier::Type> Type;

	/** If widget is in a wrapper widget, this is the wrapper widget name */
	UPROPERTY(EditAnywhere, Category="Anchor")
	FName WrapperIdentifier;

	/** If reference is an asset, we use this to resolve it */
	UPROPERTY(EditAnywhere, Category="Anchor")
	FStringAssetReference Asset;
};

/** Content that is displayed relative to a widget */
USTRUCT()
struct INTROTUTORIALS_API FTutorialWidgetContent
{
	GENERATED_USTRUCT_BODY()

	FTutorialWidgetContent()
	{
		ContentWidth = 350.0f;
		HorizontalAlignment = HAlign_Center;
		VerticalAlignment = VAlign_Bottom;
	}

	/** Anchor for content widget to highlight */
	UPROPERTY(EditAnywhere, Category="Widget")
	FTutorialContentAnchor WidgetAnchor;

	UPROPERTY(EditAnywhere, Category="Widget")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	UPROPERTY(EditAnywhere, Category="Widget")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** Custom offset from widget */
	UPROPERTY(EditAnywhere, Category="Widget")
	FVector2D Offset;

	/** Content width - text will be wrapped at this point */
	UPROPERTY(EditAnywhere, Category="Widget", meta=(UIMin="10.0", UIMax="600.0"))
	float ContentWidth;

	/** Content to associate with widget */
	UPROPERTY(EditAnywhere, EditInline, Category="Widget")
	FTutorialContent Content;
};

/** A single tutorial stage, containing the optional main content & a number of widgets with content attached */
USTRUCT()
struct INTROTUTORIALS_API FTutorialStage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category="Stage")
	FName Name;

	UPROPERTY(EditAnywhere, EditInline, Category="Stage")
	FTutorialContent Content;

	UPROPERTY(EditAnywhere, EditInline, Category="Stage")
	TArray<FTutorialWidgetContent> WidgetContent;
};

/** An asset used to build a stage-by-stage tutorial in the editor */
UCLASS(Blueprintable, EditInlineNew)
class INTROTUTORIALS_API UEditorTutorial : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Title of this tutorial, used when presented to the user */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	FText Title;

	/** Icon for this tutorial, used when presented to the user */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	FString Icon;

	/** Category of this tutorial, used to organize tutorials when presented to the user */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	FString Category;

	/** Content to be displayed for this tutorial when presented to the user in summary */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	FTutorialContent SummaryContent;

	/** The various stages of this tutorial */
	UPROPERTY(EditAnywhere, EditInline, Category="Stages")
	TArray<FTutorialStage> Stages;

	/** Tutorial to optionally chain onto after this tutorial completes */
	UPROPERTY(EditAnywhere, Category="Tutorial", meta=(MetaClass="EditorTutorial"))
	FStringClassReference NextTutorial;

	/** A standalone tutorial displays no navigation buttons and each content widget has a close button */
	UPROPERTY(EditAnywhere, Category="Tutorial")
	bool bIsStandalone;

	/** Event fired when a tutorial stage begins */
	UFUNCTION(BlueprintImplementableEvent, Category="Tutorial")
	void OnTutorialStageStarted(FName StageName) const;

	/** Event fired when a tutorial stage ends */
	UFUNCTION(BlueprintImplementableEvent, Category="Tutorial")
	void OnTutorialStageCompleted(FName StageName) const;

	/** Advance to the next stage of a tutorial */
	UFUNCTION(BlueprintCallable, Category="Tutorial")
	static void GotoNextTutorialStage();

	/** Advance to the previous stage of a tutorial */
	UFUNCTION(BlueprintCallable, Category="Tutorial")
	static void GotoPreviousTutorialStage();

	/** Begin a tutorial. Note that this will end the current tutorial that is in progress, if any */
	UFUNCTION(BlueprintCallable, Category="Tutorial")
	static void BeginTutorial(UEditorTutorial* TutorialToStart);
};