
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Toolkits/IToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "Toolkits/SStandaloneAssetEditorToolkitHost.h"
#include "Toolkits/SAssetEditorCommon.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "MainFrame.h"

#define LOCTEXT_NAMESPACE "StandaloneAssetEditorToolkit"

void SStandaloneAssetEditorToolkitHost::Construct( const SStandaloneAssetEditorToolkitHost::FArguments& InArgs, const TSharedPtr<FTabManager>& InTabManager, const FName InitAppName )
{
	EditorCloseRequest = InArgs._OnRequestClose;
	AppName = InitAppName;

	MyTabManager = InTabManager;
}

void SStandaloneAssetEditorToolkitHost::SetupInitialContent( const TSharedRef<FTabManager::FLayout>& DefaultLayout, const TSharedPtr<SDockTab>& InHostTab, const bool bCreateDefaultStandaloneMenu )
{
	// @todo toolkit major: Expose common asset editing features here! (or require the asset editor's content to do this itself!)
	//		- Add a "toolkit menu"
	//				- Toolkits can access this and add menu items as needed
	//				- In world-centric, main frame menu becomes extendable
	//						- e.g., "Blueprint", "Debug" menus added
	//				- In standalone, toolkits get their own menu
	//						- Also, the core menu is just added as the first pull-down in the standalone menu
	//				- Multiple toolkits can be active and add their own menu items!
	//				- In world-centric, the core toolkit menu is available from the drop down
	//						- No longer need drop down next to toolkit display?  Not sure... Probably still want this
	//		- Add a "toolkit toolbar"
	//				- In world-centric, draws next to the level editor tool bar (or on top of)
	//						- Could either extend existing tool bar or add additional tool bars
	//						- May need to change arrangement to allow for wider tool bars (maybe displace grid settings too)
	//				- In standalone, just draws under the toolkit's menu

	
	if (bCreateDefaultStandaloneMenu)
	{
		struct Local
		{
			static void FillFileMenu( FMenuBuilder& MenuBuilder, TWeakPtr< FAssetEditorToolkit > AssetEditorToolkitWeak )
			{
				auto AssetEditorToolkit( AssetEditorToolkitWeak.Pin().ToSharedRef() );
				
				AssetEditorToolkit->FillDefaultFileMenuCommands( MenuBuilder );
			}

			static void AddAssetMenu( FMenuBarBuilder& MenuBarBuilder, TWeakPtr< FAssetEditorToolkit > AssetEditorToolkitWeak )
			{
				MenuBarBuilder.AddPullDownMenu( 
					LOCTEXT("AssetMenuLabel", "Asset"),		// @todo toolkit major: Either use "Asset", "File", or the asset type name e.g. "Blueprint" (Also update custom pull-down menus)
					LOCTEXT("AssetMenuLabel_ToolTip", "Opens a menu with commands for managing this asset"),
					FNewMenuDelegate::CreateStatic( &Local::FillAssetMenu, AssetEditorToolkitWeak ),
					"Asset");

				auto AssetEditorToolkit( AssetEditorToolkitWeak.Pin().ToSharedRef() );
			}

			static void FillAssetMenu( FMenuBuilder& MenuBuilder, TWeakPtr< FAssetEditorToolkit > AssetEditorToolkitWeak )
			{
				auto AssetEditorToolkit( AssetEditorToolkitWeak.Pin().ToSharedRef() );
				
				MenuBuilder.BeginSection("AssetEditorActions", LOCTEXT("ActionsHeading", "Actions") );
				{
					AssetEditorToolkit->FillDefaultAssetMenuCommands( MenuBuilder );
				}
				MenuBuilder.EndSection();
			}

			static void ExtendHelpMenu( FMenuBuilder& MenuBuilder, TWeakPtr< FAssetEditorToolkit > AssetEditorToolkitWeak )
			{
				auto AssetEditorToolkit( AssetEditorToolkitWeak.Pin().ToSharedRef() );

				MenuBuilder.BeginSection("HelpBrowse", NSLOCTEXT("MainHelpMenu", "Browse", "Browse"));
				{
					AssetEditorToolkit->FillDefaultHelpMenuCommands( MenuBuilder );
				}
				MenuBuilder.EndSection();
			}
		};

		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

		auto AssetEditorToolkit = StaticCastSharedRef< FAssetEditorToolkit >( HostedToolkit.ToSharedRef() );

		// Add asset-specific menu items to the top of the "File" menu
		MenuExtender->AddMenuExtension( "FileLoadAndSave", EExtensionHook::First, AssetEditorToolkit->GetToolkitCommands(), FMenuExtensionDelegate::CreateStatic( &Local::FillFileMenu, TWeakPtr< FAssetEditorToolkit >( AssetEditorToolkit ) ) );

		// Add the "Asset" menu, if we're editing an asset
		if (AssetEditorToolkit->IsActuallyAnAsset())
		{
			MenuExtender->AddMenuBarExtension( "Edit", EExtensionHook::After, AssetEditorToolkit->GetToolkitCommands(), FMenuBarExtensionDelegate::CreateStatic( &Local::AddAssetMenu, TWeakPtr< FAssetEditorToolkit >( AssetEditorToolkit ) ) );
		}

		MenuExtender->AddMenuExtension( "HelpOnline", EExtensionHook::Before, AssetEditorToolkit->GetToolkitCommands(), FMenuExtensionDelegate::CreateStatic( &Local::ExtendHelpMenu, TWeakPtr< FAssetEditorToolkit >( AssetEditorToolkit ) ) );

		MenuExtenders.Add(MenuExtender);
	}

	DefaultMenuWidget = SNullWidget::NullWidget;

	HostTabPtr = InHostTab;

	RestoreFromLayout(DefaultLayout);
	GenerateMenus(bCreateDefaultStandaloneMenu);
}


