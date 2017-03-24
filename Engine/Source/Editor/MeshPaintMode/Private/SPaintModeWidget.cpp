// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SPaintModeWidget.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SScrollBox.h"
#include "SBoxPanel.h"
#include "SWrapBox.h"
#include "SBox.h"
#include "SButton.h"
#include "SImage.h"
#include "MultiBoxBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PaintModeSettingsCustomization.h"
#include "PaintModePainter.h"
#include "PaintModeSettings.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "PaintModePainter"

void SPaintModeWidget::Construct(const FArguments& InArgs, FPaintModePainter* InPainter)
{
	MeshPainter = InPainter;
	PaintModeSettings = Cast<UPaintModeSettings>(MeshPainter->GetPainterSettings());
	SettingsObjects.Add(MeshPainter->GetBrushSettings());
	SettingsObjects.Add(PaintModeSettings);
	CreateDetailsView();
	
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);
	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Content()
			[
				SNew(SVerticalBox)

				/** Toolbar containing buttons to switch between different paint modes */
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.HAlign(HAlign_Center)
						[
							CreateToolBarWidget()->AsShared()
						]
					]
				]
				
				/** (Instance) Vertex paint action buttons widget */
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					CreateVertexPaintWidget()->AsShared()
				]
				
				/** Texture paint action buttons widget */
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					CreateTexturePaintWidget()->AsShared()
				]

				/** DetailsView containing brush and paint settings */
				+ SVerticalBox::Slot()
				.AutoHeight()				
				[
					SettingsDetailsView->AsShared()
				]
			]
		]
	];
}

void SPaintModeWidget::CreateDetailsView()
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs(
		/*bUpdateFromSelection=*/ false,
		/*bLockable=*/ false,
		/*bAllowSearch=*/ false,
		FDetailsViewArgs::HideNameArea,
		/*bHideSelectionTip=*/ true,
		/*InNotifyHook=*/ nullptr,
		/*InSearchInitialKeyFocus=*/ false,
		/*InViewIdentifier=*/ NAME_None);
	DetailsViewArgs.DefaultsOnlyVisibility = FDetailsViewArgs::EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;

	SettingsDetailsView = EditModule.CreateDetailView(DetailsViewArgs);
	SettingsDetailsView->SetRootObjectCustomizationInstance(MakeShareable(new FPaintModeSettingsRootObjectCustomization));
	SettingsDetailsView->SetObjects(SettingsObjects);
}

TSharedPtr<SWidget> SPaintModeWidget::CreateVertexPaintWidget()
{
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);

	TSharedPtr<SWidget> VertexColorWidget;
	TSharedPtr<SWrapBox> VertexColorActionWrapbox;
	TSharedPtr<SWrapBox> InstanceColorActionWrapbox;

	SAssignNew(VertexColorWidget, SVerticalBox)
	.Visibility(this, &SPaintModeWidget::IsVertexPaintModeVisible)
		
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(VertexColorActionWrapbox, SWrapBox)
			.UseAllottedWidth(true)				
		]
	]		 

	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(InstanceColorActionWrapbox, SWrapBox)
			.UseAllottedWidth(true)
		]
	];

	/** Populates the vertex color action widget with buttons for each individual EVertexColorAction enum entry */
	const UEnum* VertexColorActionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EVertexColorAction"));	
	auto CanApplyVertexAction = [this](int32 Action) -> bool { return MeshPainter->CanApplyVertexColorAction((EVertexColorAction)Action); };
	auto ApplyVertexAction = [this](int32 Action) -> FReply { MeshPainter->ApplyVertexColorAction((EVertexColorAction)Action); return FReply::Handled(); };
	CreateActionEnumButtons(VertexColorActionWrapbox, VertexColorActionEnum, CanApplyVertexAction, ApplyVertexAction);

	/** Populates the instance vertex color action widget with buttons for each individual EInstanceColorAction enum entry */
	const UEnum* InstanceColorActionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EInstanceColorAction"));
	auto CanApplyInstanceAction = [this](int32 Action) -> bool { return MeshPainter->CanApplyInstanceColorAction((EInstanceColorAction)Action); };
	auto ApplyInstanceAction = [this](int32 Action) -> FReply { MeshPainter->ApplyInstanceColorAction((EInstanceColorAction)Action); return FReply::Handled(); };
	CreateActionEnumButtons(InstanceColorActionWrapbox, InstanceColorActionEnum, CanApplyInstanceAction, ApplyInstanceAction);

	return VertexColorWidget->AsShared();
}

