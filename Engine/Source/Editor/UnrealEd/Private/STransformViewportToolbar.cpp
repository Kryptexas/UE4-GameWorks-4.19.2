// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "STransformViewportToolbar.h"
#include "SLevelViewport.h"
#include "EditorViewportCommands.h"

#define LOCTEXT_NAMESPACE "TransformToolBar"

namespace TransformViewportToolbarDefs
{
	/** Size of the arrow shown on SGridSnapSettings Menu button */
	const float DownArrowSize = 4.0f;

	/** Size of the icon displayed on the toggle button of SGridSnapSettings */
	const float ToggleImageScale = 16.0f;
}

/**
 * Custom widget to display a toggle/drop down menu. 
 *	Displayed as shown:
 * +--------+-------------+
 * | Toggle | Menu button |
 * +--------+-------------+
 */
class SGridSnapSetting : public SCompoundWidget 
{
public:
	SLATE_BEGIN_ARGS( SGridSnapSetting ){}
	
		/** We need to know about the toolbar we are in */
		SLATE_ARGUMENT( TSharedPtr<class SViewportToolBar>, ParentToolBar );

		/** Content for the drop down menu  */
		SLATE_EVENT( FOnGetContent, OnGetMenuContent )

		/** Called upon state change with the value of the next state */
		SLATE_EVENT( FOnCheckStateChanged, OnCheckStateChanged )
			
		/** Sets the current checked state of the checkbox */
		SLATE_ATTRIBUTE( ESlateCheckBoxState::Type, IsChecked )

		/** Icon shown on the toggle button */
		SLATE_ATTRIBUTE( FSlateIcon, Icon )

		/** Label shown on the menu button */
		SLATE_ATTRIBUTE( FText, Label )

		/** Overall style */
		SLATE_ATTRIBUTE( FName, Style )

		/** ToolTip shown on the menu button */
		SLATE_ATTRIBUTE( FText, MenuButtonToolTip )

		/** ToolTip shown on the toggle button */
		SLATE_ATTRIBUTE( FText, ToggleButtonToolTip )

	SLATE_END_ARGS( )

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs )
	{
		FName ButtonStyle = FEditorStyle::Join(InArgs._Style.Get(), ".Button");
		FName CheckboxStyle = FEditorStyle::Join(InArgs._Style.Get(), ".ToggleButton");
	
		const FSlateIcon& Icon = InArgs._Icon.Get();

		ParentToolBar = InArgs._ParentToolBar;
	
		TSharedPtr<SCheckBox> ToggleControl;
		{
			ToggleControl = SNew(SCheckBox)
			.Cursor( EMouseCursor::Default )
			.Padding(FMargin( 4.0f ))
			.Style(FEditorStyle::Get(), EMultiBlockLocation::ToName(CheckboxStyle, EMultiBlockLocation::Start))
			.OnCheckStateChanged(InArgs._OnCheckStateChanged)
			.ToolTipText(InArgs._ToggleButtonToolTip)
			.IsChecked(InArgs._IsChecked)
			.Content()
			[
				SNew( SBox )
				.WidthOverride( TransformViewportToolbarDefs::ToggleImageScale )
				.HeightOverride( TransformViewportToolbarDefs::ToggleImageScale )
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(Icon.GetIcon())
				]
			];
		}

		{
			SAssignNew( MenuAnchor, SMenuAnchor )
			.Placement( MenuPlacement_BelowAnchor )
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), EMultiBlockLocation::ToName(ButtonStyle,EMultiBlockLocation::End) )
				.ContentPadding( FMargin( 5.0f, 0.0f ) )
				.ToolTipText(InArgs._MenuButtonToolTip)
				.OnClicked(this, &SGridSnapSetting::OnMenuClicked)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Font( FEditorStyle::GetFontStyle( InArgs._Style.Get(), ".Label.Font" ) )
						.Text(InArgs._Label)
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Bottom)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)

						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew( SBox )
							.WidthOverride( TransformViewportToolbarDefs::DownArrowSize )
							.HeightOverride( TransformViewportToolbarDefs::DownArrowSize )
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
								.ColorAndOpacity(FLinearColor::Black)
							]
						]

						+SHorizontalBox::Slot()
							.FillWidth(1.0f)
					]
				]
			]
			.OnGetMenuContent( InArgs._OnGetMenuContent );
		}


		ChildSlot
		[
			SNew(SHorizontalBox)

			//Checkbox concept
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				ToggleControl->AsShared()
			]

			// Black Seperator line
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( 1 )
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush(EMultiBlockLocation::ToName(ButtonStyle,EMultiBlockLocation::Middle)))
					.ColorAndOpacity(FLinearColor::Black)
				]
			]

			// Menu dropdown concept
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				MenuAnchor->AsShared()
			]
		];
	}
	
