// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "SAnimViewportToolBar.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EngineGlobals.h"
#include "AssetData.h"
#include "Engine/Engine.h"
#include "EditorStyleSet.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Preferences/PersonaOptions.h"
#include "EditorViewportCommands.h"
#include "AnimViewportMenuCommands.h"
#include "AnimViewportShowCommands.h"
#include "AnimViewportLODCommands.h"
#include "AnimViewportPlaybackCommands.h"
#include "SAnimPlusMinusSlider.h"
#include "SEditorViewportToolBarMenu.h"
#include "STransformViewportToolbar.h"
#include "SEditorViewportViewMenu.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "AssetViewerSettings.h"
#include "PersonaPreviewSceneDescription.h"
#include "Engine/PreviewMeshCollection.h"
#include "PreviewSceneCustomizations.h"
#include "ClothingSimulation.h"
#include "SimulationEditorExtender.h"
#include "ClothingSimulationFactoryInterface.h"
#include "ClothingSystemEditorInterfaceModule.h"
#include "Widgets/SWidget.h"
#include "ISlateMetaData.h"
#include "Textures/SlateIcon.h"
#include "ShowFlagMenuCommands.h"
#include "BufferVisualizationMenuCommands.h"
#include "IPinnedCommandList.h"
#include "UICommandList_Pinnable.h"
#include "BoneSelectionWidget.h"
#include "SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "AnimViewportToolBar"

//Class definition which represents widget to modify strength of wind for clothing
class SClothWindSettings : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SClothWindSettings)
	{}

		SLATE_ARGUMENT(TWeakPtr<SAnimationEditorViewportTabBody>, AnimEditorViewport)
	SLATE_END_ARGS()

	/** Constructs this widget from its declaration */
	void Construct(const FArguments& InArgs )
	{
		AnimViewportPtr = InArgs._AnimEditorViewport;

		this->ChildSlot
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
				.WidthOverride(100.0f)
				[
					SNew(SNumericEntryBox<float>)
					.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
					.ToolTipText(LOCTEXT("WindStrength_ToolTip", "Change wind strength"))
					.MinValue(0)
					.AllowSpin(true)
					.MinSliderValue(0)
					.MaxSliderValue(2)
					.Value(AnimViewportPtr.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::GetWindStrengthSliderValue)
					.OnValueChanged(SSpinBox<float>::FOnValueChanged::CreateSP(AnimViewportPtr.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::SetWindStrength))
				]
			]
		];
	}

protected:

	/** Callback function which determines whether this widget is enabled */
	bool IsWindEnabled() const
	{
		return AnimViewportPtr.Pin()->IsApplyingClothWind();
	}

protected:
	/** The viewport hosting this widget */
	TWeakPtr<SAnimationEditorViewportTabBody> AnimViewportPtr;
};

//Class definition which represents widget to modify gravity for preview
class SGravitySettings : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SGravitySettings)
	{}

		SLATE_ARGUMENT(TWeakPtr<SAnimationEditorViewportTabBody>, AnimEditorViewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs )
	{
		AnimViewportPtr = InArgs._AnimEditorViewport;

		this->ChildSlot
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
				.WidthOverride(100.0f)
				[
					SNew(SSpinBox<float>)
					.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
					.ToolTipText(LOCTEXT("GravityScale_ToolTip", "Change gravity scale"))
					.MinValue(0)
					.MaxValue(4)
					.Value(AnimViewportPtr.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::GetGravityScaleSliderValue)
					.OnValueChanged(SSpinBox<float>::FOnValueChanged::CreateSP(AnimViewportPtr.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::SetGravityScale))
				]
			]
		];
	}

protected:
	FReply OnDecreaseGravityScale()
	{
		const float DeltaValue = 0.025f;
		AnimViewportPtr.Pin()->SetGravityScale( AnimViewportPtr.Pin()->GetGravityScaleSliderValue() - DeltaValue );
		return FReply::Handled();
	}

	FReply OnIncreaseGravityScale()
	{
		const float DeltaValue = 0.025f;
		AnimViewportPtr.Pin()->SetGravityScale( AnimViewportPtr.Pin()->GetGravityScaleSliderValue() + DeltaValue );
		return FReply::Handled();
	}

protected:
	TWeakPtr<SAnimationEditorViewportTabBody> AnimViewportPtr;
};

///////////////////////////////////////////////////////////
// SAnimViewportToolBar