void SStandaloneAssetEditorToolkitHost::RestoreFromLayout( const TSharedRef<FTabManager::FLayout>& NewLayout )
{
	const TSharedRef<SDockTab> HostTab = HostTabPtr.Pin().ToSharedRef();
	HostTab->SetCanCloseTab(EditorCloseRequest);

	this->ChildSlot[SNullWidget::NullWidget];
	MyTabManager->CloseAllAreas();

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow( HostTab );
	TSharedPtr<SWidget> RestoredUI = MyTabManager->RestoreFrom( NewLayout, ParentWindow );

	checkf(RestoredUI.IsValid(), TEXT("The layout must have a primary dock area") );
	
	this->ChildSlot
	[
		SNew( SVerticalBox )
			// Menu bar area
			+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SOverlay)
					// The menu bar
					+SOverlay::Slot()
					[
						SAssignNew(MenuWidgetContent, SBorder)
						.Padding(0)
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
						[
							DefaultMenuWidget.ToSharedRef()
						]
					]
					// The menu bar overlay
					+SOverlay::Slot()
					.HAlign(HAlign_Right)
					[
						SAssignNew(MenuOverlayWidgetContent, SBorder)
						.Padding(0)
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
					]
				]
			// Viewport/Document/Docking area
			+SVerticalBox::Slot()
				.Padding( 1.0f )
				
				// Fills all leftover space
				.FillHeight( 1.0f )
				[
					RestoredUI.ToSharedRef()
				]
	];
}


void SStandaloneAssetEditorToolkitHost::GenerateMenus(bool bForceCreateMenu)
{
	if( bForceCreateMenu || DefaultMenuWidget != SNullWidget::NullWidget )
	{
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
		DefaultMenuWidget = MainFrameModule.MakeMainMenu( MyTabManager, FExtender::Combine(MenuExtenders).ToSharedRef() );

		MenuWidgetContent->SetContent(DefaultMenuWidget.ToSharedRef());
	}
}

void SStandaloneAssetEditorToolkitHost::SetMenuOverlay( TSharedRef<SWidget> NewOverlay )
{
	MenuOverlayWidgetContent->SetContent(NewOverlay);
}

SStandaloneAssetEditorToolkitHost::~SStandaloneAssetEditorToolkitHost()
{
	// Let the toolkit manager know that we're going away now
	FToolkitManager::Get().OnToolkitHostDestroyed( this );
}


TSharedRef< SWidget > SStandaloneAssetEditorToolkitHost::GetParentWidget()
{
	return AsShared();
}


void SStandaloneAssetEditorToolkitHost::BringToFront()
{
	FGlobalTabmanager::Get()->DrawAttentionToTabManager( this->MyTabManager.ToSharedRef() );
}


TSharedRef< SDockTabStack > SStandaloneAssetEditorToolkitHost::GetTabSpot( const EToolkitTabSpot::Type TabSpot )
{
	return TSharedPtr<SDockTabStack>().ToSharedRef();
}


void SStandaloneAssetEditorToolkitHost::OnToolkitHostingStarted( const TSharedRef< class IToolkit >& Toolkit )
{
	// Keep track of the toolkit we're hosting
	check( !HostedToolkit.IsValid() );
	HostedToolkit = Toolkit;

	// The tab manager needs to know how to spawn tabs from this toolkit
	Toolkit->RegisterTabSpawners(MyTabManager.ToSharedRef());
}


void SStandaloneAssetEditorToolkitHost::OnToolkitHostingFinished( const TSharedRef< class IToolkit >& Toolkit )
{
	// No need to worry about the toolkit anymore
	check( Toolkit == HostedToolkit );
	
	// The tab manager should forget how to spawn tabs from this toolkit
	Toolkit->UnregisterTabSpawners(MyTabManager.ToSharedRef());

	// Standalone Asset Editors close by shutting down their major tab.
	const TSharedPtr<SDockTab> HostTab = HostTabPtr.Pin();
	if (HostTab.IsValid())
	{
		HostTab->RequestCloseTab();
	}

	HostedToolkit.Reset();
}


UWorld* SStandaloneAssetEditorToolkitHost::GetWorld() const 
{
	// Currently, standalone asset editors never have a world
	return NULL;
}


FReply SStandaloneAssetEditorToolkitHost::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	// Check to see if any of the actions for the level editor can be processed by the current keyboard event
	// If we are in debug mode do not process commands
	if( HostedToolkit.IsValid() && HostedToolkit->ProcessCommandBindings( InKeyboardEvent ) )
	{
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyboardEvent);
}


#undef LOCTEXT_NAMESPACE