private:
	/**
	 * Called when the menu button is clicked.  Will toggle the visibility of the menu content                   
	 */
	FReply OnMenuClicked()
	{
		// If the menu button is clicked toggle the state of the menu anchor which will open or close the menu
		MenuAnchor->SetIsOpen( !MenuAnchor->IsOpen() );
		ParentToolBar.Pin()->SetOpenMenu( MenuAnchor );
		return FReply::Handled();
	}

	/**
	 * Called when the mouse enters a menu button.  If there was a menu previously opened
	 * we open this menu automatically
	 */
	void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		// See if there is another menu on the same tool bar already open
		TWeakPtr<SMenuAnchor> OpenedMenu = ParentToolBar.Pin()->GetOpenMenu();
		if( OpenedMenu.IsValid() && OpenedMenu.Pin()->IsOpen() && OpenedMenu.Pin() != MenuAnchor )
		{
			// There is another menu open so we open this menu and close the other
			ParentToolBar.Pin()->SetOpenMenu( MenuAnchor ); 
			MenuAnchor->SetIsOpen( true );
		}
	}

private:
	/** Our menus anchor */
	TSharedPtr<SMenuAnchor> MenuAnchor;

	/** Parent tool bar for querying other open menus */
	TWeakPtr<class SViewportToolBar> ParentToolBar;
};

/**
 * Custom widget to display an icon/drop down menu. 
 *	Displayed as shown:
 * +--------+-------------+
 * | Icon   | Menu button |
 * +--------+-------------+
 */
class SCameraSpeedSetting : public SCompoundWidget 
{
public:
	SLATE_BEGIN_ARGS( SCameraSpeedSetting ){}
	
		/** We need to know about the toolbar we are in */
		SLATE_ARGUMENT( TSharedPtr<class SViewportToolBar>, ParentToolBar );

		/** Content for the drop down menu  */
		SLATE_EVENT( FOnGetContent, OnGetMenuContent )

		/** Icon shown */
		SLATE_ATTRIBUTE( FSlateIcon, Icon )

		/** Label shown on the menu button */
		SLATE_ATTRIBUTE( FText, Label )

		/** Overall style */
		SLATE_ATTRIBUTE( FName, Style )

	SLATE_END_ARGS( )

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs )
	{
		ParentToolBar = InArgs._ParentToolBar;

		const FName ButtonStyle = FEditorStyle::Join(InArgs._Style.Get(), ".Button");

		const FSlateIcon& Icon = InArgs._Icon.Get();

		SAssignNew( MenuAnchor, SMenuAnchor )
		.Placement( MenuPlacement_BelowAnchor )
		.OnGetMenuContent( InArgs._OnGetMenuContent )
		[
			SNew(SButton)
			.ButtonStyle( FEditorStyle::Get(), ButtonStyle )
			.ContentPadding( FMargin( 5.0f, 0.0f ) )
			.OnClicked(this, &SCameraSpeedSetting::OnMenuClicked)
			[
				SNew(SHorizontalBox)

				// Icon
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride( TransformViewportToolbarDefs::ToggleImageScale )
					.HeightOverride( TransformViewportToolbarDefs::ToggleImageScale )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(Icon.GetIcon())
					]
				]

				// Spacer
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( FMargin( 4.0f, 0.0f ) )

				// Menu dropdown
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Font( FEditorStyle::GetFontStyle( InArgs._Style.Get(), ".Label.Font" ) )
						.Text(InArgs._Label)
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Bottom)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)

						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew( SBox )
							.WidthOverride( TransformViewportToolbarDefs::DownArrowSize )
							.HeightOverride( TransformViewportToolbarDefs::DownArrowSize )
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
								.ColorAndOpacity(FLinearColor::Black)
							]
						]

						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
					]
				]
			]
		];

		ChildSlot
		[
			MenuAnchor.ToSharedRef()
		];
	}
	
