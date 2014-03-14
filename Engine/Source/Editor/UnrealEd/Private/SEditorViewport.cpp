// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "SceneViewport.h"
#include "EditorViewportCommands.h"

SEditorViewport::SEditorViewport()
{
}

SEditorViewport::~SEditorViewport()
{
	// Close viewport
	if( Client.IsValid() )
	{
		Client->Viewport = NULL;
	}

	// Release our reference to the viewport client
	Client.Reset();

	check( SceneViewport.IsUnique() );
}

void SEditorViewport::Construct( const FArguments& InArgs )
{
	

	ChildSlot
	[
		SNew(STutorialWrapper)
		.Name("EditorViewports")
		.Content()
		[
			SAssignNew( ViewportWidget, SViewport )
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
			.ShowEffectWhenDisabled( false )
			.EnableGammaCorrection( false ) // Scene rendering handles this
			[
				SNew( SBorder )
				.BorderImage( this, &SEditorViewport::OnGetViewportBorderBrush )
				.BorderBackgroundColor( this, &SEditorViewport::OnGetViewportBorderColorAndOpacity )
				.Visibility( this, &SEditorViewport::OnGetViewportContentVisibility )
				.Padding(0.0f)
				.ShowEffectWhenDisabled( false )
				[
					SAssignNew( ViewportOverlay, SOverlay )
			
				]
			]
		]
	];

	TSharedRef<FEditorViewportClient> ViewportClient = MakeEditorViewportClient();
	SceneViewport = MakeShareable( new FSceneViewport( &ViewportClient.Get(), ViewportWidget ) );
	ViewportClient->Viewport = SceneViewport.Get();
	ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());
	Client = ViewportClient;

	CommandList = MakeShareable( new FUICommandList );
	// Ensure the commands are registered
	FEditorViewportCommands::Register();
	BindCommands();

	TSharedPtr<SWidget> ViewportToolbar = MakeViewportToolbar();

	if( ViewportToolbar.IsValid() )
	{
		ViewportOverlay->AddSlot()
			.VAlign(VAlign_Top)
			[
				ViewportToolbar.ToSharedRef()
			];
	}
}

FReply SEditorViewport::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FReply::Unhandled();
	if( CommandList->ProcessCommandBindings( InKeyboardEvent ) )
	{
		Reply = FReply::Handled();
		Client->Invalidate();
	}

	return Reply;
		
}

bool SEditorViewport::SupportsKeyboardFocus() const 
{
	return true;
}

FReply SEditorViewport::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	// forward focus to the viewport
	return FReply::Handled().SetKeyboardFocus( ViewportWidget.ToSharedRef(), InKeyboardFocusEvent.GetCause() );
}