void SAnimViewportToolBar::Construct(const FArguments& InArgs, TSharedPtr<class SAnimationEditorViewportTabBody> InViewport, TSharedPtr<class SEditorViewport> InRealViewport)
{
	bShowShowMenu = InArgs._ShowShowMenu;
	bShowCharacterMenu= InArgs._ShowCharacterMenu;
	bShowLODMenu = InArgs._ShowLODMenu;
	bShowPlaySpeedMenu = InArgs._ShowPlaySpeedMenu;
	bShowFloorOptions = InArgs._ShowFloorOptions;
	bShowTurnTable = InArgs._ShowTurnTable;
	bShowPhysicsMenu = InArgs._ShowPhysicsMenu;

	CommandList = InRealViewport->GetCommandList();
	Extenders = InArgs._Extenders;
	Extenders.Add(GetViewMenuExtender(InRealViewport));

	// If we have no extender, make an empty one
	if (Extenders.Num() == 0)
	{
		Extenders.Add(MakeShared<FExtender>());
	}

	const FMargin ToolbarSlotPadding(2.0f, 2.0f);
	const FMargin ToolbarButtonPadding(2.0f, 0.0f);

	static const FName DefaultForegroundName("DefaultForeground");


	TSharedRef<SHorizontalBox> LeftToolbar = SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("ViewMenuTooltip", "View Options.\nShift-clicking items will 'pin' them to the toolbar."))
			.ParentToolBar(SharedThis(this))
			.Cursor(EMouseCursor::Default)
			.Image("EditorViewportToolBar.MenuDropdown")
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.MenuDropdown")))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GenerateViewMenu)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("ViewportMenuTooltip", "Viewport Options. Use this to switch between different orthographic or perspective views."))
			.ParentToolBar(SharedThis(this))
			.Cursor(EMouseCursor::Default)
			.Label(this, &SAnimViewportToolBar::GetCameraMenuLabel)
			.LabelIcon(this, &SAnimViewportToolBar::GetCameraMenuLabelIcon)
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("EditorViewportToolBar.CameraMenu")))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GenerateViewportTypeMenu)
		]
		// View menu (lit, unlit, etc...)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportViewMenu, InRealViewport.ToSharedRef(), SharedThis(this))
			.ToolTipText(LOCTEXT("ViewModeMenuTooltip", "View Mode Options. Use this to change how the view is rendered, e.g. Lit/Unlit."))
			.MenuExtenders(FExtender::Combine(Extenders))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("ShowMenuTooltip", "Show Options. Use this enable/disable the rendering of types of scene elements."))
			.ParentToolBar(SharedThis(this))
			.Cursor(EMouseCursor::Default)
			.Label(LOCTEXT("ShowMenu", "Show"))
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ViewMenuButton")))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GenerateShowMenu)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("PhysicsMenuTooltip", "Physics Options. Use this to control the physics of the scene."))
			.ParentToolBar(SharedThis(this))
			.Label(LOCTEXT("Physics", "Physics"))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GeneratePhysicsMenu)
			.Visibility(bShowPhysicsMenu ? EVisibility::Visible : EVisibility::Collapsed)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("CharacterMenuTooltip", "Character Options. Control character-related rendering options.\nShift-clicking items will 'pin' them to the toolbar."))
			.ParentToolBar(SharedThis(this))
			.Label(LOCTEXT("Character", "Character"))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GenerateCharacterMenu)
			.Visibility(bShowCharacterMenu ? EVisibility::Visible : EVisibility::Collapsed)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			//LOD
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("LODMenuTooltip", "LOD Options. Control how LODs are displayed.\nShift-clicking items will 'pin' them to the toolbar."))
			.ParentToolBar(SharedThis(this))
			.Label(this, &SAnimViewportToolBar::GetLODMenuLabel)
			.OnGetMenuContent(this, &SAnimViewportToolBar::GenerateLODMenu)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(ToolbarSlotPadding)
		[
			SNew(SEditorViewportToolbarMenu)
			.ToolTipText(LOCTEXT("PlaybackSpeedMenuTooltip", "Playback Speed Options. Control the time dilation of the scene's update.\nShift-clicking items will 'pin' them to the toolbar."))
			.ParentToolBar(SharedThis(this))
			.Label(this, &SAnimViewportToolBar::GetPlaybackMenuLabel)
			.LabelIcon(FEditorStyle::GetBrush("AnimViewportMenu.PlayBackSpeed"))
			.OnGetMenuContent(this, &SAnimViewportToolBar::GeneratePlaybackMenu)
		]
		+ SHorizontalBox::Slot()
		.Padding(ToolbarSlotPadding)
		.HAlign(HAlign_Right)
		[
			SNew(STransformViewportToolBar)
			.Viewport(InRealViewport)
			.CommandList(InRealViewport->GetCommandList())
			.Visibility(this, &SAnimViewportToolBar::GetTransformToolbarVisibility)
		];
			
	
	//@TODO: Need clipping horizontal box: LeftToolbar->AddWrapButton();

	// Create our pinned commands before we bind commands
	IPinnedCommandListModule& PinnedCommandListModule = FModuleManager::LoadModuleChecked<IPinnedCommandListModule>(TEXT("PinnedCommandList"));
	PinnedCommands = PinnedCommandListModule.CreatePinnedCommandList(InArgs._ContextName != NAME_None ? InArgs._ContextName : TEXT("PersonaViewport"));
	PinnedCommands->SetStyle(&FEditorStyle::Get(), TEXT("ViewportPinnedCommandList"));

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush("NoBorder") )
		// Color and opacity is changed based on whether or not the mouse cursor is hovering over the toolbar area
		.ColorAndOpacity( this, &SViewportToolBar::OnGetColorAndOpacity )
		.ForegroundColor( FEditorStyle::GetSlateColor(DefaultForegroundName) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				LeftToolbar
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				PinnedCommands.ToSharedRef()
			]
			+SVerticalBox::Slot()
			.Padding(FMargin(4.0f, 3.0f, 0.0f, 0.0f))
			[
				// Display text (e.g., item being previewed)
				SNew(SRichTextBlock)
				.DecoratorStyleSet(&FEditorStyle::Get())
				.Text(InViewport.Get(), &SAnimationEditorViewportTabBody::GetDisplayString)
				.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("AnimViewport.MessageText"))
			]
		]
	];
	
	SViewportToolBar::Construct(SViewportToolBar::FArguments());

	// Register all the custom widgets we can use here
	PinnedCommands->RegisterCustomWidget(IPinnedCommandList::FOnGenerateCustomWidget::CreateSP(this, &SAnimViewportToolBar::MakeFloorOffsetWidget), TEXT("FloorOffsetWidget"), LOCTEXT("FloorHeightOffset", "Floor Height Offset"));
	PinnedCommands->RegisterCustomWidget(IPinnedCommandList::FOnGenerateCustomWidget::CreateSP(this, &SAnimViewportToolBar::MakeFOVWidget), TEXT("FOVWidget"), LOCTEXT("Viewport_FOVLabel", "Field Of View"));
	PinnedCommands->BindCommandList(InViewport->GetCommandList().ToSharedRef());

	// We assign the viewport pointer her rather that initially, as SViewportToolbar::Construct 
	// ends up calling through and attempting to perform operations on the not-yet-full-constructed viewport
	Viewport = InViewport;
}

