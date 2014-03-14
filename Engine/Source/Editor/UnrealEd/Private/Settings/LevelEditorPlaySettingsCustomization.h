// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LevelEditorPlaySettingsCustomization.h: Declares the FLevelEditorPlaySettingsCustomization class.
=============================================================================*/

#pragma once


#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"


#define LOCTEXT_NAMESPACE "FLevelEditorPlaySettingsCustomization"


class SScreenPositionCustomization
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SScreenPositionCustomization) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param LayoutBuilder - The layout builder to use for generating property widgets.
	 * @param InWindowPositionProperty - The handle to the window position property.
	 * @param InCenterWindowProperty - The handle to the center window property.
	 */
	void Construct( const FArguments& InArgs, IDetailLayoutBuilder* LayoutBuilder, const TSharedRef<IPropertyHandle>& InWindowPositionProperty, const TSharedRef<IPropertyHandle>& InCenterWindowProperty )
	{
		check(LayoutBuilder != NULL);

		CenterWindowProperty = InCenterWindowProperty;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
						.IsEnabled(this, &SScreenPositionCustomization::HandleNewWindowPositionPropertyIsEnabled)

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InWindowPositionProperty->CreatePropertyNameWidget(LOCTEXT("WindowPosXLabel", "Left Position").ToString())
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InWindowPositionProperty->GetChildHandle(0)->CreatePropertyValueWidget()
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
						.IsEnabled(this, &SScreenPositionCustomization::HandleNewWindowPositionPropertyIsEnabled)

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InWindowPositionProperty->CreatePropertyNameWidget(LOCTEXT("TopPositionLabel", "Top Position").ToString())
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							InWindowPositionProperty->GetChildHandle(1)->CreatePropertyValueWidget()
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Bottom)
				[
					InCenterWindowProperty->CreatePropertyValueWidget()
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				[
					InWindowPositionProperty->CreatePropertyNameWidget(LOCTEXT("CenterWindowLabel", "Always center window to screen").ToString())
				]
		];
	}

private:

	// Callback for checking whether the window position properties are enabled.
	bool HandleNewWindowPositionPropertyIsEnabled( ) const
	{
		bool CenterNewWindow;
		CenterWindowProperty->GetValue(CenterNewWindow);

		return !CenterNewWindow;

	}

private:

	// Holds the 'Center window' property
	TSharedPtr<IPropertyHandle> CenterWindowProperty;
};


/**
 * Implements a screen resolution picker widget.
 */
class SScreenResolutionCustomization
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SScreenResolutionCustomization) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param LayoutBuilder - The layout builder to use for generating property widgets.
	 * @param InWindowHeightProperty - The handle to the window height property.
	 * @param InWindowWidthProperty - The handle to the window width property.
	 */
	void Construct( const FArguments& InArgs, IDetailLayoutBuilder* LayoutBuilder, const TSharedRef<IPropertyHandle>& InWindowHeightProperty, const TSharedRef<IPropertyHandle>& InWindowWidthProperty )
	{
		check(LayoutBuilder != NULL);

		WindowHeightProperty = InWindowHeightProperty;
		WindowWidthProperty = InWindowWidthProperty;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							WindowWidthProperty->CreatePropertyNameWidget(LOCTEXT("WindowWidthLabel", "Window Width").ToString())
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							WindowWidthProperty->CreatePropertyValueWidget()
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							WindowHeightProperty->CreatePropertyNameWidget(LOCTEXT("WindowHeightLabel", "Window Height").ToString())
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						[
							WindowHeightProperty->CreatePropertyValueWidget()
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				[
					SNew(SComboButton)
						.ButtonContent()
						[
							SNew(STextBlock)
								.Font(LayoutBuilder->GetDetailFont())
								.Text(LOCTEXT("CommonResolutionsButtonText", "Common Window Sizes").ToString())
						]
						.ContentPadding(FMargin(6.0f, 1.0f))
						.MenuContent()
						[
							MakeCommonResolutionsMenu()
						]
						.ToolTipText(LOCTEXT("CommonResolutionsButtonTooltip", "Pick from a list of common screen resolutions").ToString())
				]		
		];
	}