private:
	/**
	 * Called when the menu button is clicked.  Will toggle the visibility of the menu content                   
	 */
	FReply OnMenuClicked()
	{
		// If the menu button is clicked toggle the state of the menu anchor which will open or close the menu
		MenuAnchor->SetIsOpen( !MenuAnchor->IsOpen() );
		ParentToolBar.Pin()->SetOpenMenu( MenuAnchor );
		return FReply::Handled();
	}

	/**
	 * Called when the mouse enters a menu button.  If there was a menu previously opened
	 * we open this menu automatically
	 */
	void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		// See if there is another menu on the same tool bar already open
		TWeakPtr<SMenuAnchor> OpenedMenu = ParentToolBar.Pin()->GetOpenMenu();
		if( OpenedMenu.IsValid() && OpenedMenu.Pin()->IsOpen() && OpenedMenu.Pin() != MenuAnchor )
		{
			// There is another menu open so we open this menu and close the other
			ParentToolBar.Pin()->SetOpenMenu( MenuAnchor ); 
			MenuAnchor->SetIsOpen( true );
		}
	}

private:
	/** Our menus anchor */
	TSharedPtr<SMenuAnchor> MenuAnchor;

	/** Parent tool bar for querying other open menus */
	TWeakPtr<class SViewportToolBar> ParentToolBar;
};