EVisibility SAnimViewportToolBar::GetTransformToolbarVisibility() const
{
	return Viewport.Pin()->CanUseGizmos() ? EVisibility::Visible : EVisibility::Hidden;
}

TSharedRef<SWidget> SAnimViewportToolBar::MakeFloorOffsetWidget() const
{
	return
		SNew(SBox)
		.HAlign(HAlign_Right)
		[
			SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			.WidthOverride(100.0f)
			[
				SNew(SNumericEntryBox<float>)
				.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
				.AllowSpin(true)
				.MinSliderValue(-100.0f)
				.MaxSliderValue(100.0f)
				.Value(this, &SAnimViewportToolBar::OnGetFloorOffset)
				.OnValueChanged(this, &SAnimViewportToolBar::OnFloorOffsetChanged)
				.ToolTipText(LOCTEXT("FloorOffsetToolTip", "Height offset for the floor mesh (stored per-mesh)"))
			]
		];
}

TSharedRef<SWidget> SAnimViewportToolBar::MakeFOVWidget() const
{
	const float FOVMin = 5.f;
	const float FOVMax = 170.f;

	return
		SNew(SBox)
		.HAlign(HAlign_Right)
		[
			SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			.WidthOverride(100.0f)
			[
				SNew(SNumericEntryBox<float>)
				.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
				.AllowSpin(true)
				.MinValue(FOVMin)
				.MaxValue(FOVMax)
				.MinSliderValue(FOVMin)
				.MaxSliderValue(FOVMax)
				.Value(this, &SAnimViewportToolBar::OnGetFOVValue)
				.OnValueChanged(this, &SAnimViewportToolBar::OnFOVValueChanged)
				.OnValueCommitted(this, &SAnimViewportToolBar::OnFOVValueCommitted)
			]
		];
}

