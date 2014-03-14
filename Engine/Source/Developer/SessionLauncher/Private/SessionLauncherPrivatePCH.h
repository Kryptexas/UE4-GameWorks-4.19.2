// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionLauncherPrivatePCH.h: Pre-compiled header file for the SessionLauncher module.
=============================================================================*/

#ifndef UNREAL_SESSIONLAUNCHER_PRIVATEPCH_H
#define UNREAL_SESSIONLAUNCHER_PRIVATEPCH_H


#include "../Public/SessionLauncher.h"


/* Dependencies
 *****************************************************************************/

#include "DesktopPlatformModule.h"
#include "IFilter.h"
#include "ModuleManager.h"
#include "TargetDeviceServices.h"
#include "TargetPlatform.h"
#include "TextFilter.h"


/* Static helpers
 *****************************************************************************/

// @todo gmp: move this into a shared library or create a SImageButton widget.
static TSharedRef<SButton> MakeImageButton( const FSlateBrush* ButtonImage, const FText& ButtonTip, const FOnClicked& OnClickedDelegate )
{
	return SNew(SButton)
		.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
		.OnClicked(OnClickedDelegate)
		.ToolTipText(ButtonTip)
		.ContentPadding(2)
		.VAlign(VAlign_Center)
		.ForegroundColor(FSlateColor::UseForeground())
		.Content()
		[
			SNew(SImage)
				.Image(ButtonImage)
				.ColorAndOpacity(FSlateColor::UseForeground())
		];
}


/* Private includes
 *****************************************************************************/

#include "SessionLauncherCommands.h"
#include "SessionLauncherModel.h"

#include "SSessionLauncherBuildConfigurationSelector.h"
#include "SSessionLauncherFormLabel.h"

#include "SSessionLauncherProjectPicker.h"
#include "SSessionLauncherProjectPage.h"

#include "SSessionLauncherBuildPage.h"

#include "SSessionLauncherMapListRow.h"
#include "SSessionLauncherCultureListRow.h"
#include "SSessionLauncherCookedPlatforms.h"
#include "SSessionLauncherCookByTheBookSettings.h"
#include "SSessionLauncherCookOnTheFlySettings.h"
#include "SSessionLauncherCookPage.h"
#include "SSessionLauncherSimpleCookPage.h"

#include "SSessionLauncherDeviceGroupSelector.h"
#include "SSessionLauncherDeviceListRow.h"
#include "SSessionLauncherDeployTargets.h"
#include "SSessionLauncherDeployFileServerSettings.h"
#include "SSessionLauncherDeployToDeviceSettings.h"
#include "SSessionLauncherDeployRepositorySettings.h"
#include "SSessionLauncherDeployPage.h"

#include "SSessionLauncherLaunchRoleEditor.h"
#include "SSessionLauncherLaunchCustomRoles.h"
#include "SSessionLauncherLaunchPage.h"

#include "SSessionLauncherPackagingSettings.h"
#include "SSessionLauncherPackagePage.h"

#include "SSessionLauncherPreviewPage.h"

#include "SSessionLauncherTaskListRow.h"
#include "SSessionLauncherMessageListRow.h"
#include "SSessionLauncherProgress.h"

#include "SSessionLauncherSettings.h"
#include "SSessionLauncherDeployTaskSettings.h"
#include "SSessionLauncherBuildTaskSettings.h"
#include "SSessionLauncherLaunchTaskSettings.h"

#include "SSessionLauncherProfileSelector.h"
#include "SSessionLauncherToolbar.h"
#include "SSessionLauncherValidation.h"
#include "SSessionLauncher.h"


#endif //UNREAL_SESSIONLAUNCHER_PRIVATEPCH_H