void STransformViewportToolBar::Construct( const FArguments& InArgs )
{
	Viewport = InArgs._Viewport;
	CommandList = InArgs._CommandList;

	ChildSlot
	[
		MakeTransformToolBar(InArgs._Extenders)
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef< SWidget > STransformViewportToolBar::MakeTransformToolBar( const TSharedPtr< FExtender > InExtenders )
{
	FToolBarBuilder ToolbarBuilder( CommandList, FMultiBoxCustomization::None, InExtenders );

	// Use a custom style
	FName ToolBarStyle = "ViewportMenu";
	ToolbarBuilder.SetStyle(&FEditorStyle::Get(), ToolBarStyle);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	// Transform controls cannot be focusable as it fights with the press space to change transform mode feature
	ToolbarBuilder.SetIsFocusable( false );

	ToolbarBuilder.BeginSection("Transform");
	ToolbarBuilder.BeginBlockGroup();
	{
		// Move Mode
		static FName TranslateModeName = FName(TEXT("TranslateMode"));
		ToolbarBuilder.AddToolBarButton( FEditorViewportCommands::Get().TranslateMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), TranslateModeName );
		
		// TranslateRotate Mode
		static FName TranslateRotateModeName = FName(TEXT("TranslateRotateMode"));
		ToolbarBuilder.AddToolBarButton( FEditorViewportCommands::Get().TranslateRotateMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), TranslateRotateModeName );

		// Rotate Mode
		static FName RotateModeName = FName(TEXT("RotateMode"));
		ToolbarBuilder.AddToolBarButton( FEditorViewportCommands::Get().RotateMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), RotateModeName );

		// Scale Mode
		static FName ScaleModeName = FName(TEXT("ScaleMode"));
		ToolbarBuilder.AddToolBarButton( FEditorViewportCommands::Get().ScaleMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), ScaleModeName );
	}
	ToolbarBuilder.EndBlockGroup();
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.SetIsFocusable( true );
	
	ToolbarBuilder.BeginSection("LocalToWorld");
	ToolbarBuilder.BeginBlockGroup();
	{
		// Move Mode
		ToolbarBuilder.AddToolBarButton( FEditorViewportCommands::Get().CycleTransformGizmoCoordSystem,
										NAME_None,
										TAttribute<FText>(),
										TAttribute<FText>(),
										TAttribute<FSlateIcon>(this, &STransformViewportToolBar::GetLocalToWorldIcon),
										FName(TEXT("CycleTransformGizmoCoordSystem"))
										);
	}
	ToolbarBuilder.EndBlockGroup();
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.BeginSection("LocationGridSnap");
	{
		// Grab the existing UICommand 
		FUICommandInfo* Command = FEditorViewportCommands::Get().LocationGridSnap.Get();

		static FName PositionSnapName = FName(TEXT("PositionSnap"));

		// Setup a GridSnapSetting with the UICommand
		ToolbarBuilder.AddWidget(	SNew(SGridSnapSetting)
									.Style(ToolBarStyle)
									.Cursor( EMouseCursor::Default )
									.IsChecked(this, &STransformViewportToolBar::IsLocationGridSnapChecked)
									.OnCheckStateChanged(this, &STransformViewportToolBar::HandleToggleLocationGridSnap)
									.Label(this, &STransformViewportToolBar::GetLocationGridLabel)
									.OnGetMenuContent(this, &STransformViewportToolBar::FillLocationGridSnapMenu)
									.ToggleButtonToolTip(Command->GetDescription())
									.MenuButtonToolTip(LOCTEXT("LocationGridSnap_ToolTip", "Set the Position Grid Snap value"))
									.Icon(Command->GetIcon())
									.ParentToolBar(SharedThis(this))
									, PositionSnapName );
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("RotationGridSnap");
	{
		// Grab the existing UICommand 
		FUICommandInfo* Command = FEditorViewportCommands::Get().RotationGridSnap.Get();

		static FName RotationSnapName = FName(TEXT("RotationSnap"));

		// Setup a GridSnapSetting with the UICommand
		ToolbarBuilder.AddWidget(	SNew(SGridSnapSetting)
									.Cursor( EMouseCursor::Default )
									.Style(ToolBarStyle)
									.IsChecked(this, &STransformViewportToolBar::IsRotationGridSnapChecked)
									.OnCheckStateChanged(this, &STransformViewportToolBar::HandleToggleRotationGridSnap)
									.Label(this, &STransformViewportToolBar::GetRotationGridLabel)
									.OnGetMenuContent(this ,&STransformViewportToolBar::FillRotationGridSnapMenu)
									.ToggleButtonToolTip(Command->GetDescription())
									.MenuButtonToolTip(LOCTEXT("RotationGridSnap_ToolTip", "Set the Rotation Grid Snap value"))
									.Icon(Command->GetIcon())
									.ParentToolBar(SharedThis(this))
									, RotationSnapName );
	}
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.BeginSection("ScaleGridSnap");
	{
		// Grab the existing UICommand 
		FUICommandInfo* Command = FEditorViewportCommands::Get().ScaleGridSnap.Get();

		static FName ScaleSnapName = FName(TEXT("ScaleSnap"));

		// Setup a GridSnapSetting with the UICommand
		ToolbarBuilder.AddWidget(	SNew(SGridSnapSetting)
									.Cursor( EMouseCursor::Default )
									.Style(ToolBarStyle)
									.IsChecked(this,&STransformViewportToolBar::IsScaleGridSnapChecked)
									.OnCheckStateChanged(this, &STransformViewportToolBar::HandleToggleScaleGridSnap)
									.Label(this ,&STransformViewportToolBar::GetScaleGridLabel)
									.OnGetMenuContent(this, &STransformViewportToolBar::FillScaleGridSnapMenu)
									.ToggleButtonToolTip(Command->GetDescription())
									.MenuButtonToolTip(LOCTEXT("ScaleGridSnap_ToolTip", "Set the Scale Grid Snap value"))
									.Icon(Command->GetIcon())
									.ParentToolBar(SharedThis(this))
									, ScaleSnapName );
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("CameraSpeed");
	ToolbarBuilder.BeginBlockGroup();
	{
		// Camera speed 
		ToolbarBuilder.AddWidget(	SNew(SCameraSpeedSetting)
									.Cursor(EMouseCursor::Default)
									.Style(ToolBarStyle)
									.Label(this, &STransformViewportToolBar::GetCameraSpeedLabel)
									.OnGetMenuContent(this, &STransformViewportToolBar::FillCameraSpeedMenu)
									.ToolTipText(LOCTEXT("CameraSpeed_ToolTip","Camera Speed"))
									.Icon(FSlateIcon(FEditorStyle::GetStyleSetName(), "EditorViewport.CamSpeedSetting"))
									.ParentToolBar(SharedThis(this))
									);
	}
	ToolbarBuilder.EndBlockGroup();
	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}


TSharedRef<SWidget> STransformViewportToolBar::FillCameraSpeedMenu()
{
	TSharedRef<SWidget> ReturnWidget = SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Background")))
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 2.0f, 60.0f, 2.0f) )
		.HAlign( HAlign_Left )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("MouseSettingsCamSpeed", "Camera Speed")  )
			.Font( FEditorStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 4.0f) )
		[	
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding( FMargin(0.0f, 2.0f) )
			[
				SAssignNew(CamSpeedSlider, SSlider)
				.Value(this, &STransformViewportToolBar::GetCamSpeedSliderPosition)
				.OnValueChanged(this, &STransformViewportToolBar::OnSetCamSpeed)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 8.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew( STextBlock )
				.Text(this, &STransformViewportToolBar::GetCameraSpeedLabel )
				.Font( FEditorStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
			]
		]
	];

	return ReturnWidget;
}

