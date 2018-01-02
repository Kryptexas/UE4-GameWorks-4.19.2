// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "Framework/Commands/UICommandInfo.h"
#include "ShowFlagFilter.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Templates/Function.h"
#include "EditorViewportClient.h"

class COMMONMENUEXTENSIONS_API FShowFlagMenuCommands : public TCommands<FShowFlagMenuCommands>
{
public:
	struct FShowFlagCommand
	{
		FEngineShowFlags::EShowFlag FlagIndex;
		TSharedPtr<FUICommandInfo> ShowMenuItem;
		FText LabelOverride;

		FShowFlagCommand(FEngineShowFlags::EShowFlag InFlagIndex, const TSharedPtr<FUICommandInfo>& InShowMenuItem, const FText& InLabelOverride)
			: FlagIndex(InFlagIndex),
			  ShowMenuItem(InShowMenuItem),
			  LabelOverride(InLabelOverride)
		{
		}

		FShowFlagCommand(FEngineShowFlags::EShowFlag InFlagIndex, const TSharedPtr<FUICommandInfo>& InShowMenuItem)
			: FlagIndex(InFlagIndex),
			  ShowMenuItem(InShowMenuItem),
			  LabelOverride()
		{
		}
	};

	FShowFlagMenuCommands();

	virtual void RegisterCommands() override;

	void BindCommands(FUICommandList& CommandList, const TSharedPtr<FEditorViewportClient>& Client) const;
	void BuildShowFlagsMenu(FMenuBuilder& Menu, const FShowFlagFilter& Filter = FShowFlagFilter(FShowFlagFilter::IncludeAllFlagsByDefault)) const;

private:
	static void StaticCreateShowFlagsSubMenu(FMenuBuilder& MenuBuilder, TArray<uint32> FlagIndices, int32 EntryOffset);
	static void ToggleShowFlag(const TSharedPtr<FEditorViewportClient>& Client, FEngineShowFlags::EShowFlag EngineShowFlagIndex);
	static bool IsShowFlagEnabled(const TSharedPtr<FEditorViewportClient>& Client, FEngineShowFlags::EShowFlag EngineShowFlagIndex);

	FSlateIcon GetShowFlagIcon(const FShowFlagData& Flag) const;

	void CreateShowFlagCommands();
	void CreateCommonShowFlagMenuItems(FMenuBuilder& Menu, const FShowFlagFilter& Filter) const;
	void CreateSubMenuIfRequired(FMenuBuilder& Menu, const FShowFlagFilter& Filter, EShowFlagGroup Group, const FText& MenuLabel, const FText& ToolTip) const;
	void CreateShowFlagsSubMenu(FMenuBuilder& MenuBuilder, TArray<uint32> MenuCommands, int32 EntryOffset) const;

	TArray<FShowFlagCommand> ShowFlagCommands;
	bool bCommandsInitialised;
};