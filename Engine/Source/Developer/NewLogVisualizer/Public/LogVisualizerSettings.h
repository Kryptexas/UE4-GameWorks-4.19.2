// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
GameplayDebuggerSettings.h: Declares the UGameplayDebuggerSettings class.
=============================================================================*/
#pragma once


#include "LogVisualizerSettings.generated.h"

UCLASS(config = EditorUserSettings)
class NEWLOGVISUALIZER_API ULogVisualizerSettings : public UObject
{
	GENERATED_UCLASS_BODY()
public:
#if WITH_EDITOR
	// Begin UObject Interface
	//virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//virtual void PostInitProperties() override;
	// End UObject Interface
#endif

public:
	/**Whether to show trivial logs, i.e. the ones with only one entry.*/
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger")
	bool bIgnoreTrivialLogs;

	/**Threshold for trivial Logs*/
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger", meta = (EditCondition = "bIgnoreTrivialLogs", ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10"))
	int32 TrivialLogsThreshold;

	/**Whether to show the recent data or not.*/
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger")
	bool bStickToRecentData;

	/**Whether to show histogram labels inside graph or outside.*/
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger")
	bool bShowHistogramLabelsOutside;

	/** Camera distance used to setup location during reaction on log item double click */
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger", meta = (ClampMin = "10", ClampMax = "1000", UIMin = "10", UIMax = "1000"))
	float DefaultCameraDistance;

	DECLARE_EVENT_OneParam(ULogVisualizerSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged() { return SettingChangedEvent; }

	/**Whether to search/filter categories or to get text vlogs into account too */
	UPROPERTY(EditAnywhere, config, Category = "VisualLogger")
	bool bSearchInsideLogs;

protected:

	// UObject overrides

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;

};