void SEditorViewport::BindCommands()
{
	FUICommandList& CommandListRef = *CommandList;

	const FEditorViewportCommands& Commands = FEditorViewportCommands::Get();

	TSharedRef<FEditorViewportClient> ClientRef = Client.ToSharedRef();

	CommandListRef.MapAction( 
		Commands.ToggleRealTime,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnToggleRealtime ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( this, &SEditorViewport::IsRealtime ) );

	CommandListRef.MapAction( 
		Commands.ToggleStats,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnToggleStats ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::ShouldShowStats ) );

	CommandListRef.MapAction( 
		Commands.ToggleFPS,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnToggleFPS ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::ShouldShowFPS ) );

	CommandListRef.MapAction(
		Commands.IncrementPositionGridSize,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnIncrementPositionGridSize )
		);

	CommandListRef.MapAction(
		Commands.DecrementPositionGridSize,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnDecrementPositionGridSize )
		);

	CommandListRef.MapAction(
		Commands.IncrementRotationGridSize,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnIncrementRotationGridSize )
		);

	CommandListRef.MapAction(
		Commands.DecrementRotationGridSize,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnDecrementRotationGridSize )
		);

	CommandListRef.MapAction( 
		Commands.Perspective,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetViewportType, LVT_Perspective ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::IsActiveViewportType, LVT_Perspective ) );

	CommandListRef.MapAction( 
		Commands.Front,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetViewportType, LVT_OrthoXZ ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::IsActiveViewportType, LVT_OrthoXZ ) );

	CommandListRef.MapAction( 
		Commands.Side,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetViewportType, LVT_OrthoYZ ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::IsActiveViewportType, LVT_OrthoYZ ) );

	CommandListRef.MapAction( 
		Commands.Top,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetViewportType, LVT_OrthoXY ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::IsActiveViewportType, LVT_OrthoXY ) );

	CommandListRef.MapAction(
		Commands.ScreenCapture,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnScreenCapture ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( this, &SEditorViewport::DoesAllowScreenCapture )
		);

	CommandListRef.MapAction(
		Commands.ScreenCaptureForProjectThumbnail,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnScreenCaptureForProjectThumbnail ),
		FCanExecuteAction(),
		FCanExecuteAction::CreateSP( this, &SEditorViewport::DoesAllowScreenCapture )
		);

	
	CommandListRef.MapAction(
		Commands.TranslateMode,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetMode, FWidget::WM_Translate ),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::CanSetWidgetMode, FWidget::WM_Translate ),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsWidgetModeActive, FWidget::WM_Translate ) 
		);

	CommandListRef.MapAction( 
		Commands.RotateMode,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetMode, FWidget::WM_Rotate ),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::CanSetWidgetMode, FWidget::WM_Rotate ),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsWidgetModeActive, FWidget::WM_Rotate )
		);
		

	CommandListRef.MapAction( 
		Commands.ScaleMode,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetMode, FWidget::WM_Scale ),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::CanSetWidgetMode, FWidget::WM_Scale ),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsWidgetModeActive, FWidget::WM_Scale )
		);

	CommandListRef.MapAction( 
		Commands.TranslateRotateMode,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetMode, FWidget::WM_TranslateRotateZ ),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::CanSetWidgetMode, FWidget::WM_TranslateRotateZ ),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsWidgetModeActive, FWidget::WM_TranslateRotateZ ),
		FIsActionButtonVisible::CreateSP( this, &SEditorViewport::IsTranslateRotateModeVisible )
		);

	CommandListRef.MapAction( 
		Commands.RelativeCoordinateSystem_World,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetCoordSystemSpace, COORD_World ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsCoordSystemActive, COORD_World )
		);

	CommandListRef.MapAction( 
		Commands.RelativeCoordinateSystem_Local,
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetWidgetCoordSystemSpace, COORD_Local ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SEditorViewport::IsCoordSystemActive, COORD_Local )
		);

	CommandListRef.MapAction(
		Commands.CycleTransformGizmos,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnCycleWidgetMode ),
		FCanExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::CanCycleWidgetMode )
		);

	CommandListRef.MapAction(
		Commands.CycleTransformGizmoCoordSystem,
		FExecuteAction::CreateSP( this, &SEditorViewport::OnCycleCoordinateSystem )
		);

							
	CommandListRef.MapAction( 
		Commands.FocusViewportToSelection, 
		FExecuteAction::CreateSP( this, &SEditorViewport::OnFocusViewportToSelection )
		//FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::ExecuteExecCommand, FString( TEXT("CAMERA ALIGN ACTIVEVIEWPORTONLY") ) )
		);


		// Simple macro for binding many Exposure Setting commands
#define MAP_EXPOSURE_ACTION( Command, ID ) \
	CommandListRef.MapAction( \
	Command, \
	FExecuteAction::CreateSP( this, &SEditorViewport::ChangeExposureSetting, ID ), \
	FCanExecuteAction(), \
	FIsActionChecked::CreateSP( this, &SEditorViewport::IsExposureSettingSelected, ID ) ) 

	MAP_EXPOSURE_ACTION( Commands.ToggleAutoExposure, FEditorViewportCommands::AutoExposureRadioID );
	MAP_EXPOSURE_ACTION( Commands.FixedExposure4m, -4);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure3m, -3);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure2m, -2);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure1m, -1);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure0, 0);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure1p, 1);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure2p, 2);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure3p, 3);
	MAP_EXPOSURE_ACTION( Commands.FixedExposure4p, 4);



		// Simple macro for binding many view mode UI commands
#define MAP_VIEWMODE_ACTION( ViewModeCommand, ViewModeID ) \
	CommandListRef.MapAction( \
		ViewModeCommand, \
		FExecuteAction::CreateSP( ClientRef, &FEditorViewportClient::SetViewMode, ViewModeID ), \
		FCanExecuteAction(), \
		FIsActionChecked::CreateSP( ClientRef, &FEditorViewportClient::IsViewModeEnabled, ViewModeID ) ) 

	// Map each view mode
	MAP_VIEWMODE_ACTION( Commands.WireframeMode, VMI_BrushWireframe );
	MAP_VIEWMODE_ACTION( Commands.UnlitMode, VMI_Unlit );
	MAP_VIEWMODE_ACTION( Commands.LitMode, VMI_Lit );
	MAP_VIEWMODE_ACTION( Commands.DetailLightingMode, VMI_Lit_DetailLighting );
	MAP_VIEWMODE_ACTION( Commands.LightingOnlyMode, VMI_LightingOnly );
	MAP_VIEWMODE_ACTION( Commands.LightComplexityMode, VMI_LightComplexity );
	MAP_VIEWMODE_ACTION( Commands.ShaderComplexityMode, VMI_ShaderComplexity );
	MAP_VIEWMODE_ACTION( Commands.StationaryLightOverlapMode, VMI_StationaryLightOverlap );
	MAP_VIEWMODE_ACTION( Commands.LightmapDensityMode, VMI_LightmapDensity );
	MAP_VIEWMODE_ACTION( Commands.ReflectionOverrideMode, VMI_ReflectionOverride );
	MAP_VIEWMODE_ACTION( Commands.VisualizeBufferMode, VMI_VisualizeBuffer );
	MAP_VIEWMODE_ACTION( Commands.CollisionPawn, VMI_CollisionPawn);
	MAP_VIEWMODE_ACTION( Commands.CollisionVisibility, VMI_CollisionVisibility);
}