TSharedRef<SWidget> SAnimViewportToolBar::GenerateViewMenu() const
{
	const FAnimViewportMenuCommands& Actions = FAnimViewportMenuCommands::Get();

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList(), MenuExtender);

	InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());

	InMenuBuilder.BeginSection("AnimViewportSceneSetup", LOCTEXT("ViewMenu_SceneSetupLabel", "Scene Setup"));
	{
		InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
		InMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().PreviewSceneSettings);
		InMenuBuilder.PopCommandList();

		if(bShowFloorOptions)
		{
			InMenuBuilder.AddWidget(MakeFloorOffsetWidget(), LOCTEXT("FloorHeightOffset", "Floor Height Offset"));

			InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
			InMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().AutoAlignFloorToMesh);
			InMenuBuilder.PopCommandList();
		}
			
		if (bShowTurnTable)
		{
			InMenuBuilder.AddSubMenu(
				LOCTEXT("TurnTableLabel", "Turn Table"),
				LOCTEXT("TurnTableTooltip", "Set up auto-rotation of preview."),
				FNewMenuDelegate::CreateRaw(this, &SAnimViewportToolBar::GenerateTurnTableMenu),
				false,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "AnimViewportMenu.TurnTableSpeed")
				);
		}
	}
	InMenuBuilder.EndSection();

	InMenuBuilder.BeginSection("AnimViewportCamera", LOCTEXT("ViewMenu_CameraLabel", "Camera"));
	{
		InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().FocusViewportToSelection);

		InMenuBuilder.AddSubMenu(
			LOCTEXT("CameraFollowModeLabel", "Camera Follow Mode"),
			LOCTEXT("CameraFollowModeTooltip", "Set various camera follow modes"),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InSubMenuBuilder)
			{
				InSubMenuBuilder.BeginSection("AnimViewportCameraFollowMode", LOCTEXT("ViewMenu_CameraFollowModeLabel", "Camera Follow Mode"));
				{
					InSubMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());

					InSubMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().CameraFollowNone);
					InSubMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().CameraFollowBounds);

					InSubMenuBuilder.PopCommandList();
				}
				InSubMenuBuilder.EndSection();
		
				InSubMenuBuilder.BeginSection("AnimViewportCameraFollowBone", FText());
				{
					InSubMenuBuilder.AddWidget(
						SNew(SBox)
						.MaxDesiredHeight(400.0f)
						[
							SNew(SBoneTreeMenu)
							.Title(FAnimViewportMenuCommands::Get().CameraFollowBone->GetLabel())
							.bShowVirtualBones(true)
							.OnBoneSelectionChanged_Lambda([this](FName InBoneName)
							{
								Viewport.Pin()->SetCameraFollowMode(EAnimationViewportCameraFollowMode::Bone, InBoneName);
								FSlateApplication::Get().DismissAllMenus();
							})
							.SelectedBone(Viewport.Pin()->GetCameraFollowBoneName())
							.OnGetReferenceSkeleton_Lambda([this]() -> const FReferenceSkeleton&
							{
								return Viewport.Pin()->GetPreviewScene()->GetPersonaToolkit()->GetEditableSkeleton()->GetSkeleton().GetReferenceSkeleton();
							})
						],
						FText(), 
						true);
				}
				InSubMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon(FEditorStyle::GetStyleSetName(), "AnimViewportMenu.CameraFollow")
			);

		InMenuBuilder.AddWidget(MakeFOVWidget(), LOCTEXT("Viewport_FOVLabel", "Field Of View"));
	}
	InMenuBuilder.EndSection();

	InMenuBuilder.BeginSection("AnimViewportDefaultCamera", LOCTEXT("ViewMenu_DefaultCameraLabel", "Default Camera"));
	{
		InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
		InMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().JumpToDefaultCamera);
		InMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().SaveCameraAsDefault);
		InMenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().ClearDefaultCamera);
		InMenuBuilder.PopCommandList();
	}
	InMenuBuilder.EndSection();

	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();

	return InMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAnimViewportToolBar::GeneratePhysicsMenu() const
{
	const FAnimViewportShowCommands& Actions = FAnimViewportShowCommands::Get();

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList(), MenuExtender);

	InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());
	{
		InMenuBuilder.BeginSection("AnimViewportPhysicsMenu", LOCTEXT("ViewMenu_AnimViewportPhysicsMenu", "Physics Menu"));
		InMenuBuilder.EndSection();
	}
	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();
	return InMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAnimViewportToolBar::GenerateCharacterMenu() const
{
	const FAnimViewportShowCommands& Actions = FAnimViewportShowCommands::Get();

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList(), MenuExtender);

	InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());

	{
		InMenuBuilder.BeginSection("AnimViewportSceneElements", LOCTEXT("CharacterMenu_SceneElements", "Scene Elements"));
		{
			InMenuBuilder.AddSubMenu(
				LOCTEXT("CharacterMenu_MeshSubMenu", "Mesh"),
				LOCTEXT("CharacterMenu_MeshSubMenuToolTip", "Mesh-related options"),
				FNewMenuDelegate::CreateLambda([](FMenuBuilder& SubMenuBuilder)
				{
					SubMenuBuilder.BeginSection("AnimViewportMesh", LOCTEXT("CharacterMenu_Actions_Mesh", "Mesh"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowRetargetBasePose );
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBound );
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().UseInGameBound );
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowPreviewMesh );
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowMorphTargets );
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowVertexColors );
					}
					SubMenuBuilder.EndSection();

					SubMenuBuilder.BeginSection("AnimViewportMeshInfo", LOCTEXT("CharacterMenu_Actions_MeshInfo", "Mesh Info"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowDisplayInfoBasic);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowDisplayInfoDetailed);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowDisplayInfoSkelControls);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().HideDisplayInfo);
					}
					SubMenuBuilder.EndSection();

					SubMenuBuilder.BeginSection("AnimViewportPreviewOverlayDraw", LOCTEXT("CharacterMenu_Actions_Overlay", "Mesh Overlay Drawing"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowOverlayNone);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneWeight);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowMorphTargetVerts);
					}
					SubMenuBuilder.EndSection();
				})
			);

			InMenuBuilder.AddSubMenu(
				LOCTEXT("CharacterMenu_AnimationSubMenu", "Animation"),
				LOCTEXT("CharacterMenu_AnimationSubMenuToolTip", "Animation-related options"),
				FNewMenuDelegate::CreateLambda([](FMenuBuilder& SubMenuBuilder)
				{
					SubMenuBuilder.BeginSection("AnimViewportRootMotion", LOCTEXT("CharacterMenu_RootMotionLabel", "Root Motion"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ProcessRootMotion);
					}
					SubMenuBuilder.EndSection();

					SubMenuBuilder.BeginSection("AnimViewportAnimation", LOCTEXT("CharacterMenu_Actions_AnimationAsset", "Animation"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowRawAnimation);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowNonRetargetedAnimation);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowAdditiveBaseBones);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowSourceRawAnimation);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBakedAnimation);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().DisablePostProcessBlueprint);
					}
					SubMenuBuilder.EndSection();
				})
			);

			InMenuBuilder.AddSubMenu(
				LOCTEXT("CharacterMenu_BoneDrawSubMenu", "Bones"),
				LOCTEXT("CharacterMenu_BoneDrawSubMenuToolTip", "Bone Drawing Options"),
				FNewMenuDelegate::CreateLambda([](FMenuBuilder& SubMenuBuilder)
				{
					SubMenuBuilder.BeginSection("BonesAndSockets", LOCTEXT("CharacterMenu_BonesAndSocketsLabel", "Show"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowSockets);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneNames);
					}
					SubMenuBuilder.EndSection();

					SubMenuBuilder.BeginSection("AnimViewportPreviewHierarchyBoneDraw", LOCTEXT("CharacterMenu_Actions_BoneDrawing", "Bone Drawing"));
					{
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneDrawAll);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneDrawSelected);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneDrawSelectedAndParents);
						SubMenuBuilder.AddMenuEntry(FAnimViewportShowCommands::Get().ShowBoneDrawNone);
					}
					SubMenuBuilder.EndSection();
				})
				);