void SPaintModeWidget::CreateActionEnumButtons(TSharedPtr<SWrapBox> WrapBox, const UEnum* Enum, TFunction<bool(int32)> EnabledFunc, TFunction<FReply (int32)> ClickFunc)
{
	FMargin StandardPadding(0.0f, 0.0f, 4.0f, 0.0f);

	const int32 NumEntries = Enum->GetMaxEnumValue();

	/** Add a button for each enum entry */
	for (int32 Index = 0; Index < NumEntries; ++Index)
	{
		/** Retrieve label, tooltip and icon path from enum entry meta-data */
		const FText ButtonLabel = Enum->GetDisplayNameTextByIndex(Index);
		const FText ButtonToolTip = Enum->GetToolTipTextByIndex(Index);
		const FString& IconPathMetaData = Enum->GetMetaData(TEXT("Icon"), Index);

		/** Create horizontal box with button using the function-ptrs */
		TSharedPtr<SHorizontalBox> ButtonContentBox;
		WrapBox->AddSlot()
		.Padding(2.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(90.f)
			.HeightOverride(25.f)
			[
				SNew(SButton)			
				.ToolTipText(ButtonToolTip)			
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.IsEnabled_Lambda([EnabledFunc, Index]() -> bool { return EnabledFunc(Index); })
				.OnClicked_Lambda([ClickFunc, Index]() -> FReply { return ClickFunc(Index); })
				[
					SAssignNew(ButtonContentBox, SHorizontalBox)					
				]
			]
		];

		/** If there is an icon path retrieve it and add it to the button content, otherwise just add the label */
		if (!IconPathMetaData.IsEmpty())
		{
			const FSlateBrush* Brush = FCoreStyle::Get().GetBrush(*IconPathMetaData);
			if (Brush == FCoreStyle::Get().GetDefaultBrush())
			{
				Brush = FEditorStyle::GetBrush(*IconPathMetaData);
			}

			ButtonContentBox->AddSlot()
			.AutoWidth()
			.Padding(StandardPadding)
			[
				SNew(SImage)
				.Image(Brush)
			];
		}

		ButtonContentBox->AddSlot()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(ButtonLabel)
			.Justification(ETextJustify::Center)
		];
	}
}
 
TSharedPtr<SWidget> SPaintModeWidget::CreateTexturePaintWidget()
{
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);

	TSharedPtr<SWidget> TexturePaintWidget;
	TSharedPtr<SWrapBox> TexturePaintActionWrapbox;
	TSharedPtr<SWrapBox> InstanceColorActionWrapbox;

	SAssignNew(TexturePaintWidget, SVerticalBox)
	.Visibility(this, &SPaintModeWidget::IsTexturePaintModeVisible)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(StandardPadding)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1)
		.Padding(2.0f, 0.0f)
		[
			SAssignNew(TexturePaintActionWrapbox, SWrapBox)
			.UseAllottedWidth(true)
		]
	];
	 
	/** Populates the instance vertex color action widget with buttons for each individual ETexturePaintAction enum entry */
	const UEnum* TexturePaintActionEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETexturePaintAction"));
	CreateActionEnumButtons(TexturePaintActionWrapbox, TexturePaintActionEnum, [this](int32 Action) -> bool { return MeshPainter->CanApplyTexturePaintAction((ETexturePaintAction)Action); }, [this](int32 Action) -> FReply { MeshPainter->ApplyTexturePaintAction((ETexturePaintAction)Action); return FReply::Handled(); });

	return TexturePaintWidget->AsShared();
}

TSharedPtr<SWidget> SPaintModeWidget::CreateToolBarWidget()
{
	FToolBarBuilder ModeSwitchButtons(MakeShareable(new FUICommandList()), FMultiBoxCustomization::None);
	{
		FSlateIcon ColorPaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.ColorPaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Vertices;
			PaintModeSettings->VertexPaintSettings.MeshPaintMode = EMeshPaintMode::PaintColors;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Vertices && PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintColors; })), NAME_None, LOCTEXT("Mode.VertexColorPainting", "Vertex Color Painting"), LOCTEXT("Mode.VertexColor.Tooltip", "Vertex Color Painting mode allows painting of Vertex Colors"), ColorPaintIcon, EUserInterfaceActionType::ToggleButton);

		FSlateIcon WeightPaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.WeightPaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Vertices;
			PaintModeSettings->VertexPaintSettings.MeshPaintMode = EMeshPaintMode::PaintWeights;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Vertices && PaintModeSettings->VertexPaintSettings.MeshPaintMode == EMeshPaintMode::PaintWeights; })), NAME_None, LOCTEXT("Mode.VertexWeightPainting", "Vertex Weight Painting"), LOCTEXT("Mode.VertexWeight.Tooltip", "Vertex Weight Painting mode allows painting of Vertex Weights"), WeightPaintIcon, EUserInterfaceActionType::ToggleButton);

		FSlateIcon TexturePaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.TexturePaint");
		ModeSwitchButtons.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([=]()
		{
			PaintModeSettings->PaintMode = EPaintMode::Textures;
			SettingsDetailsView->SetObjects(SettingsObjects, true);
		}), FCanExecuteAction(), FIsActionChecked::CreateLambda([=]() -> bool { return PaintModeSettings->PaintMode == EPaintMode::Textures; })), NAME_None, LOCTEXT("Mode.TexturePainting", "Texture Painting"), LOCTEXT("Mode.Texture.Tooltip", "Texture Weight Painting mode allows painting on Textures"), TexturePaintIcon, EUserInterfaceActionType::ToggleButton);
	}

	return ModeSwitchButtons.MakeWidget();
}

EVisibility SPaintModeWidget::IsVertexPaintModeVisible() const
{
	UPaintModeSettings* MeshPaintSettings = (UPaintModeSettings*)MeshPainter->GetPainterSettings();
	return (MeshPaintSettings->PaintMode == EPaintMode::Vertices) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SPaintModeWidget::IsTexturePaintModeVisible() const
{
	UPaintModeSettings* MeshPaintSettings = (UPaintModeSettings*)MeshPainter->GetPainterSettings();
	return (MeshPaintSettings->PaintMode == EPaintMode::Textures) ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE // "PaintModePainter"