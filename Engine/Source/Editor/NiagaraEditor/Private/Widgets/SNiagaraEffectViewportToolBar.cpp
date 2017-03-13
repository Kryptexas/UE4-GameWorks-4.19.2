// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEffectViewportToolBar.h"
#include "Widgets/Layout/SBorder.h"
#include "EditorStyleSet.h"
#include "NiagaraEditorCommands.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectViewportToolBar"


///////////////////////////////////////////////////////////
// SNiagaraEffectViewportToolBar

void SNiagaraEffectViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<class SNiagaraEffectViewport> InViewport)
{
	SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments(), InViewport);
}

TSharedRef<SWidget> SNiagaraEffectViewportToolBar::GenerateShowMenu() const
{
	GetInfoProvider().OnFloatingButtonClicked();

	TSharedRef<SEditorViewport> ViewportRef = GetInfoProvider().GetViewportWidget();

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder ShowMenuBuilder(bInShouldCloseWindowAfterMenuSelection, ViewportRef->GetCommandList());
	{
		auto Commands = FNiagaraEditorCommands::Get();

		//ShowMenuBuilder.AddMenuEntry(Commands.ToggleMaterialStats);
		//ShowMenuBuilder.AddMenuEntry(Commands.ToggleMobileStats);

		ShowMenuBuilder.AddMenuSeparator();

		ShowMenuBuilder.AddMenuEntry(Commands.TogglePreviewGrid);
		//ShowMenuBuilder.AddMenuEntry(Commands.TogglePreviewBackground);
	}

	return ShowMenuBuilder.MakeWidget();
}

bool SNiagaraEffectViewportToolBar::IsViewModeSupported(EViewModeIndex ViewModeIndex) const 
{
	switch (ViewModeIndex)
	{
	case VMI_PrimitiveDistanceAccuracy:
	case VMI_MeshUVDensityAccuracy:
	case VMI_RequiredTextureResolution:
		return false;
	default:
		return true;
	}
	return true; 
}

#undef LOCTEXT_NAMESPACE