#if WITH_APEX_CLOTHING
			UDebugSkelMeshComponent* PreviewComp = Viewport.Pin()->GetPreviewScene()->GetPreviewMeshComponent();

			if(PreviewComp)
			{
				InMenuBuilder.AddSubMenu(
					LOCTEXT("CharacterMenu_ClothingSubMenu", "Clothing"),
					LOCTEXT("CharacterMenu_ClothingSubMenuToolTip", "Options relating to clothing"),
					FNewMenuDelegate::CreateRaw(this, &SAnimViewportToolBar::FillCharacterClothingMenu));
			}
#endif // #if WITH_APEX_CLOTHING
		}

		InMenuBuilder.AddSubMenu(
			LOCTEXT("CharacterMenu_AudioSubMenu", "Audio"),
			LOCTEXT("CharacterMenu_AudioSubMenuToolTip", "Audio options"),
			FNewMenuDelegate::CreateLambda([&Actions](FMenuBuilder& SubMenuBuilder)
			{
				SubMenuBuilder.BeginSection("AnimViewportAudio", LOCTEXT("CharacterMenu_Audio", "Audio"));
				{
					SubMenuBuilder.AddMenuEntry(Actions.MuteAudio);
					SubMenuBuilder.AddMenuEntry(Actions.UseAudioAttenuation);
				}
				SubMenuBuilder.EndSection();
			}));

		InMenuBuilder.AddSubMenu(
			LOCTEXT("CharacterMenu_AdvancedSubMenu", "Advanced"),
			LOCTEXT("CharacterMenu_AdvancedSubMenuToolTip", "Advanced options"),
			FNewMenuDelegate::CreateRaw(this, &SAnimViewportToolBar::FillCharacterAdvancedMenu));

		InMenuBuilder.EndSection();
	}

	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();

	return InMenuBuilder.MakeWidget();
}

void SAnimViewportToolBar::FillCharacterAdvancedMenu(FMenuBuilder& MenuBuilder) const
{
	const FAnimViewportShowCommands& Actions = FAnimViewportShowCommands::Get();

	// Draw UVs
	MenuBuilder.BeginSection("UVVisualization", LOCTEXT("UVVisualization_Label", "UV Visualization"));
	{
		MenuBuilder.AddWidget(Viewport.Pin()->UVChannelCombo.ToSharedRef(), FText());
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Skinning", LOCTEXT("Skinning_Label", "Skinning"));
	{
		MenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().SetCPUSkinning);
	}
	MenuBuilder.EndSection();
	

	MenuBuilder.BeginSection("ShowVertex", LOCTEXT("ShowVertex_Label", "Vertex Normal Visualization"));
	{
		// Vertex debug flags
		MenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().SetShowNormals);
		MenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().SetShowTangents);
		MenuBuilder.AddMenuEntry(FAnimViewportMenuCommands::Get().SetShowBinormals);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimViewportPreviewHierarchyLocalAxes", LOCTEXT("ShowMenu_Actions_HierarchyAxes", "Hierarchy Local Axes") );
	{
		MenuBuilder.AddMenuEntry( Actions.ShowLocalAxesAll );
		MenuBuilder.AddMenuEntry( Actions.ShowLocalAxesSelected );
		MenuBuilder.AddMenuEntry( Actions.ShowLocalAxesNone );
	}
	MenuBuilder.EndSection();
}

