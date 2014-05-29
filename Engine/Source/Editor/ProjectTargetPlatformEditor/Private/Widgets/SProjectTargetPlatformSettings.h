// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FProjectTargetPlatform
{
	FProjectTargetPlatform(FText InDisplayName, FName InPlatformName, const FSlateBrush* InIcon)
		: DisplayName(InDisplayName)
		, PlatformName(InPlatformName)
		, Icon(InIcon)
	{
	}

	/** Localized display name to show in the UI, eg, Windows */
	FText DisplayName;

	/** Internal platform name to use as a key, eg, WindowsNoEditor */
	FName PlatformName;

	/** Icon representing this platform */
	const FSlateBrush* Icon;
};

class SProjectTargetPlatformSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectTargetPlatformSettings)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	/** Generate a row for the available platforms list */
	TSharedRef<ITableRow> HandlePlatformsListViewGenerateRow(TSharedPtr<FProjectTargetPlatform> TargetPlatform, const TSharedRef<STableViewBase>& OwnerTable);

	/** List of available target platforms */
	TArray<TSharedPtr<FProjectTargetPlatform>> AvailablePlatforms;
};