protected:

	/**
	 * Adds a menu entry to the common screen resolutions menu.
	 */
	void AddCommonResolutionEntry( FMenuBuilder& MenuBuilder, int32 Width, int32 Height, const FString& AspectRatio, const FText& Description )
	{
	}

	void AddScreenResolutionSection( FMenuBuilder& MenuBuilder, const TArray<FPlayScreenResolution>& Resolutions, const FText& SectionName )
	{
		MenuBuilder.BeginSection(NAME_None, SectionName);
		{
			for (auto Iter = Resolutions.CreateConstIterator(); Iter; ++Iter)
			{
				FUIAction Action(FExecuteAction::CreateRaw(this, &SScreenResolutionCustomization::HandleCommonResolutionSelected, Iter->Width, Iter->Height));

				FFormatNamedArguments Args;
				Args.Add(TEXT("Width"), FText::AsNumber(Iter->Width, NULL, FInternationalization::GetInvariantCulture()));
				Args.Add(TEXT("Height"), FText::AsNumber(Iter->Height, NULL, FInternationalization::GetInvariantCulture()));
				Args.Add(TEXT("AspectRatio"), FText::FromString(Iter->AspectRatio));

				MenuBuilder.AddMenuEntry(FText::FromString(Iter->Description), FText::Format(LOCTEXT("CommonResolutionFormat", "{Width} x {Height} (AspectRatio)"), Args), FSlateIcon(), Action);
			}
		}
		MenuBuilder.EndSection();
	}

	/**
	 * Creates a widget for the resolution picker.
	 *
	 * @return The widget.
	 */
	TSharedRef<SWidget> MakeCommonResolutionsMenu( )
	{
		const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();
		FMenuBuilder MenuBuilder(true, NULL);

		AddScreenResolutionSection(MenuBuilder, PlaySettings->PhoneScreenResolutions, LOCTEXT("CommonPhonesSectionHeader", "Phones"));
		AddScreenResolutionSection(MenuBuilder, PlaySettings->TabletScreenResolutions, LOCTEXT("CommonTabletsSectionHeader", "Tablets"));
		AddScreenResolutionSection(MenuBuilder, PlaySettings->LaptopScreenResolutions, LOCTEXT("CommonLaptopsSectionHeader", "Laptops"));
		AddScreenResolutionSection(MenuBuilder, PlaySettings->MonitorScreenResolutions, LOCTEXT("CommoMonitorsSectionHeader", "Monitors"));
		AddScreenResolutionSection(MenuBuilder, PlaySettings->TelevisionScreenResolutions, LOCTEXT("CommonTelevesionsSectionHeader", "Televisions"));

		return MenuBuilder.MakeWidget();
	}

private:

	// Handles selecting a common screen resolution.
	void HandleCommonResolutionSelected( int32 Width, int32 Height )
	{
		WindowHeightProperty->SetValue(Height);
		WindowWidthProperty->SetValue(Width);
	}

private:

	// Holds the handle to the window height property.
	TSharedPtr<IPropertyHandle> WindowHeightProperty;

	// Holds the handle to the window width property.
	TSharedPtr<IPropertyHandle> WindowWidthProperty;
};


/**
 * Implements a details view customization for ULevelEditorPlaySettings objects.
 */