FSlateIcon STransformViewportToolBar::GetLocalToWorldIcon() const
{
	if( Viewport.IsValid() && Viewport.Pin()->IsCoordSystemActive(COORD_World) )
	{
		static FName WorldIcon("EditorViewport.RelativeCoordinateSystem_World");
		return FSlateIcon(FEditorStyle::GetStyleSetName(), WorldIcon);
	}

	static FName LocalIcon("EditorViewport.RelativeCoordinateSystem_Local");
	return FSlateIcon(FEditorStyle::GetStyleSetName(), LocalIcon);
}

FText STransformViewportToolBar::GetLocationGridLabel() const
{
	return FText::AsNumber( GEditor->GetGridSize() );
}

FText STransformViewportToolBar::GetRotationGridLabel() const
{
	return FText::Format( LOCTEXT("GridRotation - Number - DegreeSymbol", "{0}\u00b0"), FText::AsNumber( GEditor->GetRotGridSize().Pitch ) );
}

FText STransformViewportToolBar::GetScaleGridLabel() const
{
	return FText::AsNumber( GEditor->GetScaleGridSize() );
}

FText STransformViewportToolBar::GetCameraSpeedLabel() const
{
	return FText::AsNumber( GetDefault<ULevelEditorViewportSettings>()->CameraSpeed );
}	

float STransformViewportToolBar::GetCamSpeedSliderPosition() const
{
	const int32 SpeedSetting = GetDefault<ULevelEditorViewportSettings>()->CameraSpeed;	
	float SliderPos = (SpeedSetting - 1) / ((float)FEditorViewportClient::MaxCameraSpeeds - 1);
	return SliderPos;
}

void STransformViewportToolBar::OnSetCamSpeed(float NewValue)
{
	int32 SpeedSetting = NewValue * ((float)FEditorViewportClient::MaxCameraSpeeds - 1) + 1;
	GetMutableDefault<ULevelEditorViewportSettings>()->CameraSpeed = SpeedSetting;
}

/**
	* Sets our grid size based on what the user selected in the UI
	*
	* @param	InIndex		The new index of the grid size to use
	*/
void STransformViewportToolBar::SetGridSize( int32 InIndex )
{
	GEditor->SetGridSize( InIndex );
}


/**
	* Sets the rotation grid size
	*
	* @param	InIndex		The new index of the rotation grid size to use
	* @param	InGridMode	Controls whether to use Preset or User selected values
	*/
void STransformViewportToolBar::SetRotationGridSize( int32 InIndex, ERotationGridMode InGridMode )
{
	GEditor->SetRotGridSize( InIndex, InGridMode );
}

		
/**
	* Sets the scale grid size
	*
	* @param	InIndex	The new index of the scale grid size to use
	*/
void STransformViewportToolBar::SetScaleGridSize( int32 InIndex )
{
	GEditor->SetScaleGridSize( InIndex );
}

		
/**
	* Checks to see if the specified grid size index is the current grid size index
	*
	* @param	GridSizeIndex	The grid size index to test
	*
	* @return	True if the specified grid size index is the current one
	*/
bool STransformViewportToolBar::IsGridSizeChecked( int32 GridSizeIndex )
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
	return (ViewportSettings->CurrentPosGridSize == GridSizeIndex);
}


/**
	* Checks to see if the specified rotation grid angle is the current rotation grid angle
	*
	* @param	GridSizeIndex	The grid size index to test
	* @param	InGridMode		Controls whether to use Preset or User selected values
	*
	* @return	True if the specified rotation grid size angle is the current one
	*/
bool STransformViewportToolBar::IsRotationGridSizeChecked( int32 GridSizeIndex, ERotationGridMode GridMode )
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
	return (ViewportSettings->CurrentRotGridSize == GridSizeIndex) && (ViewportSettings->CurrentRotGridMode == GridMode);
}


/**
	* Checks to see if the specified scale grid size is the current scale grid size
	*
	* @param	GridSizeIndex	The grid size index to test
	*
	* @return	True if the specified scale grid size is the current one
	*/