void SAnimViewportToolBar::FillCharacterClothingMenu(FMenuBuilder& MenuBuilder)
{
#if WITH_APEX_CLOTHING
	const FAnimViewportShowCommands& Actions = FAnimViewportShowCommands::Get();

	MenuBuilder.BeginSection("ClothPreview", LOCTEXT("ClothPreview_Label", "Simulation"));
	{
		MenuBuilder.AddMenuEntry(Actions.EnableClothSimulation);
		MenuBuilder.AddMenuEntry(Actions.ResetClothSimulation);

		TSharedPtr<SWidget> WindWidget = SNew(SClothWindSettings).AnimEditorViewport(Viewport);
		MenuBuilder.AddWidget(WindWidget.ToSharedRef(), LOCTEXT("ClothPreview_WindStrength", "Wind Strength:"));

		TSharedPtr<SWidget> GravityWidget = SNew(SGravitySettings).AnimEditorViewport(Viewport);
		MenuBuilder.AddWidget(GravityWidget.ToSharedRef(), LOCTEXT("ClothPreview_GravityScale", "Gravity Scale:"));

		MenuBuilder.AddMenuEntry(Actions.EnableCollisionWithAttachedClothChildren);
		MenuBuilder.AddMenuEntry(Actions.PauseClothWithAnim);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("ClothAdditionalVisualization", LOCTEXT("ClothAdditionalVisualization_Label", "Sections Display Mode"));
	{
		MenuBuilder.AddMenuEntry(Actions.ShowAllSections);
		MenuBuilder.AddMenuEntry(Actions.ShowOnlyClothSections);
		MenuBuilder.AddMenuEntry(Actions.HideOnlyClothSections);
	}
	MenuBuilder.EndSection();

	// Call into the clothing editor module to customize the menu (this is mainly for debug visualizations and sim-specific options)
	TSharedPtr<SAnimationEditorViewportTabBody> SharedViewport = Viewport.Pin();
	if(SharedViewport.IsValid())
	{
		TSharedRef<IPersonaPreviewScene> PreviewScene = SharedViewport->GetAnimationViewportClient()->GetPreviewScene();
		if(UDebugSkelMeshComponent* PreviewComponent = PreviewScene->GetPreviewMeshComponent())
		{
			FClothingSystemEditorInterfaceModule& ClothingEditorModule = FModuleManager::LoadModuleChecked<FClothingSystemEditorInterfaceModule>(TEXT("ClothingSystemEditorInterface"));

			if(ISimulationEditorExtender* Extender = ClothingEditorModule.GetSimulationEditorExtender(PreviewComponent->ClothingSimulationFactory->GetFName()))
			{
				Extender->ExtendViewportShowMenu(MenuBuilder, PreviewScene);
			}
		}
	}

#endif // #if WITH_APEX_CLOTHING
}

TSharedRef<SWidget> SAnimViewportToolBar::GenerateShowMenu() const
{
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, CommandList, MenuExtender);

	// Only include helpful show flags.
	static const FShowFlagFilter ShowFlagFilter = FShowFlagFilter(FShowFlagFilter::ExcludeAllFlagsByDefault)
		// General
		.IncludeFlag(FEngineShowFlags::SF_AntiAliasing)
		.IncludeFlag(FEngineShowFlags::SF_Collision)
		.IncludeFlag(FEngineShowFlags::SF_Grid)
		.IncludeFlag(FEngineShowFlags::SF_Particles)
		.IncludeFlag(FEngineShowFlags::SF_Translucency)
		// Post Processing
		.IncludeFlag(FEngineShowFlags::SF_Bloom)
		.IncludeFlag(FEngineShowFlags::SF_DepthOfField)
		.IncludeFlag(FEngineShowFlags::SF_EyeAdaptation)
		.IncludeFlag(FEngineShowFlags::SF_HMDDistortion)
		.IncludeFlag(FEngineShowFlags::SF_MotionBlur)
		.IncludeFlag(FEngineShowFlags::SF_Tonemapper)
		// Lighting Components
		.IncludeGroup(SFG_LightingComponents)
		// Lighting Features
		.IncludeFlag(FEngineShowFlags::SF_AmbientCubemap)
		.IncludeFlag(FEngineShowFlags::SF_DistanceFieldAO)
		.IncludeFlag(FEngineShowFlags::SF_IndirectLightingCache)
		.IncludeFlag(FEngineShowFlags::SF_LightFunctions)
		.IncludeFlag(FEngineShowFlags::SF_LightShafts)
		.IncludeFlag(FEngineShowFlags::SF_ReflectionEnvironment)
		.IncludeFlag(FEngineShowFlags::SF_ScreenSpaceAO)
		.IncludeFlag(FEngineShowFlags::SF_ContactShadows)
		.IncludeFlag(FEngineShowFlags::SF_ScreenSpaceReflections)
		.IncludeFlag(FEngineShowFlags::SF_SubsurfaceScattering)
		.IncludeFlag(FEngineShowFlags::SF_TexturedLightProfiles)
		// Developer
		.IncludeFlag(FEngineShowFlags::SF_Refraction)
		// Advanced
		.IncludeFlag(FEngineShowFlags::SF_DeferredLighting)
		.IncludeFlag(FEngineShowFlags::SF_Selection)
		.IncludeFlag(FEngineShowFlags::SF_SeparateTranslucency)
		.IncludeFlag(FEngineShowFlags::SF_TemporalAA)
		.IncludeFlag(FEngineShowFlags::SF_Tessellation)
		;

	FShowFlagMenuCommands::Get().BuildShowFlagsMenu(InMenuBuilder, ShowFlagFilter);

	return InMenuBuilder.MakeWidget();
}

FText SAnimViewportToolBar::GetLODMenuLabel() const
{
	FText Label = LOCTEXT("LODMenu_AutoLabel", "LOD Auto");
	if (Viewport.IsValid())
	{
		int32 LODSelectionType = Viewport.Pin()->GetLODSelection();

		if (LODSelectionType > 0)
		{
			FString TitleLabel = FString::Printf(TEXT("LOD %d"), LODSelectionType - 1);
			Label = FText::FromString(TitleLabel);
		}
	}
	return Label;
}

