// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

/* Dependencies
*****************************************************************************/
#include "Engine.h"
#include "Messaging.h"
#include "ModuleManager.h"
#include "SlateBasics.h"
#include "SlateStyle.h"
#include "EditorStyle.h"

/* Private includes
*****************************************************************************/
#include "ILogVisualizer.h"
#include "VisualLogger/VisualLogger.h"
#include "LogVisualizerSettings.h"
#include "LogVisualizerSessionSettings.h"

DECLARE_DELEGATE_OneParam(FOnItemSelectionChanged, const FVisualLogDevice::FVisualLogEntryItem&);
DECLARE_DELEGATE_OneParam(FOnObjectSelectionChanged, TSharedPtr<class STimeline>);
DECLARE_DELEGATE_OneParam(FOnFiltersSearchChanged, const FText&);
DECLARE_DELEGATE(FOnFiltersChanged);

struct FVisualLoggerEvents
{
	FVisualLoggerEvents() 
	{
	
	}

	FOnItemSelectionChanged OnItemSelectionChanged;
	FOnFiltersChanged OnFiltersChanged;
	FOnObjectSelectionChanged OnObjectSelectionChanged;
};

struct IVisualLoggerInterface
{
	virtual ~IVisualLoggerInterface() {}
	virtual bool HasValidCategories(TArray<FVisualLoggerCategoryVerbosityPair> Categories) = 0;
	virtual bool IsValidCategory(const FString& InCategoryName, TEnumAsByte<ELogVerbosity::Type> Verbosity = ELogVerbosity::All) = 0;
	virtual bool IsValidCategory(const FString& InGraphName, const FString& InDataName, TEnumAsByte<ELogVerbosity::Type> Verbosity = ELogVerbosity::All) = 0;
};

class FSequencerTimeSliderController;
struct LOGVISUALIZER_API FLogVisualizer
{
	/** LogVisualizer interface*/
	void Goto(float Timestamp, FName LogOwner = NAME_None);
	void GotoNextItem();
	void GotoPreviousItem();

	FLinearColor GetColorForCategory(int32 Index) const;
	FLinearColor GetColorForCategory(const FString& InFilterName) const;
	TSharedPtr<FSequencerTimeSliderController> GetTimeSliderController() { return TimeSliderController; }
	UWorld* GetWorld(UObject* OptionalObject = nullptr);
	class AActor* GetVisualLoggerHelperActor();
	FVisualLoggerEvents& GetVisualLoggerEvents() { return VisualLoggerEvents; }

	TSharedPtr<IVisualLoggerInterface> GetVisualLoggerInterface() { return VisualLoggerInterface.Pin(); }
	void SetVisualLoggerInterface(TSharedPtr<IVisualLoggerInterface> InVisualLoggerInterface) { VisualLoggerInterface = InVisualLoggerInterface; }
	void SetCurrentVisualizer(TSharedPtr<class SVisualLogger> Visualizer) { CurrentVisualizer = Visualizer; }

	void OnObjectSelectionChanged(TSharedPtr<class STimeline> TimeLine) { CurrentTimeLine = TimeLine; }

	/** Static access */
	static void Initialize();
	static void Shutdown();
	static FLogVisualizer& Get();
protected:
	static TSharedPtr< struct FLogVisualizer > StaticInstance;
	
	TSharedPtr<FSequencerTimeSliderController> TimeSliderController;
	FVisualLoggerEvents VisualLoggerEvents;
	TWeakPtr<IVisualLoggerInterface> VisualLoggerInterface;
	TWeakPtr<class STimeline> CurrentTimeLine;
	TWeakPtr<class SVisualLogger> CurrentVisualizer;
};

#include "LogVisualizerStyle.h"
#include "SVisualLogger.h"
#include "SVisualLoggerToolbar.h"
#include "VisualLoggerCommands.h"
#include "SVisualLoggerFilters.h"
#include "SVisualLoggerView.h"
#include "SVisualLoggerLogsList.h"
#include "SVisualLoggerStatusView.h"
#include "STimeline.h"
