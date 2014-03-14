// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceToolbar.cpp: Implements the SDeviceToolbar class.
=============================================================================*/

#include "DeviceManagerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SDeviceToolbar"


/* SDeviceToolbar interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDeviceToolbar::Construct( const FArguments& InArgs, const FDeviceManagerModelRef& InModel )
{
	FDeviceDetailsCommands::Register();

	Model = InModel;

	// create and bind the commands
	UICommandList = MakeShareable(new FUICommandList);
	BindCommands();

	// create the toolbar
	FToolBarBuilder Toolbar(UICommandList, FMultiBoxCustomization::None);
	{
		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().Claim);
		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().Release);
		Toolbar.AddSeparator();

		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().Share);
		Toolbar.AddSeparator();

		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().PowerOn);
		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().PowerOff);
		Toolbar.AddToolBarButton(FDeviceDetailsCommands::Get().Reboot);
	}

	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.IsEnabled(this, &SDeviceToolbar::HandleToolbarIsEnabled)
			.Padding(0.0f)
			[
				Toolbar.MakeWidget()
			]
	];
}


/* SDeviceToolbar implementation
 *****************************************************************************/

void SDeviceToolbar::BindCommands( )
{
	const FDeviceDetailsCommands& Commands = FDeviceDetailsCommands::Get();

	// device claim commands
	UICommandList->MapAction(
		Commands.Claim,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandleClaimActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandleClaimActionCanExecute));

	UICommandList->MapAction(
		Commands.Release,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandleReleaseActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandleReleaseActionCanExecute));

	UICommandList->MapAction(
		Commands.Share,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandleShareActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandleShareActionCanExecute),
		FIsActionChecked::CreateSP(this, &SDeviceToolbar::HandleShareActionIsChecked));

	// device control commands
	UICommandList->MapAction(
		Commands.PowerOn,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandlePowerOnActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandlePowerOnActionCanExecute));

	UICommandList->MapAction(
		Commands.PowerOff,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandlePowerOffActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandlePowerOffActionCanExecute));

	UICommandList->MapAction(
		Commands.Reboot,
		FExecuteAction::CreateSP(this, &SDeviceToolbar::HandleRebootActionExecute),
		FCanExecuteAction::CreateSP(this, &SDeviceToolbar::HandleRebootActionCanExecute));
}


bool SDeviceToolbar::ValidateDeviceAction( const ITargetDeviceRef& Device ) const
{
	// @todo gmp: this needs to be improved, i.e. TargetPlatformManager::GetLocalDevice
	if (Device->GetName() != FPlatformProcess::ComputerName())
	{
		return true;
	}

	int32 DialogResult = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("LocalHostDialogPrompt", "WARNING: This device represents your local computer.\n\nAre you sure you want to proceed?"));

	return (DialogResult == EAppReturnType::Yes);
}


/* SDeviceToolbar callbacks
 *****************************************************************************/

void SDeviceToolbar::HandleClaimActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		DeviceService->Start();
	}
}


bool SDeviceToolbar::HandleClaimActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && !DeviceService->IsRunning() && DeviceService->GetClaimUser().IsEmpty());
}


void SDeviceToolbar::HandlePowerOffActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid() && ValidateDeviceAction(TargetDevice.ToSharedRef()))
		{
			TargetDevice->PowerOff(false);
		}
	}
}


bool SDeviceToolbar::HandlePowerOffActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid())
		{
			return (TargetDevice->IsConnected() && TargetDevice->SupportsFeature(ETargetDeviceFeatures::PowerOff));
		}
	}

	return false;
}


void SDeviceToolbar::HandlePowerOnActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid())
		{
			TargetDevice->PowerOn();
		}
	}
}


bool SDeviceToolbar::HandlePowerOnActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid())
		{
			return (TargetDevice->IsConnected() && TargetDevice->SupportsFeature(ETargetDeviceFeatures::PowerOn));
		}
	}

	return false;
}


void SDeviceToolbar::HandleRebootActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid() && ValidateDeviceAction(TargetDevice.ToSharedRef()))
		{
			TargetDevice->Reboot(true);
		}
	}
}


bool SDeviceToolbar::HandleRebootActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid())
		{
			return (TargetDevice->IsConnected() && TargetDevice->SupportsFeature(ETargetDeviceFeatures::Reboot));
		}
	}

	return false;
}


void SDeviceToolbar::HandleReleaseActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid())
	{
		DeviceService->Stop();
	}
}


bool SDeviceToolbar::HandleReleaseActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && DeviceService->IsRunning());
}


void SDeviceToolbar::HandleShareActionExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	if (DeviceService.IsValid() && DeviceService->IsRunning())
	{
		DeviceService->SetShared(!DeviceService->IsShared());
	}
}


bool SDeviceToolbar::HandleShareActionIsChecked( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && DeviceService->IsShared());
}


bool SDeviceToolbar::HandleShareActionCanExecute( )
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && DeviceService->IsRunning());
}


bool SDeviceToolbar::HandleToolbarIsEnabled( ) const
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && DeviceService->GetDevice().IsValid());
}


#undef LOCTEXT_NAMESPACE