bool STransformViewportToolBar::IsScaleGridSizeChecked( int32 GridSizeIndex )
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
	return (ViewportSettings->CurrentScalingGridSize == GridSizeIndex);
}

void STransformViewportToolBar::TogglePreserveNonUniformScale()
{
	ULevelEditorViewportSettings* ViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
	ViewportSettings->PreserveNonUniformScale = !ViewportSettings->PreserveNonUniformScale;
}

bool STransformViewportToolBar::IsPreserveNonUniformScaleChecked()
{
	return GetDefault<ULevelEditorViewportSettings>()->PreserveNonUniformScale;
}

TSharedRef<SWidget> STransformViewportToolBar::FillLocationGridSnapMenu()
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

	return BuildLocationGridCheckBoxList("Snap", LOCTEXT("LocationSnapText", "Snap Sizes"), ViewportSettings->bUsePowerOf2SnapSize ? ViewportSettings->Pow2GridSizes : ViewportSettings->DecimalGridSizes );
}

TSharedRef<SWidget> STransformViewportToolBar::BuildLocationGridCheckBoxList(FName InExtentionHook, const FText& InHeading, const TArray<float>& InGridSizes) const
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder LocationGridMenuBuilder( bShouldCloseWindowAfterMenuSelection, CommandList );

	LocationGridMenuBuilder.BeginSection(InExtentionHook, InHeading);
	for( int32 CurGridSizeIndex = 0; CurGridSizeIndex < InGridSizes.Num(); ++CurGridSizeIndex )
	{
		const float CurGridSize = InGridSizes[ CurGridSizeIndex ];

		LocationGridMenuBuilder.AddMenuEntry(
			FText::AsNumber( CurGridSize ),
			FText::Format( LOCTEXT("LocationGridSize_ToolTip", "Sets grid size to {0}"), FText::AsNumber( CurGridSize ) ),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateStatic( &STransformViewportToolBar::SetGridSize, CurGridSizeIndex ),
			FCanExecuteAction(),
			FIsActionChecked::CreateStatic( &STransformViewportToolBar::IsGridSizeChecked, CurGridSizeIndex ) ),
			NAME_None,
			EUserInterfaceActionType::RadioButton );
	}
	LocationGridMenuBuilder.EndSection();

	return LocationGridMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> STransformViewportToolBar::FillRotationGridSnapMenu()
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();

	return SNew(SUniformGridPanel)

	+ SUniformGridPanel::Slot(0, 0)
		[
			BuildRotationGridCheckBoxList("Common", LOCTEXT("RotationCommonText", "Common"), ViewportSettings->CommonRotGridSizes, GridMode_Common)
		]

	+ SUniformGridPanel::Slot(1, 0)
		[
			BuildRotationGridCheckBoxList("Div360", LOCTEXT("RotationDivisions360DegreesText", "Divisions of 360\u00b0"), ViewportSettings->DivisionsOf360RotGridSizes, GridMode_DivisionsOf360)
		];
}

