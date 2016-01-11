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
#include "SDockTab.h"

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
DECLARE_DELEGATE_ThreeParams(FOnLogLineSelectionChanged, TSharedPtr<FLogEntryItem> /*SelectedItem*/, int64 /*UserData*/, FName /*TagName*/);
DECLARE_DELEGATE_RetVal_TwoParams(FReply, FOnKeyboardEvent, const FGeometry& /*MyGeometry*/, const FKeyEvent& /*InKeyEvent*/);

struct FVisualLoggerEvents
{
	FVisualLoggerEvents() 
	{
	
	}

	FOnItemSelectionChanged OnItemSelectionChanged;
	FOnFiltersChanged OnFiltersChanged;
	FOnObjectSelectionChanged OnObjectSelectionChanged;
	FOnLogLineSelectionChanged OnLogLineSelectionChanged;
	FOnKeyboardEvent OnKeyboardEvent;
};

class FVisualLoggerTimeSliderController;
struct LOGVISUALIZER_API FLogVisualizer
{
	/** LogVisualizer interface*/
	void Goto(float Timestamp, FName LogOwner = NAME_None);
	void GotoNextItem( int32 Distance);
	void GotoPreviousItem(int32 Distance);
	void MoveCamera();

	FLinearColor GetColorForCategory(int32 Index) const;
	FLinearColor GetColorForCategory(const FString& InFilterName) const;
	TSharedPtr<FVisualLoggerTimeSliderController> GetTimeSliderController() { return TimeSliderController; }
	UWorld* GetWorld(UObject* OptionalObject = nullptr);
	class AActor* GetVisualLoggerHelperActor();
	FVisualLoggerEvents& GetVisualLoggerEvents() { return VisualLoggerEvents; }

	void SetCurrentVisualizer(TSharedPtr<class SVisualLogger> Visualizer) { CurrentVisualizer = Visualizer; }

	void OnObjectSelectionChanged(TSharedPtr<class STimeline> TimeLine) { CurrentTimeLine = TimeLine; }

	/** Static access */
	static void Initialize();
	static void Shutdown();
	static FLogVisualizer& Get();
protected:
	static TSharedPtr< struct FLogVisualizer > StaticInstance;
	
	TSharedPtr<FVisualLoggerTimeSliderController> TimeSliderController;
	FVisualLoggerEvents VisualLoggerEvents;
	TWeakPtr<class STimeline> CurrentTimeLine;
	TWeakPtr<class SVisualLogger> CurrentVisualizer;
};

class SVisualLoggerTab : public SDockTab
{
public:
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		return FLogVisualizer::Get().GetVisualLoggerEvents().OnKeyboardEvent.Execute(MyGeometry, InKeyEvent);
	}
};

class SVisualLoggerBaseWidget : public SCompoundWidget
{
public:
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		return FLogVisualizer::Get().GetVisualLoggerEvents().OnKeyboardEvent.Execute(MyGeometry, InKeyEvent);
	}
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