TSharedRef<SWidget> SAnimViewportToolBar::GenerateLODMenu() const
{
	const FAnimViewportLODCommands& Actions = FAnimViewportLODCommands::Get();

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList(), MenuExtender);

	InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());

	{
		// LOD Models
		InMenuBuilder.BeginSection("AnimViewportPreviewLODs", LOCTEXT("ShowLOD_PreviewLabel", "Preview LODs") );
		{
			InMenuBuilder.AddMenuEntry( Actions.LODAuto );
			InMenuBuilder.AddMenuEntry( Actions.LOD0 );

			int32 LODCount = Viewport.Pin()->GetLODModelCount();
			for (int32 LODId = 1; LODId < LODCount; ++LODId)
			{
				FString TitleLabel = FString::Printf(TEXT("LOD %d"), LODId);

				FUIAction Action(FExecuteAction::CreateSP(Viewport.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::OnSetLODModel, LODId + 1),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(Viewport.Pin().ToSharedRef(), &SAnimationEditorViewportTabBody::IsLODModelSelected, LODId + 1));

				InMenuBuilder.AddMenuEntry(FText::FromString(TitleLabel), FText::GetEmpty(), FSlateIcon(), Action, NAME_None, EUserInterfaceActionType::RadioButton);
			}
		}
		InMenuBuilder.EndSection();
	}

	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();

	return InMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAnimViewportToolBar::GenerateViewportTypeMenu() const
{

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, CommandList, MenuExtender);
	InMenuBuilder.SetStyle(&FEditorStyle::Get(), "Menu");
	InMenuBuilder.PushCommandList(CommandList.ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());

	// Camera types
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Perspective);

	InMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", LOCTEXT("CameraTypeHeader_Ortho", "Orthographic"));
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Top);
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Bottom);
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Left);
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Right);
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Front);
	InMenuBuilder.AddMenuEntry(FEditorViewportCommands::Get().Back);
	InMenuBuilder.EndSection();

	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();

	return InMenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SAnimViewportToolBar::GeneratePlaybackMenu() const
{
	const FAnimViewportPlaybackCommands& Actions = FAnimViewportPlaybackCommands::Get();

	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder InMenuBuilder(bInShouldCloseWindowAfterMenuSelection, Viewport.Pin()->GetCommandList(), MenuExtender);

	InMenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	InMenuBuilder.PushExtender(MenuExtender.ToSharedRef());
	{
		// View modes
		{
			InMenuBuilder.BeginSection("AnimViewportPlaybackSpeed", LOCTEXT("PlaybackMenu_SpeedLabel", "Playback Speed") );
			{
				for(int32 PlaybackSpeedIndex = 0; PlaybackSpeedIndex < EAnimationPlaybackSpeeds::NumPlaybackSpeeds; ++PlaybackSpeedIndex)
				{
					InMenuBuilder.AddMenuEntry( Actions.PlaybackSpeedCommands[PlaybackSpeedIndex] );
				}
			}
			InMenuBuilder.EndSection();
		}
	}
	InMenuBuilder.PopCommandList();
	InMenuBuilder.PopExtender();

	return InMenuBuilder.MakeWidget();
}

