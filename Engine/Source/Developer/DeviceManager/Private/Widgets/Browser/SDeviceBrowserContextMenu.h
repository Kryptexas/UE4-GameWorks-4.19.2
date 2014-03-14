// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceBrowserContextMenu.h: Declares the SDeviceBrowserContextMenu class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SDeviceBrowserContextMenu"


namespace EDeviceBrowserContextMenuActions
{
	/**
	 * Enumerates available context menu actions.
	 */
	enum Type
	{
		/**
		 * Power off the device (forcefully).
		 */
		ForcePowerOff,

		/**
		 * Power off the device.
		 */
		PowerOff,

		/**
		 * Power on the device.
		 */
		PowerOn,

		/**
		 * Reboot the device.
		 */
		Reboot
	};
}


/**
 * Implements a context menu for the device browser list view.
 */
class SDeviceBrowserContextMenu
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDeviceBrowserContextMenu) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InDeviceProxy - The target device proxy to use.
	 */
	void Construct( const FArguments& InArgs, ITargetDeviceServicePtr InDeviceService )
	{
		DeviceService = InDeviceService;

		ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Content()
				[
					MakeContextMenu()
				]
		];
	}

protected:

	/**
	 * Builds the context menu widget.
	 *
	 * @return The context menu.
	 */
	TSharedRef<SWidget> MakeContextMenu( )
	{
		FMenuBuilder MenuBuilder(true, NULL);

		MenuBuilder.BeginSection("RemoteControl", LOCTEXT("MenuHeadingText", "Remote Control"));
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("MenuEntryPowerOnText", "Power on"), FText::GetEmpty(), FSlateIcon(), FUIAction(
				FExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryExecute, EDeviceBrowserContextMenuActions::PowerOn),
				FCanExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryCanExecute, EDeviceBrowserContextMenuActions::PowerOn))
				);

			MenuBuilder.AddMenuEntry(LOCTEXT("MenuEntryPowerOffText", "Power off"), FText::GetEmpty(), FSlateIcon(), FUIAction(
				FExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryExecute, EDeviceBrowserContextMenuActions::PowerOff),
				FCanExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryCanExecute, EDeviceBrowserContextMenuActions::PowerOff))
				);

			MenuBuilder.AddMenuEntry(LOCTEXT("MenuEntryForcePowerOffText", "Power off (force)"), FText::GetEmpty(), FSlateIcon(), FUIAction(
				FExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryExecute, EDeviceBrowserContextMenuActions::ForcePowerOff),
				FCanExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryCanExecute, EDeviceBrowserContextMenuActions::ForcePowerOff))
				);

			MenuBuilder.AddMenuEntry(LOCTEXT("MenuEntryRebootText", "Reboot"), FText::GetEmpty(), FSlateIcon(), FUIAction(
				FExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryExecute, EDeviceBrowserContextMenuActions::Reboot),
				FCanExecuteAction::CreateRaw(this, &SDeviceBrowserContextMenu::HandleContextMenuEntryCanExecute, EDeviceBrowserContextMenuActions::Reboot))
				);
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

private:

	// Callback for determining whether the specified context menu action can be executed.
	bool HandleContextMenuEntryCanExecute( EDeviceBrowserContextMenuActions::Type Action )
	{
		if (DeviceService.IsValid())
		{
			const ITargetDevicePtr& Device = DeviceService->GetDevice();

			if (Device.IsValid())
			{
				switch (Action)
				{
				case EDeviceBrowserContextMenuActions::ForcePowerOff:
				case EDeviceBrowserContextMenuActions::PowerOff:
					return Device->SupportsFeature(ETargetDeviceFeatures::PowerOff);

				case EDeviceBrowserContextMenuActions::PowerOn:
					return Device->SupportsFeature(ETargetDeviceFeatures::PowerOn);

				case EDeviceBrowserContextMenuActions::Reboot:
					return Device->SupportsFeature(ETargetDeviceFeatures::Reboot);
				}
			}
		}

		return false;
	}

	// Callback for executing the specified context menu action.
	void HandleContextMenuEntryExecute( EDeviceBrowserContextMenuActions::Type Action )
	{
		if (DeviceService.IsValid())
		{
			if (DeviceService->GetCachedDeviceName() == FPlatformProcess::ComputerName())
			{
				int32 DialogResult = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("LocalHostDialogPrompt", "WARNING: This device represents your local computer.\n\nAre you sure you want to proceed?"));

				if (DialogResult != EAppReturnType::Yes)
				{
					return;
				}
			}

			const ITargetDevicePtr& Device = DeviceService->GetDevice();

			if (!Device.IsValid())
			{
				return;
			}

			switch (Action)
			{
			case EDeviceBrowserContextMenuActions::ForcePowerOff:
				Device->PowerOff(true);
				break;

			case EDeviceBrowserContextMenuActions::PowerOff:
				Device->PowerOff(false);
				break;

			case EDeviceBrowserContextMenuActions::PowerOn:
				Device->PowerOn();
				break;

			case EDeviceBrowserContextMenuActions::Reboot:
				Device->Reboot();
				break;

			default:

				return;
			}
		}
	}

private:

	// Holds a pointer to the target device service.
	ITargetDeviceServicePtr DeviceService;
};


#undef LOCTEXT_NAMESPACE