TSharedRef<SWidget> STransformViewportToolBar::BuildRotationGridCheckBoxList(FName InExtentionHook, const FText& InHeading, const TArray<float>& InGridSizes, ERotationGridMode InGridMode) const
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder RotationGridMenuBuilder( bShouldCloseWindowAfterMenuSelection, CommandList );

	RotationGridMenuBuilder.BeginSection(InExtentionHook, InHeading);
	for( int32 CurGridAngleIndex = 0; CurGridAngleIndex < InGridSizes.Num(); ++CurGridAngleIndex )
	{
		const float CurGridAngle = InGridSizes[ CurGridAngleIndex ];

		FText MenuName = FText::Format( LOCTEXT("RotationGridAngle", "{0}\u00b0"), FText::AsNumber( CurGridAngle ) ); /*degree symbol*/
		FText ToolTipText = FText::Format( LOCTEXT("RotationGridAngle_ToolTip", "Sets rotation grid angle to {0}"), MenuName ) ; /*degree symbol*/

		RotationGridMenuBuilder.AddMenuEntry(
			MenuName,
			ToolTipText,
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateStatic( &STransformViewportToolBar::SetRotationGridSize, CurGridAngleIndex, InGridMode ),
			FCanExecuteAction(),
			FIsActionChecked::CreateStatic( &STransformViewportToolBar::IsRotationGridSizeChecked, CurGridAngleIndex, InGridMode ) ),
			NAME_None,
			EUserInterfaceActionType::RadioButton );
	}
	RotationGridMenuBuilder.EndSection();

	return RotationGridMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> STransformViewportToolBar::FillScaleGridSnapMenu()
{
	const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
	const bool bShouldCloseWindowAfterMenuSelection = true;

	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MaximumFractionalDigits = 5;

	FMenuBuilder ScaleGridMenuBuilder( bShouldCloseWindowAfterMenuSelection, CommandList                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   );

	for( int32 CurGridAmountIndex = 0; CurGridAmountIndex < ViewportSettings->ScalingGridSizes.Num(); ++CurGridAmountIndex )
	{
		const float CurGridAmount = ViewportSettings->ScalingGridSizes[ CurGridAmountIndex ];

		FText MenuText;
		FText ToolTipText;

		if( GEditor->UsePercentageBasedScaling() )
		{
			MenuText = FText::AsPercent( CurGridAmount / 100 );
			ToolTipText = FText::Format( LOCTEXT("ScaleGridAmountOld_ToolTip", "Snaps scale values to {0}"), MenuText );
		}
		else
		{
			MenuText = FText::AsNumber( CurGridAmount, &NumberFormattingOptions );
			ToolTipText = FText::Format( LOCTEXT("ScaleGridAmount_ToolTip", "Snaps scale values to increments of {0}"), MenuText );
		}

		ScaleGridMenuBuilder.AddMenuEntry(
			MenuText,
			ToolTipText,
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateStatic( &STransformViewportToolBar::SetScaleGridSize, CurGridAmountIndex ),
			FCanExecuteAction(),
			FIsActionChecked::CreateStatic( &STransformViewportToolBar::IsScaleGridSizeChecked, CurGridAmountIndex ) ),
			NAME_None,
			EUserInterfaceActionType::RadioButton );
	}

	if( !GEditor->UsePercentageBasedScaling() )
	{
		ScaleGridMenuBuilder.AddMenuSeparator();

		ScaleGridMenuBuilder.AddMenuEntry(
			LOCTEXT("ScaleGridPreserveNonUniformScale", "Preserve Non-Uniform Scale"),
			LOCTEXT("ScaleGridPreserveNonUniformScale_ToolTip", "When this option is checked, scaling objects that have a non-uniform scale will preserve the ratios between each axis, snapping the axis with the largest value."),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateStatic( &STransformViewportToolBar::TogglePreserveNonUniformScale ),
			FCanExecuteAction(),
			FIsActionChecked::CreateStatic( &STransformViewportToolBar::IsPreserveNonUniformScaleChecked ) ),
			NAME_None,
			EUserInterfaceActionType::Check );
	}


	return ScaleGridMenuBuilder.MakeWidget();
}

ESlateCheckBoxState::Type STransformViewportToolBar::IsLocationGridSnapChecked() const
{
	return GetDefault<ULevelEditorViewportSettings>()->GridEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

ESlateCheckBoxState::Type STransformViewportToolBar::IsRotationGridSnapChecked() const
{
	return GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

ESlateCheckBoxState::Type STransformViewportToolBar::IsScaleGridSnapChecked() const
{
	return GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STransformViewportToolBar::HandleToggleLocationGridSnap( ESlateCheckBoxState::Type InState )
{
	GUnrealEd->Exec( GEditor->GetEditorWorldContext().World(), *FString::Printf( TEXT("MODE GRID=%d"), !GetDefault<ULevelEditorViewportSettings>()->GridEnabled ? 1 : 0 ) );
}

void STransformViewportToolBar::HandleToggleRotationGridSnap( ESlateCheckBoxState::Type InState )
{
	GUnrealEd->Exec( GEditor->GetEditorWorldContext().World(), *FString::Printf( TEXT("MODE ROTGRID=%d"), !GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled ? 1 : 0 ) );
}

void STransformViewportToolBar::HandleToggleScaleGridSnap( ESlateCheckBoxState::Type InState )
{
	GUnrealEd->Exec( GEditor->GetEditorWorldContext().World(), *FString::Printf( TEXT("MODE SCALEGRID=%d"), !GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled ? 1 : 0 ) );
}

#undef LOCTEXT_NAMESPACE 