void SAnimViewportToolBar::GenerateTurnTableMenu(FMenuBuilder& MenuBuilder) const
{
	const FAnimViewportPlaybackCommands& Actions = FAnimViewportPlaybackCommands::Get();

	const bool bInShouldCloseWindowAfterMenuSelection = true;

	MenuBuilder.PushCommandList(Viewport.Pin()->GetCommandList().ToSharedRef());
	MenuBuilder.BeginSection("AnimViewportTurnTableMode", LOCTEXT("TurnTableMenu_ModeLabel", "Turn Table Mode"));
	{
		MenuBuilder.AddMenuEntry(Actions.PersonaTurnTablePlay);
		MenuBuilder.AddMenuEntry(Actions.PersonaTurnTablePause);
		MenuBuilder.AddMenuEntry(Actions.PersonaTurnTableStop);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimViewportTurnTableSpeed", LOCTEXT("TurnTableMenu_SpeedLabel", "Turn Table Speed"));
	{
		for (int i = 0; i < EAnimationPlaybackSpeeds::NumPlaybackSpeeds; ++i)
		{
			MenuBuilder.AddMenuEntry(Actions.TurnTableSpeeds[i]);
		}
	}
	MenuBuilder.EndSection();
	MenuBuilder.PopCommandList();
}

FSlateColor SAnimViewportToolBar::GetFontColor() const
{
	const UAssetViewerSettings* Settings = UAssetViewerSettings::Get();
	const UEditorPerProjectUserSettings* PerProjectUserSettings = GetDefault<UEditorPerProjectUserSettings>();
	const int32 ProfileIndex = Settings->Profiles.IsValidIndex(PerProjectUserSettings->AssetViewerProfileIndex) ? PerProjectUserSettings->AssetViewerProfileIndex : 0;

	ensureMsgf(Settings->Profiles.IsValidIndex(PerProjectUserSettings->AssetViewerProfileIndex), TEXT("Invalid default settings pointer or current profile index"));

	FLinearColor FontColor;
	if (Settings->Profiles[ProfileIndex].bShowEnvironment)
	{
		FontColor = FLinearColor::White;
	}
	else
	{
		FLinearColor Color = Settings->Profiles[ProfileIndex].EnvironmentColor * Settings->Profiles[ProfileIndex].EnvironmentIntensity;

		// see if it's dark, if V is less than 0.2
		if (Color.B < 0.3f )
		{
			FontColor = FLinearColor::White;
		}
		else
		{
			FontColor = FLinearColor::Black;
		}
	}

	return FontColor;
}

FText SAnimViewportToolBar::GetPlaybackMenuLabel() const
{
	FText Label = LOCTEXT("PlaybackError", "Error");
	if (Viewport.IsValid())
	{
		for(int i = 0; i < EAnimationPlaybackSpeeds::NumPlaybackSpeeds; ++i)
		{
			if (Viewport.Pin()->IsPlaybackSpeedSelected(i))
			{
				int32 NumFractionalDigits = (i == EAnimationPlaybackSpeeds::Quarter) ? 2 : 1;

				const FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
					.SetMinimumFractionalDigits(NumFractionalDigits)
					.SetMaximumFractionalDigits(NumFractionalDigits);

				Label = FText::Format(LOCTEXT("AnimViewportPlaybackMenuLabel", "x{0}"), FText::AsNumber(EAnimationPlaybackSpeeds::Values[i], &FormatOptions));
			}
		}
	}
	return Label;
}

FText SAnimViewportToolBar::GetCameraMenuLabel() const
{
	FText Label = LOCTEXT("Viewport_Default", "Camera");
	TSharedPtr< SAnimationEditorViewportTabBody > PinnedViewport(Viewport.Pin());
	if( PinnedViewport.IsValid() )
	{
		switch( PinnedViewport->GetLevelViewportClient().ViewportType )
		{
		case LVT_Perspective:
			Label = LOCTEXT("CameraMenuTitle_Perspective", "Perspective");
			break;

		case LVT_OrthoXY:
			Label = LOCTEXT("CameraMenuTitle_Top", "Top");
			break;

		case LVT_OrthoYZ:
			Label = LOCTEXT("CameraMenuTitle_Left", "Left");
			break;

		case LVT_OrthoXZ:
			Label = LOCTEXT("CameraMenuTitle_Front", "Front");
			break;

		case LVT_OrthoNegativeXY:
			Label = LOCTEXT("CameraMenuTitle_Bottom", "Bottom");
			break;

		case LVT_OrthoNegativeYZ:
			Label = LOCTEXT("CameraMenuTitle_Right", "Right");
			break;

		case LVT_OrthoNegativeXZ:
			Label = LOCTEXT("CameraMenuTitle_Back", "Back");
			break;
		case LVT_OrthoFreelook:
			break;
		}
	}

	return Label;
}

const FSlateBrush* SAnimViewportToolBar::GetCameraMenuLabelIcon() const
{
	FName Icon = NAME_None;
	TSharedPtr< SAnimationEditorViewportTabBody > PinnedViewport(Viewport.Pin());
	if (PinnedViewport.IsValid())
	{
		static FName PerspectiveIcon("EditorViewport.Perspective");
		static FName TopIcon("EditorViewport.Top");
		static FName LeftIcon("EditorViewport.Left");
		static FName FrontIcon("EditorViewport.Front");
		static FName BottomIcon("EditorViewport.Bottom");
		static FName RightIcon("EditorViewport.Right");
		static FName BackIcon("EditorViewport.Back");

		switch (PinnedViewport->GetLevelViewportClient().ViewportType)
		{
		case LVT_Perspective:
			Icon = PerspectiveIcon;
			break;

		case LVT_OrthoXY:
			Icon = TopIcon;
			break;

		case LVT_OrthoYZ:
			Icon = LeftIcon;
			break;

		case LVT_OrthoXZ:
			Icon = FrontIcon;
			break;

		case LVT_OrthoNegativeXY:
			Icon = BottomIcon;
			break;

		case LVT_OrthoNegativeYZ:
			Icon = RightIcon;
			break;

		case LVT_OrthoNegativeXZ:
			Icon = BackIcon;
			break;
		case LVT_OrthoFreelook:
			break;
		}
	}

	return FEditorStyle::GetBrush(Icon);
}

TOptional<float> SAnimViewportToolBar::OnGetFOVValue() const
{
	if(Viewport.IsValid())
	{
		return Viewport.Pin()->GetLevelViewportClient().ViewFOV;
	}
	return 0.0f;
}

void SAnimViewportToolBar::OnFOVValueChanged( float NewValue )
{
	FEditorViewportClient& ViewportClient = Viewport.Pin()->GetLevelViewportClient();

	ViewportClient.FOVAngle = NewValue;

	FAnimationViewportClient& AnimViewportClient = (FAnimationViewportClient&)(ViewportClient);
	AnimViewportClient.ConfigOption->SetViewFOV(AnimViewportClient.GetAssetEditorToolkit()->GetEditorName(), NewValue, AnimViewportClient.GetViewportIndex());

	ViewportClient.ViewFOV = NewValue;
	ViewportClient.Invalidate();

	PinnedCommands->AddCustomWidget(TEXT("FOVWidget"));
}

void SAnimViewportToolBar::OnFOVValueCommitted( float NewValue, ETextCommit::Type CommitInfo )
{
	//OnFOVValueChanged will be called... nothing needed here.
}

TOptional<float> SAnimViewportToolBar::OnGetFloorOffset() const
{
	if(Viewport.IsValid())
	{
		FAnimationViewportClient& AnimViewportClient = (FAnimationViewportClient&)Viewport.Pin()->GetLevelViewportClient();
		return AnimViewportClient.GetFloorOffset();
	}

	return 0.0f;
}

void SAnimViewportToolBar::OnFloorOffsetChanged( float NewValue )
{
	FAnimationViewportClient& AnimViewportClient = (FAnimationViewportClient&)Viewport.Pin()->GetLevelViewportClient();

	AnimViewportClient.SetFloorOffset( NewValue );

	PinnedCommands->AddCustomWidget(TEXT("FloorOffsetWidget"));
}

TSharedRef<FExtender> SAnimViewportToolBar::GetViewMenuExtender(TSharedPtr<class SEditorViewport> InRealViewport)
{
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension(
		TEXT("ViewMode"),
		EExtensionHook::After,
		InRealViewport->GetCommandList(),
		FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& InMenuBuilder)
		{
			InMenuBuilder.AddSubMenu(LOCTEXT("VisualizeBufferViewModeDisplayName", "Buffer Visualization"),
				LOCTEXT("BufferVisualizationMenu_ToolTip", "Select a mode for buffer visualization"),
				FNewMenuDelegate::CreateStatic(&FBufferVisualizationMenuCommands::BuildVisualisationSubMenu),
				false,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "EditorViewport.VisualizeBufferMode"));
		}));

	return Extender;
}

#undef LOCTEXT_NAMESPACE