class FLevelEditorPlaySettingsCustomization
	: public IDetailCustomization
{
public:

	// Begin IDetailCustomization interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& LayoutBuilder ) OVERRIDE
	{
		const float MaxPropertyWidth = 400.0f;

		// play in new window settings
		IDetailCategoryBuilder& PlayInNewWindowCategory = LayoutBuilder.EditCategory("PlayInNewWindow");
		{
			// new window size
			TSharedRef<IPropertyHandle> WindowHeightHandle = LayoutBuilder.GetProperty("NewWindowHeight");
			TSharedRef<IPropertyHandle> WindowWidthHandle = LayoutBuilder.GetProperty("NewWindowWidth");
			TSharedRef<IPropertyHandle> WindowPositionHandle = LayoutBuilder.GetProperty("NewWindowPosition");
			TSharedRef<IPropertyHandle> CenterNewWindowHandle = LayoutBuilder.GetProperty("CenterNewWindow");

			WindowHeightHandle->MarkHiddenByCustomization();
			WindowWidthHandle->MarkHiddenByCustomization();
			WindowPositionHandle->MarkHiddenByCustomization();
			CenterNewWindowHandle->MarkHiddenByCustomization();

			PlayInNewWindowCategory.AddCustomRow(LOCTEXT("NewWindowSizeRow", "New Window Size").ToString(), false)
				.NameContent()
				[
					SNew(STextBlock)
						.Font(LayoutBuilder.GetDetailFont())
						.Text(LOCTEXT("NewWindowSizeName", "New Window Size"))
						.ToolTipText(LOCTEXT("NewWindowSizeTooltip", "Sets the width and height of floating PIE windows (in pixels)"))
				]
				.ValueContent()
				.MaxDesiredWidth(MaxPropertyWidth)
				[
					SNew(SScreenResolutionCustomization, &LayoutBuilder, WindowHeightHandle, WindowWidthHandle)
				];

			PlayInNewWindowCategory.AddCustomRow(LOCTEXT("NewWindowPositionRow", "New Window Position").ToString(), false)
				.NameContent()
				[
					SNew(STextBlock)
						.Font(LayoutBuilder.GetDetailFont())
						.Text(LOCTEXT("NewWindowPositionName", "New Window Position"))
						.ToolTipText(LOCTEXT("NewWindowPositionTooltip", "Sets the screen coordinates for the top-left corner of floating PIE windows (in pixels)"))
				]
				.ValueContent()
				.MaxDesiredWidth(MaxPropertyWidth)
				[
					SNew(SScreenPositionCustomization, &LayoutBuilder, WindowPositionHandle, CenterNewWindowHandle)
				];
		}

		// play in standalone game settings
		IDetailCategoryBuilder& PlayInStandaloneCategory = LayoutBuilder.EditCategory("PlayInStandaloneGame");
		{
			// standalone window size
			TSharedRef<IPropertyHandle> WindowHeightHandle = LayoutBuilder.GetProperty("StandaloneWindowHeight");
			TSharedRef<IPropertyHandle> WindowWidthHandle = LayoutBuilder.GetProperty("StandaloneWindowWidth");

			WindowHeightHandle->MarkHiddenByCustomization();
			WindowWidthHandle->MarkHiddenByCustomization();

			PlayInStandaloneCategory.AddCustomRow(LOCTEXT("PlayInStandaloneWindowDetails", "Standalone Window Size").ToString(), false)
				.NameContent()
				[
					SNew(STextBlock)
						.Font(LayoutBuilder.GetDetailFont())
						.Text(LOCTEXT("StandaloneWindowSizeName", "Standalone Window Size"))
						.ToolTipText(LOCTEXT("StandaloneWindowSizeTooltip", "Sets the width and height of standalone game windows (in pixels)"))
				]
				.ValueContent()
				.MaxDesiredWidth(MaxPropertyWidth)
				[
					SNew(SScreenResolutionCustomization, &LayoutBuilder, WindowHeightHandle, WindowWidthHandle)
				];

			// command line options
			TSharedPtr<IPropertyHandle> DisableStandaloneSoundProperty = LayoutBuilder.GetProperty("DisableStandaloneSound");

			DisableStandaloneSoundProperty->MarkHiddenByCustomization();

			PlayInStandaloneCategory.AddCustomRow(LOCTEXT("AdditionalOptionsDetails", "Additional Options").ToString(), true)
				.NameContent()
				[
					SNew(STextBlock)
						.Font(LayoutBuilder.GetDetailFont())
						.Text(LOCTEXT("ClientCmdLineName", "Command Line Options"))
						.ToolTipText(LOCTEXT("ClientCmdLineTooltip", "Generates a command line for additional settings that will be passed to the game clients."))
				]
				.ValueContent()
				.MaxDesiredWidth(MaxPropertyWidth)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							DisableStandaloneSoundProperty->CreatePropertyValueWidget()
						]

					+ SHorizontalBox::Slot()
						.Padding(0.0f, 2.5f)
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							DisableStandaloneSoundProperty->CreatePropertyNameWidget(LOCTEXT("DisableStandaloneSoundLabel", "Disable Sound (-nosound)").ToString())
						]
		
				];
		}

		// multi-player options
		IDetailCategoryBuilder& NetworkCategory = LayoutBuilder.EditCategory("MultiplayerOptions");
		{
			// net mode & clients
			NetModeProperty = LayoutBuilder.GetProperty("PlayNetMode", ULevelEditorPlaySettings::StaticClass());
			NumClientsProperty = LayoutBuilder.GetProperty("PlayNumberOfClients", ULevelEditorPlaySettings::StaticClass());
			RunUnderOneProcessProperty = LayoutBuilder.GetProperty("RunUnderOneProcess", ULevelEditorPlaySettings::StaticClass());			

			// Number of players
			NetworkCategory.AddProperty("PlayNumberOfClients")
				.DisplayName(TEXT("Number of Players"))
				.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandlePlayNumberOfClientsIsEnabled)));

			NetworkCategory.AddProperty("AdditionalServerGameOptions")
				.DisplayName(TEXT("Server Game Options"))
				.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandleGameOptionsIsEnabled)));

			NetworkCategory.AddProperty("PlayNetDedicated")
				.DisplayName(TEXT("Run Dedicated Server"))
				.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandlePlayNetDedicatedPropertyIsEnabled)));

			// client window size
			TSharedRef<IPropertyHandle> WindowHeightHandle = LayoutBuilder.GetProperty("ClientWindowHeight");
			TSharedRef<IPropertyHandle> WindowWidthHandle = LayoutBuilder.GetProperty("ClientWindowWidth");

			WindowHeightHandle->MarkHiddenByCustomization();
			WindowWidthHandle->MarkHiddenByCustomization();

			NetworkCategory.AddProperty("RouteGamepadToSecondWindow")
				.DisplayName(TEXT("Route 1st Gamepad to 2nd Client"))
				.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandleRerouteInputToSecondWindowEnabled)))
				.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandleRerouteInputToSecondWindowVisibility)));
			

			// Run under one instance
			if (GEditor && GEditor->bAllowMultiplePIEWorlds)
			{

				NetworkCategory.AddProperty("RunUnderOneProcess")
					.DisplayName( TEXT("Use Single Process") )
					.IsEnabled( TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP( this, &FLevelEditorPlaySettingsCustomization::HandleRunUnderOneProcessIsEnabled ) ) );
			}
			else
			{
				NetworkCategory.AddProperty("RunUnderOneProcess")
					.DisplayName( TEXT("Run Under One Process is disabled.") )
					.Visibility( EVisibility::Collapsed )
					.IsEnabled( false );
			}

			// Net Mode
			NetworkCategory.AddProperty("PlayNetMode")
				.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandlePlayNetModeVisibility)))
				.DisplayName(TEXT("Editor Client Mode"));

			NetworkCategory.AddProperty("AdditionalLaunchOptions")
				.DisplayName(TEXT("Command Line Arguments"))
				.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandleCmdLineVisibility)));

			NetworkCategory.AddCustomRow(LOCTEXT("PlayInNetworkWindowDetails", "Client Window Size").ToString(), false)
				.NameContent()
				[
					WindowHeightHandle->CreatePropertyNameWidget(LOCTEXT("ClientWindowSizeName", "Client Window Size (in pixels)").ToString())
				]
				.ValueContent()
				.MaxDesiredWidth(MaxPropertyWidth)
				[
					SNew(SScreenResolutionCustomization, &LayoutBuilder, WindowHeightHandle, WindowWidthHandle)
				]
				.IsEnabled(TAttribute<bool>(this, &FLevelEditorPlaySettingsCustomization::HandleClientWindowSizePropertyIsEnabled))
				.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FLevelEditorPlaySettingsCustomization::HandleClientWindowSizePropertyVisibility)));
				
		}
	}

	// End IDetailCustomization interface