EVisibility SEditorViewport::OnGetViewportContentVisibility() const
{
	return GEditorModeTools().IsViewportUIHidden() ? EVisibility::Collapsed : EVisibility::Visible;
}

void SEditorViewport::OnToggleRealtime()
{
	 Client->SetRealtime( ! Client->IsRealtime() );
}

void SEditorViewport::OnToggleStats()
{
	bool bIsEnabled =  Client->ShouldShowStats();
	 Client->SetShowStats( !bIsEnabled );

	if( !bIsEnabled )
	{
		// We cannot show stats unless realtime rendering is enabled
		 Client->SetRealtime( true );
	}
}

void SEditorViewport::OnToggleFPS()
{
	bool bIsEnabled = Client->ShouldShowFPS();
	Client->SetShowFPS( !bIsEnabled );
	if( !bIsEnabled )
	{
		// We cannot show stats unless realtime rendering is enabled
		 Client->SetRealtime( true );
	}
}

void SEditorViewport::ChangeExposureSetting( int32 ID )
{
	Client->ExposureSettings.bFixed = (ID != FEditorViewportCommands::AutoExposureRadioID);
	Client->ExposureSettings.LogOffset = ID;
}

bool SEditorViewport::IsExposureSettingSelected( int32 ID ) const
{
	if (ID == FEditorViewportCommands::AutoExposureRadioID)
	{
		return !Client->ExposureSettings.bFixed;
	}
	else
	{
		return Client->ExposureSettings.bFixed && Client->ExposureSettings.LogOffset == ID;
	}
}


bool SEditorViewport::IsRealtime() const
{
	return Client->IsRealtime();
}

void SEditorViewport::OnScreenCapture()
{
	Client->TakeScreenshot(Client->Viewport, true);
}

void SEditorViewport::OnScreenCaptureForProjectThumbnail()
{
	if ( FApp::HasGameName() )
	{
		const FString BaseFilename = FString(FApp::GetGameName()) + TEXT(".png");
		const FString ScreenshotFilename = FPaths::Combine(*FPaths::GameDir(), *BaseFilename);
		UThumbnailManager::CaptureProjectThumbnail(Client->Viewport, ScreenshotFilename, true);
	}
}

bool SEditorViewport::IsWidgetModeActive( FWidget::EWidgetMode Mode ) const
{
	return Client->GetWidgetMode() == Mode;
}

bool SEditorViewport::IsTranslateRotateModeVisible() const
{
	return GEditor->GetEditorUserSettings().bAllowTranslateRotateZWidget;
}

bool SEditorViewport::IsCoordSystemActive( ECoordSystem CoordSystem ) const
{
	return Client->GetWidgetCoordSystemSpace() == CoordSystem;
}

void SEditorViewport::OnCycleWidgetMode()
{
	FWidget::EWidgetMode WidgetMode = Client->GetWidgetMode();

	int32 WidgetModeAsInt = WidgetMode;

	do
	{
		++WidgetModeAsInt;

		if ((WidgetModeAsInt == FWidget::WM_TranslateRotateZ) && (!GEditor->GetEditorUserSettings().bAllowTranslateRotateZWidget))
		{
			++WidgetModeAsInt;
		}

		if( WidgetModeAsInt == FWidget::WM_Max )
		{
			WidgetModeAsInt -= FWidget::WM_Max;
		}
	}
	while( !Client->CanSetWidgetMode( (FWidget::EWidgetMode)WidgetModeAsInt ) && WidgetModeAsInt != WidgetMode );

	Client->SetWidgetMode( (FWidget::EWidgetMode)WidgetModeAsInt );
}

void SEditorViewport::OnCycleCoordinateSystem()
{
	int32 CoordSystemAsInt = Client->GetWidgetCoordSystemSpace();

	++CoordSystemAsInt;
	if( CoordSystemAsInt == COORD_Max )
	{
		CoordSystemAsInt -= COORD_Max;
	}

	Client->SetWidgetCoordSystemSpace( (ECoordSystem)CoordSystemAsInt );
}