public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for play-in settings.
	 */
	static TSharedRef<IDetailCustomization> MakeInstance( )
	{
		return MakeShareable(new FLevelEditorPlaySettingsCustomization());
	}

private:

	// Callback for checking whether the ClientWindowHeight and ClientWindowWidth properties are enabled.
	bool HandleClientWindowSizePropertyIsEnabled( ) const
	{
		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);
		
		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);

		if (NetMode == PIE_Standalone && RunUnderOneProcess)
		{
			return false;
		}

		int32 NumClients;
		NumClientsProperty->GetValue(NumClients);

		return (NumClients >= 2);
	}

	EVisibility HandleClientWindowSizePropertyVisibility() const
	{
		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);

		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);

		if (RunUnderOneProcess)
		{
			return EVisibility::Hidden;
		}

		return EVisibility::Visible;
	}

	// Callback for checking whether the PlayNetDedicated is enabled.
	bool HandlePlayNetDedicatedPropertyIsEnabled( ) const
	{		
		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);
		if (RunUnderOneProcess)
		{
			return true;
		}

		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);
		return (NetMode == PIE_Client);
	}

	// Callback for checking whether the PlayNumberOfClients is enabled.
	bool HandlePlayNumberOfClientsIsEnabled( ) const
	{
		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);

		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);

		return (NetMode != PIE_Standalone) || RunUnderOneProcess;
	}

	// Callback for checking whether the PlayNumberOfClients is enabled.
	bool HandleGameOptionsIsEnabled( ) const
	{
		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);

		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);

		return (NetMode != PIE_Standalone) || RunUnderOneProcess;
	}

	// Callback for checking whether the CmdLineOptions is enabled.
	bool HandleCmdLineOptionsIsEnabled( ) const
	{
		uint8 NetMode;
		NetModeProperty->GetValue(NetMode);

		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);

		return (NetMode != PIE_Standalone) || RunUnderOneProcess;
	}

	bool HandleRunUnderOneProcessIsEnabled( ) const
	{
		return true;
	}

	bool HandleRerouteInputToSecondWindowEnabled( ) const
	{
		int32 NumClients;
		NumClientsProperty->GetValue(NumClients);

		return NumClients > 1;	
	}
	
	EVisibility HandleRerouteInputToSecondWindowVisibility( ) const
	{
		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);
		return (RunUnderOneProcess ? EVisibility::Visible : EVisibility::Hidden);
	}

	EVisibility HandlePlayNetModeVisibility( ) const
	{
		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);
		return (RunUnderOneProcess ? EVisibility::Hidden : EVisibility::Visible);
	}

	EVisibility HandleCmdLineVisibility( ) const
	{
		bool RunUnderOneProcess;
		RunUnderOneProcessProperty->GetValue(RunUnderOneProcess);
		return (RunUnderOneProcess ? EVisibility::Hidden : EVisibility::Visible);
	}

private:

	// Holds the 'Net Mode' property.
	TSharedPtr<IPropertyHandle> NetModeProperty;

	// Holds the 'Num Clients' property.
	TSharedPtr<IPropertyHandle> NumClientsProperty;

	// Holds the 'Run under one instance' property.
	TSharedPtr<IPropertyHandle> RunUnderOneProcessProperty;
};


#undef LOCTEXT_NAMESPACE
