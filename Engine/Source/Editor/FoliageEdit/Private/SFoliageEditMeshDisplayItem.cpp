// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "SFoliageEdit.h"
#include "FoliageEditActions.h"
#include "FoliageEdMode.h"
#include "Runtime/Engine/Classes/Foliage/InstancedFoliageSettings.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#include "SFoliageEditMeshDisplayItem.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFoliageEditMeshDisplayItem::Construct( const FArguments& InArgs )
{
	BorderImage = FEditorStyle::GetBrush("FoliageEditMode.BubbleBorder");
	BorderBackgroundColor = TAttribute<FSlateColor>(this, &SFoliageEditMeshDisplayItem::GetBorderColor);
 
	FoliageEditPtr = InArgs._FoliageEditPtr;
	FoliageSettingsPtr = InArgs._FoliageSettingsPtr;
	AssociatedStaticMesh = InArgs._AssociatedStaticMesh;
	Thumbnail = InArgs._AssetThumbnail;
	FoliageMeshUIInfo = InArgs._FoliageMeshUIInfo;

	ThumbnailWidget = InArgs._AssetThumbnail->MakeThumbnailWidget();

	// Create the command list and bind the commands.
	UICommandList = MakeShareable( new FUICommandList );
	BindCommands();

	CurrentViewSettings = ECurrentViewSettings::ShowPaintSettings;

	// Create the details panels for the clustering tab.
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	const FDetailsViewArgs DetailsViewArgs( false, false, true, false, true, this );
	ClusterSettingsDetails = PropertyEditorModule.CreateDetailView( DetailsViewArgs );

	DetailsObjectList.Add(FoliageSettingsPtr);
	ClusterSettingsDetails->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SFoliageEditMeshDisplayItem::IsPropertyVisible));
	ClusterSettingsDetails->SetObjects(DetailsObjectList);
	
	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(2.0f, 2.0f, 2.0f, 2.0f);
	FMargin StandardSidePadding(2.0f, 0.0f, 2.0f, 0.0f);

	// Create the toolbar for the tab options.
	FToolBarBuilder Toolbar(UICommandList, FMultiBoxCustomization::None );
	{
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetNoSettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaintSettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetClusterSettings);
	}

	SAssignNew( ThumbnailBox, SBox )
		.WidthOverride( 80 ) .HeightOverride( 80 )
		[
			SAssignNew(ThumbnailWidgetBorder, SBorder)
			[
				ThumbnailWidget.ToSharedRef()
			]
		];

	const float MAIN_TITLE = 1.0f;
	const float SPINBOX_PREFIX = 0.3f;
	const float SPINBOX_SINGLE = 2.2f;

	TSharedRef<SHorizontalBox> DensityBox = SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
			.FillWidth(1.0f)
		[
			SNew(SVerticalBox)

			// Density
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew( SHorizontalBox )
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNotReapplySettingsVisible)
				
				+SHorizontalBox::Slot()
				.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
				[
					SNew( SHorizontalBox )

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Density", "Density / 1Kuu"))
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DensitySuperscript", "2"))
						.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf" ), 8))
					]
				]

				+SHorizontalBox::Slot()
				.FillWidth(SPINBOX_SINGLE)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(65536.0f)
					.MaxSliderValue(1000.0f)
					.Value(this, &SFoliageEditMeshDisplayItem::GetDensity)
					.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnDensityChanged)
				]
			]

			// Density Adjustment
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew( SHorizontalBox )
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnDensityReapply)
					.IsChecked(this, &SFoliageEditMeshDisplayItem::IsDensityReapplyChecked)
				]

				+SHorizontalBox::Slot()
				.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DensityAdjust", "Density Adjust"))
				]

				+SHorizontalBox::Slot()
				.FillWidth(SPINBOX_SINGLE)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(4.0f)
					.MaxSliderValue(2.0f)
					.Value(this, &SFoliageEditMeshDisplayItem::GetDensity)
					.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnDensityChanged)
				]

			]

			// Radius
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew( SHorizontalBox )

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
					.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnRadiusReapply)
					.IsChecked(this, &SFoliageEditMeshDisplayItem::IsRadiusReapplyChecked)
				]

				+SHorizontalBox::Slot()
				.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Radius", "Radius"))
				]

				+SHorizontalBox::Slot()
				.FillWidth(SPINBOX_SINGLE)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(65536.0f)
					.MaxSliderValue(200.0f)
					.Value(this, &SFoliageEditMeshDisplayItem::GetRadius)
					.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnRadiusChanged)
				]
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(StandardPadding)
		[
			ThumbnailBox.ToSharedRef()	
		];

	TSharedRef<SHorizontalBox> AlignToNormalBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnAlignToNormalReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsAlignToNormalReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnAlignToNormal)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsAlignToNormalChecked)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AlignToNormal", "Align to Normal"))
			]
		];

	TSharedRef<SHorizontalBox> MaxAngleBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsAlignToNormalVisible)

		// Dummy Checkbox
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible_Dummy)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MaxAngle", "Max Angle +/-"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-180.f)
				.MaxValue(180.0f)
				.MinSliderValue(0.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetMaxAngle)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnMaxAngleChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> RandomYawBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnRandomYawReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsRandomYawReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnRandomYaw)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsRandomYawChecked)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RandomYaw", "Random Yaw"))
			]
		];

	TSharedRef<SHorizontalBox> UniformScaleBox = SNew( SHorizontalBox )

		// Dummy Checkbox
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible_Dummy)
		]

		// Uniform Scale Checkbox
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnUniformScale)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsUniformScaleChecked)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UniformScale", "Uniform Scale"))
			]
		]

		+SHorizontalBox::Slot()
			.FillWidth(1.0f)
		[
			SNew(SSpacer)
		]
		// "Lock" Non-uniform scaling checkbox.
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Lock", "Lock"))
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible)
		];

	TSharedRef<SHorizontalBox> ScaleUniformBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsUniformScalingVisible)

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleUniformReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleUniformReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScaleUniform", "Scale"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.5)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleUniformMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleUniformMinChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleUniformMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleUniformMaxChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> ScaleXBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible)

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleXReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleXReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScaleX", "Scale X"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleXMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleXMinChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleXMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleXMaxChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleXLocked)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleXLockedChecked)
			]
		];

	TSharedRef<SHorizontalBox> ScaleYBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible)

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleYReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleYReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScaleY", "Scale Y"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleYMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleYMinChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleYMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleYMaxChanged)
			]
				
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleYLocked)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleYLockedChecked)
			]
		];

	TSharedRef<SHorizontalBox> ScaleZBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible)

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleZReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleZReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScaleZ", "Scale Z"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )
					
			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleZMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleZMinChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.0f)
				.MaxValue(100.0f)
				.MaxSliderValue(10.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetScaleZMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnScaleZMaxChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnScaleZLocked)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsScaleZLockedChecked)
			]
		];

	TSharedRef<SHorizontalBox> ZOffsetBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnZOffsetReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsZOffsetReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ZOffset", "Z Offset"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-65536.0f)
				.MaxValue(65536.0f)
				.MinSliderValue(-100.0f)
				.MaxSliderValue(100.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetZOffsetMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnZOffsetMin)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-65536.0f)
				.MaxValue(65536.0f)
				.MinSliderValue(-100.0f)
				.MaxSliderValue(100.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetZOffsetMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnZOffsetMax)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> RandomPitchBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnRandomPitchReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsRandomPitchReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RandomPitchAngle", "Random Pitch +/-"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-180.0f)
				.MinSliderValue(0.0f)
				.MaxValue(180.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetRandomPitch)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnRandomPitchChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> GroundSlopeBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnGroundSlopeReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsGroundSlopeReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GroundSlope", "Ground Slope"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-180.0f)
				.MinSliderValue(0.0f)
				.MaxValue(180.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetGroundSlope)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnGroundSlopeChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> HeightBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnHeightReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsHeightReapplyChecked)
		]
		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Height", "Height"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Min", "Min"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)
				.MinValue(-262144.0f)
				.MaxValue(262144.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetHeightMin)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnHeightMinChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Max", "Max"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.5f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(-262144.0f)
				.MaxValue(262144.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetHeightMax)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnHeightMaxChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> LandscapeLayerBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnLandscapeLayerReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsLandscapeLayerReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LandscapeLayer", "Landscape Layer"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &SFoliageEditMeshDisplayItem::GetLandscapeLayer)
				.OnTextChanged(this, &SFoliageEditMeshDisplayItem::OnLandscapeLayerChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> CollisionWithWorldBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnCollisionWithWorldReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsCollisionWithWorldReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnCollisionWithWorld)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsCollisionWithWorldChecked)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CollisionWithWorld", "Collision with World"))
			]
		];

		TSharedRef<SHorizontalBox> CollisionScaleX = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsCollisionWithWorldVisible)

		// Dummy Checkbox
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible_Dummy)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CollisionScale", "Collision Scale"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_PREFIX)
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot().FillWidth(1.0f) // empty slot to eat the space for the alignment
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("X", "X"))
			]
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth(0.333333f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.01f)
				.MaxValue(5.0f)
				.MaxSliderValue(1.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetCollisionScaleX)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnCollisionScaleXChanged)
			]

			// Y
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Y", "Y"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.333333f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.01f)
				.MaxValue(5.0f)
				.MaxSliderValue(1.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetCollisionScaleY)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnCollisionScaleYChanged)
			]

			// Z
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(StandardSidePadding)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Z", "Z"))
			]

			+SHorizontalBox::Slot()
			.FillWidth(0.333333f)
			[
				SNew(SSpinBox<float>)	
				.MinValue(0.01f)
				.MaxValue(5.0f)
				.MaxSliderValue(1.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetCollisionScaleZ)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnCollisionScaleZChanged)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Visibility(this, &SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy)
			]
		];

	TSharedRef<SHorizontalBox> VertexColorMaskBox = SNew( SHorizontalBox )

		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible)
			.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMaskReapply)
			.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskReapplyChecked)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("VertexColorMask", "Vertex Color Mask"))
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMask, FOLIAGEVERTEXCOLORMASK_Disabled)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked, FOLIAGEVERTEXCOLORMASK_Disabled)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VertexColorDisabled", "Disabled"))
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMask, FOLIAGEVERTEXCOLORMASK_Red)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked, FOLIAGEVERTEXCOLORMASK_Red)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VertexColorR", "R"))
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMask, FOLIAGEVERTEXCOLORMASK_Green)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked, FOLIAGEVERTEXCOLORMASK_Green)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VertexColorG", "G"))
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[

				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMask, FOLIAGEVERTEXCOLORMASK_Blue)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked, FOLIAGEVERTEXCOLORMASK_Blue)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VertexColorB", "B"))
				]
			]


			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[

				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMask, FOLIAGEVERTEXCOLORMASK_Alpha)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked, FOLIAGEVERTEXCOLORMASK_Alpha)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VertexColorA", "A"))
				]
			]
		];

	TSharedRef<SHorizontalBox> VertexColorMaskThresholdBox = SNew( SHorizontalBox )
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskThresholdVisible)

		// Dummy Checkbox
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsReapplySettingsVisible_Dummy)
		]

		+SHorizontalBox::Slot()
		.FillWidth(MAIN_TITLE + SPINBOX_PREFIX)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("VertexColorMaskThreshold", "Mask Threshold"))
			.Visibility(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskThresholdVisible)
		]

		+SHorizontalBox::Slot()
		.FillWidth(SPINBOX_SINGLE)
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(1.0f)
				.Value(this, &SFoliageEditMeshDisplayItem::GetVertexColorMaskThreshold)
				.OnValueChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMaskThresholdChanged)
			]
				
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(StandardSidePadding)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)									
				.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnVertexColorMaskInvert)
				.IsChecked(this, &SFoliageEditMeshDisplayItem::IsVertexColorMaskInvertChecked)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("VertexColorMaskThresholdInvert", "Invert"))
			]
		];

	TSharedRef<SVerticalBox> PaintSettings = SNew(SVerticalBox)
		.Visibility(this, &SFoliageEditMeshDisplayItem::IsPaintSettingsVisible)

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			DensityBox
		]

		// Align to Normal Checkbox
		+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				AlignToNormalBox
			]

		// Max Angle
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			MaxAngleBox
		]

		// Random Yaw Checkbox
		+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				RandomYawBox
			]

		+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				UniformScaleBox
			]

		// Uniform Scale
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			ScaleUniformBox
		]

		// Scale X
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			ScaleXBox
		]

		// Scale Y
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			ScaleYBox
		]

		// Scale Z
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			ScaleZBox
		]

		// Z Offset
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			ZOffsetBox
		]

		// Random Pitch +/-
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			RandomPitchBox
		]

		// Ground Slope
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			GroundSlopeBox
		]

		// Height
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			HeightBox
		]

		// Landscape Layer
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			LandscapeLayerBox
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			CollisionWithWorldBox
		]

		// Collision ScaleX
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			CollisionScaleX
		]

		// Vertex Color Mask
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			VertexColorMaskBox
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			VertexColorMaskThresholdBox
		];

	this->ChildSlot
	.Padding( 6.0f )
	.VAlign(VAlign_Top)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("FoliageEditMode.ItemBackground"))
		.Padding(0.0f)
		[
			SNew(SHorizontalBox)
	
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.VAlign(VAlign_Center)
				.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
				.OnMouseButtonDown(this, &SFoliageEditMeshDisplayItem::OnMouseDownSelection)
				.BorderImage(FEditorStyle::GetBrush("FoliageEditMode.SelectionBackground"))
				.BorderBackgroundColor(TAttribute<FSlateColor>(this, &SFoliageEditMeshDisplayItem::GetBorderColor))
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolTip.Background"))
					.Padding(0.0f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SFoliageEditMeshDisplayItem::IsSelected)
						.OnCheckStateChanged(this, &SFoliageEditMeshDisplayItem::OnSelectionChanged)
						.Padding(0.0f)
					]
				]
			]

			+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(2.0f)
			[
				SNew( SVerticalBox )

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					SNew( SHorizontalBox )
					.Visibility(this, &SFoliageEditMeshDisplayItem::IsSettingsVisible)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						Toolbar.MakeWidget()
					]

					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SFoliageEditMeshDisplayItem::GetStaticMeshname)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
						.OnClicked( this, &SFoliageEditMeshDisplayItem::OnReplace )
						.ToolTipText(NSLOCTEXT("FoliageEdMode", "Replace_Tooltip", "Replace all instances with the Static Mesh currently selected in the Content Browser."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush(TEXT("ContentReference.UseSelectionFromContentBrowser")) )
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)	
						.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
						.OnClicked( this, &SFoliageEditMeshDisplayItem::OnSync )
						.ToolTipText(NSLOCTEXT("FoliageEdMode", "FindInContentBrowser_Tooltip", "Find this Static Mesh in the Content Browser."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush(TEXT("ContentReference.FindInContentBrowser")) )
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)	
						.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
						.OnClicked( this, &SFoliageEditMeshDisplayItem::OnRemove )
						.ToolTipText(NSLOCTEXT("FoliageEdMode", "Remove_Tooltip", "Delete all foliage instances of this Static Mesh."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush(TEXT("ContentReference.Clear")) )
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					SNew(SHorizontalBox)
					.Visibility(this, &SFoliageEditMeshDisplayItem::IsSettingsVisible)

					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SFoliageEditMeshDisplayItem::GetSettingsLabelText)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
						.OnClicked( this, &SFoliageEditMeshDisplayItem::OnOpenSettings )
						.ToolTipText(NSLOCTEXT("FoliageEdMode", "OpenSettings_Tooltip", "Use the InstancedFoliageSettings currently selected in the Content Browser."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush(TEXT("FoliageEditMode.OpenSettings")) )
						]
					]
				
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
						.OnClicked( this, &SFoliageEditMeshDisplayItem::OnSaveRemoveSettings )
						.ToolTipText(this, &SFoliageEditMeshDisplayItem::GetSaveRemoveSettingsTooltip)
						[
							SNew(SImage)
							.Image( this, &SFoliageEditMeshDisplayItem::GetSaveSettingsBrush )
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					.Visibility(this, &SFoliageEditMeshDisplayItem::IsNoSettingsVisible)

					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)

						+SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						[
							Toolbar.MakeWidget()																	
						]

						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SFoliageEditMeshDisplayItem::GetStaticMeshname)
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						ThumbnailBox.ToSharedRef()
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SVerticalBox)
					.Visibility(this, &SFoliageEditMeshDisplayItem::IsClusterSettingsVisible)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SVerticalBox)

							+SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text( this, &SFoliageEditMeshDisplayItem::GetInstanceCountString)
							]

							+SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text( this, &SFoliageEditMeshDisplayItem::GetInstanceClusterCountString)
							]
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(StandardPadding)
						[
							ThumbnailBox.ToSharedRef()		
						]
					]

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						ClusterSettingsDetails.ToSharedRef()
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					PaintSettings
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFoliageEditMeshDisplayItem::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged )
{
	FoliageEditPtr.Pin()->GetFoliageEditMode()->ReallocateClusters(AssociatedStaticMesh);
}

void SFoliageEditMeshDisplayItem::BindCommands()
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();

	UICommandList->MapAction(
		Commands.SetNoSettings,
		FExecuteAction::CreateSP( this, &SFoliageEditMeshDisplayItem::OnNoSettings ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEditMeshDisplayItem::IsNoSettingsEnabled ) );

	UICommandList->MapAction(
		Commands.SetPaintSettings,
		FExecuteAction::CreateSP( this, &SFoliageEditMeshDisplayItem::OnPaintSettings ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEditMeshDisplayItem::IsPaintSettingsEnabled ) );

	UICommandList->MapAction(
		Commands.SetClusterSettings,
		FExecuteAction::CreateSP( this, &SFoliageEditMeshDisplayItem::OnClusterSettings ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEditMeshDisplayItem::IsClusterSettingsEnabled ) );
}

ECurrentViewSettings::Type SFoliageEditMeshDisplayItem::GetCurrentDisplayStatus() const
{
	return CurrentViewSettings;
}

void SFoliageEditMeshDisplayItem::SetCurrentDisplayStatus(ECurrentViewSettings::Type InDisplayStatus)
{
	CurrentViewSettings = InDisplayStatus;
}

void SFoliageEditMeshDisplayItem::Replace(UInstancedFoliageSettings* InFoliageSettingsPtr, UStaticMesh* InAssociatedStaticMesh, TSharedPtr<FAssetThumbnail> InAssetThumbnail, TSharedPtr<FFoliageMeshUIInfo> InFoliageMeshUIInfo)
{
	FoliageSettingsPtr = InFoliageSettingsPtr;
	AssociatedStaticMesh = InAssociatedStaticMesh;
	FoliageMeshUIInfo = InFoliageMeshUIInfo;
	Thumbnail = InAssetThumbnail;
	ThumbnailWidget = Thumbnail->MakeThumbnailWidget();

	ThumbnailWidgetBorder->SetContent(ThumbnailWidget.ToSharedRef());
}

void SFoliageEditMeshDisplayItem::OnNoSettings()
{
	CurrentViewSettings = ECurrentViewSettings::ShowNone;
}

bool SFoliageEditMeshDisplayItem::IsNoSettingsEnabled() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowNone;
}

void SFoliageEditMeshDisplayItem::OnPaintSettings()
{
	CurrentViewSettings = ECurrentViewSettings::ShowPaintSettings;
}

bool SFoliageEditMeshDisplayItem::IsPaintSettingsEnabled() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowPaintSettings;
}

void SFoliageEditMeshDisplayItem::OnClusterSettings()
{
	CurrentViewSettings = ECurrentViewSettings::ShowClusterSettings;
}

bool SFoliageEditMeshDisplayItem::IsClusterSettingsEnabled() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowClusterSettings;
}

FString SFoliageEditMeshDisplayItem::GetStaticMeshname() const
{
	return AssociatedStaticMesh->GetName();
}

FString SFoliageEditMeshDisplayItem::GetSettingsLabelText() const
{
	return FoliageSettingsPtr->GetOuter()->IsA(UPackage::StaticClass()) ? *FoliageSettingsPtr->GetPathName() : LOCTEXT("NotUsingSharedSettings", "(not using shared settings)").ToString();
}

EVisibility  SFoliageEditMeshDisplayItem::IsNoSettingsVisible() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowNone? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility  SFoliageEditMeshDisplayItem::IsSettingsVisible() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowNone? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SFoliageEditMeshDisplayItem::IsPaintSettingsVisible() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowPaintSettings? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliageEditMeshDisplayItem::IsClusterSettingsVisible() const
{
	return CurrentViewSettings == ECurrentViewSettings::ShowClusterSettings? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SFoliageEditMeshDisplayItem::OnReplace()
{
	if(AssociatedStaticMesh)
	{
		// Get current selection from content browser
		FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
		USelection* SelectedSet = GEditor->GetSelectedSet( UStaticMesh::StaticClass() );
		UStaticMesh* SelectedStaticMesh = Cast<UStaticMesh>( SelectedSet->GetTop( UStaticMesh::StaticClass() ) );
		if( SelectedStaticMesh != NULL )
		{
			FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
			
			bool bMeshMerged = false;
			if(Mode->ReplaceStaticMesh(AssociatedStaticMesh, SelectedStaticMesh, bMeshMerged))
			{
				// If they were merged, simply remove the current item. Otherwise replace it.
				if(bMeshMerged)
				{
					FoliageEditPtr.Pin()->RemoveItemFromScrollbox(SharedThis(this));
				}
				else
				{
					FoliageEditPtr.Pin()->ReplaceItem(SharedThis(this), SelectedStaticMesh);
				}
			}
		}
	}


	return FReply::Handled();
}

FSlateColor SFoliageEditMeshDisplayItem::GetBorderColor() const
{
	if(FoliageSettingsPtr->IsSelected)
	{
		return 	FSlateColor(FLinearColor(0.828f, 0.364f, 0.003f));
	}

	return FSlateColor(FLinearColor(0.25f, 0.25f, 0.25f));
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsSelected() const
{
	return FoliageSettingsPtr->IsSelected? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnSelectionChanged(ESlateCheckBoxState::Type InType)
{
	FoliageSettingsPtr->IsSelected = InType == ESlateCheckBoxState::Checked;
}

FReply SFoliageEditMeshDisplayItem::OnSync()
{
	TArray<UObject*> Objects;
	Objects.Add(AssociatedStaticMesh);
	GEditor->SyncBrowserToObjects(Objects);

	return FReply::Handled();
}

FReply SFoliageEditMeshDisplayItem::OnRemove()
{
	FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
	
	if(Mode->RemoveFoliageMesh(AssociatedStaticMesh))
	{
		FoliageEditPtr.Pin()->RemoveItemFromScrollbox(SharedThis(this));
	}

	return FReply::Handled();
}

FReply SFoliageEditMeshDisplayItem::OnSaveRemoveSettings()
{
	FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );

	if(FoliageSettingsPtr->GetOuter()->IsA(UPackage::StaticClass()))
	{
		UInstancedFoliageSettings* NewSettings = NULL;
		NewSettings = Mode->CopySettingsObject(AssociatedStaticMesh);

		// Do not replace the current one if NULL is returned, just keep the old one.
		if(NewSettings)
		{
			FoliageSettingsPtr = NewSettings;
		}
	}
	else
	{
		// Build default settings asset name and path
		FString DefaultAsset = FPackageName::GetLongPackagePath(AssociatedStaticMesh->GetOutermost()->GetName()) + TEXT("/") + AssociatedStaticMesh->GetName() + TEXT("_settings");

		TSharedRef<SDlgPickAssetPath> SettingDlg = 
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("SettingsDialogTitle", "Choose Location for Foliage Settings Asset"))
			.DefaultAssetPath(FText::FromString(DefaultAsset));

		if (SettingDlg->ShowModal() != EAppReturnType::Cancel)
		{
			FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
			UInstancedFoliageSettings* NewSettings = NULL;

			NewSettings = Mode->SaveSettingsObject(SettingDlg->GetFullAssetPath(), AssociatedStaticMesh);

			// Do not replace the current one if NULL is returned, just keep the old one.
			if(NewSettings)
			{
				FoliageSettingsPtr = NewSettings;
			}
		}
	}

	return FReply::Handled();
}

FReply SFoliageEditMeshDisplayItem::OnOpenSettings()
{
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();
	USelection* SelectedSet = GEditor->GetSelectedSet( UInstancedFoliageSettings::StaticClass() );
	UInstancedFoliageSettings* SelectedSettings = Cast<UInstancedFoliageSettings>( SelectedSet->GetTop( UInstancedFoliageSettings::StaticClass() ) );
	if( SelectedSettings != NULL )
	{
		FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
		Mode->ReplaceSettingsObject(AssociatedStaticMesh, SelectedSettings);
		FoliageSettingsPtr = SelectedSettings;
	}

	return FReply::Handled();
}

FString SFoliageEditMeshDisplayItem::GetSaveRemoveSettingsTooltip() const
{
	// Remove Settings tooltip.
	if(FoliageSettingsPtr->GetOuter()->IsA(UPackage::StaticClass()))
	{
		return NSLOCTEXT("FoliageEdMode", "RemoveSettings_Tooltip", "Do not store the foliage settings in a shared InstancedFoliageSettings object.").ToString();
	}

	// Save settings tooltip.
	return NSLOCTEXT("FoliageEdMode", "SaveSettings_Tooltip", "Save these settings as an InstancedFoliageSettings object stored in a package.").ToString();
}

bool SFoliageEditMeshDisplayItem::IsPropertyVisible(UProperty const * const InProperty) const
{
	const FString Category = FObjectEditorUtils::GetCategory(InProperty);
	return Category == TEXT("Clustering") || Category == TEXT("Culling") || Category == TEXT("Lighting");
}

FString SFoliageEditMeshDisplayItem::GetInstanceCountString() const
{
	return FString::Printf(*LOCTEXT("InstanceCount_Value", "Instance Count: %d").ToString(), FoliageMeshUIInfo->MeshInfo->GetInstanceCount());
}

FString SFoliageEditMeshDisplayItem::GetInstanceClusterCountString() const
{
	return FString::Printf(*LOCTEXT("ClusterCount_Value", "Cluster Count: %d").ToString(), FoliageMeshUIInfo->MeshInfo->InstanceClusters.Num());
}

EVisibility SFoliageEditMeshDisplayItem::IsReapplySettingsVisible() const
{
	FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
	return Mode->UISettings.GetReapplyToolSelected()? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliageEditMeshDisplayItem::IsReapplySettingsVisible_Dummy() const
{
	FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
	return Mode->UISettings.GetReapplyToolSelected()? EVisibility::Hidden : EVisibility::Collapsed;
}

EVisibility SFoliageEditMeshDisplayItem::IsNotReapplySettingsVisible() const
{
	FEdModeFoliage* Mode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );
	return Mode->UISettings.GetReapplyToolSelected()? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SFoliageEditMeshDisplayItem::IsUniformScalingVisible() const
{
	return FoliageSettingsPtr->UniformScale? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible() const
{
	return FoliageSettingsPtr->UniformScale? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SFoliageEditMeshDisplayItem::IsNonUniformScalingVisible_Dummy() const
{
	return FoliageSettingsPtr->UniformScale? EVisibility::Collapsed : EVisibility::Hidden;
}

void SFoliageEditMeshDisplayItem::OnDensityReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyDensity = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsDensityReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyDensity? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnDensityChanged(float InValue)
{
	FoliageSettingsPtr->Density = InValue;
}

float SFoliageEditMeshDisplayItem::GetDensity() const
{
	return FoliageSettingsPtr->Density;
}

void SFoliageEditMeshDisplayItem::OnDensityReapplyChanged(float InValue)
{
	FoliageSettingsPtr->ReapplyDensityAmount = InValue;
}

float SFoliageEditMeshDisplayItem::GetDensityReapply() const
{
	return FoliageSettingsPtr->ReapplyDensityAmount;
}

void SFoliageEditMeshDisplayItem::OnRadiusReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyRadius = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsRadiusReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyRadius? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnRadiusChanged(float InValue)
{
	FoliageSettingsPtr->Radius = InValue;
}

float SFoliageEditMeshDisplayItem::GetRadius() const
{
	return FoliageSettingsPtr->Radius;
}

void SFoliageEditMeshDisplayItem::OnAlignToNormalReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyAlignToNormal = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsAlignToNormalReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyAlignToNormal? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnAlignToNormal(ESlateCheckBoxState::Type InState)
{
	if(InState == ESlateCheckBoxState::Checked)
	{
		FoliageSettingsPtr->AlignToNormal = true;
	}
	else
	{
		FoliageSettingsPtr->AlignToNormal = false;
	}
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsAlignToNormalChecked() const
{
	return FoliageSettingsPtr->AlignToNormal? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

EVisibility SFoliageEditMeshDisplayItem::IsAlignToNormalVisible() const
{
	return FoliageSettingsPtr->AlignToNormal? EVisibility::Visible : EVisibility::Collapsed;
}

void SFoliageEditMeshDisplayItem::OnMaxAngleChanged(float InValue)
{
	FoliageSettingsPtr->AlignMaxAngle = InValue;
}

float SFoliageEditMeshDisplayItem::GetMaxAngle() const
{
	return FoliageSettingsPtr->AlignMaxAngle;
}

void SFoliageEditMeshDisplayItem::OnRandomYawReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyRandomYaw = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsRandomYawReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyRandomYaw? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnRandomYaw(ESlateCheckBoxState::Type InState)
{
	if(InState == ESlateCheckBoxState::Checked)
	{
		FoliageSettingsPtr->RandomYaw = true;
	}
	else
	{
		FoliageSettingsPtr->RandomYaw = false;
	}
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsRandomYawChecked() const
{
	return FoliageSettingsPtr->RandomYaw? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnUniformScale(ESlateCheckBoxState::Type InState)
{
	if(InState == ESlateCheckBoxState::Checked)
	{
		FoliageSettingsPtr->UniformScale = true;

		FoliageSettingsPtr->ScaleMinY = FoliageSettingsPtr->ScaleMinX;
		FoliageSettingsPtr->ScaleMinZ = FoliageSettingsPtr->ScaleMinX;
	}
	else
	{
		FoliageSettingsPtr->UniformScale = false;
	}
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsUniformScaleChecked() const
{
	return FoliageSettingsPtr->UniformScale? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnScaleUniformReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyScaleX = InState == ESlateCheckBoxState::Checked? true : false;
	FoliageSettingsPtr->ReapplyScaleY = InState == ESlateCheckBoxState::Checked? true : false;
	FoliageSettingsPtr->ReapplyScaleZ = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleUniformReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyScaleX? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnScaleUniformMinChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMinX = InValue;
	FoliageSettingsPtr->ScaleMinY = InValue;
	FoliageSettingsPtr->ScaleMinZ = InValue;

	FoliageSettingsPtr->ScaleMaxX = FMath::Max(FoliageSettingsPtr->ScaleMaxX, InValue);
	FoliageSettingsPtr->ScaleMaxY = FoliageSettingsPtr->ScaleMaxX;
	FoliageSettingsPtr->ScaleMaxZ = FoliageSettingsPtr->ScaleMaxX;
}

float SFoliageEditMeshDisplayItem::GetScaleUniformMin() const
{
	return FoliageSettingsPtr->ScaleMinX;
}

void SFoliageEditMeshDisplayItem::OnScaleUniformMaxChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMaxX = InValue;
	FoliageSettingsPtr->ScaleMaxY = InValue;
	FoliageSettingsPtr->ScaleMaxZ = InValue;

	FoliageSettingsPtr->ScaleMinX = FMath::Min(FoliageSettingsPtr->ScaleMinX, InValue);
	FoliageSettingsPtr->ScaleMinY = FoliageSettingsPtr->ScaleMinX;
	FoliageSettingsPtr->ScaleMinZ = FoliageSettingsPtr->ScaleMinX;
}

float SFoliageEditMeshDisplayItem::GetScaleUniformMax() const
{
	return FoliageSettingsPtr->ScaleMaxX;
}

void SFoliageEditMeshDisplayItem::OnScaleXReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyScaleX = InState == ESlateCheckBoxState::Checked;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleXReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyScaleX? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnScaleXMinChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMinX = InValue;

	FoliageSettingsPtr->ScaleMaxX = FMath::Max(FoliageSettingsPtr->ScaleMaxX, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleXMin() const
{
	return FoliageSettingsPtr->ScaleMinX;
}

void SFoliageEditMeshDisplayItem::OnScaleXMaxChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMaxX = InValue;

	FoliageSettingsPtr->ScaleMinX = FMath::Min(FoliageSettingsPtr->ScaleMinX, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleXMax() const
{
	return FoliageSettingsPtr->ScaleMaxX;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleXLockedChecked() const
{
	return FoliageSettingsPtr->LockScaleX;
}

void SFoliageEditMeshDisplayItem::OnScaleXLocked(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->LockScaleX = InState == ESlateCheckBoxState::Checked;
}

void SFoliageEditMeshDisplayItem::OnScaleYReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyScaleY = InState == ESlateCheckBoxState::Checked;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleYReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyScaleY? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnScaleYMinChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMinY = InValue;

	FoliageSettingsPtr->ScaleMaxY = FMath::Max(FoliageSettingsPtr->ScaleMaxY, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleYMin() const
{
	return FoliageSettingsPtr->ScaleMinY;
}

void SFoliageEditMeshDisplayItem::OnScaleYMaxChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMaxY = InValue;

	FoliageSettingsPtr->ScaleMinY = FMath::Min(FoliageSettingsPtr->ScaleMinY, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleYMax() const
{
	return FoliageSettingsPtr->ScaleMaxY;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleYLockedChecked() const
{
	return FoliageSettingsPtr->LockScaleY;
}

void SFoliageEditMeshDisplayItem::OnScaleYLocked(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->LockScaleY = InState == ESlateCheckBoxState::Checked;
}

void SFoliageEditMeshDisplayItem::OnScaleZReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyScaleZ = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleZReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyScaleZ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnScaleZMinChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMinZ = InValue;

	FoliageSettingsPtr->ScaleMaxZ = FMath::Max(FoliageSettingsPtr->ScaleMaxZ, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleZMin() const
{
	return FoliageSettingsPtr->ScaleMinZ;
}

void SFoliageEditMeshDisplayItem::OnScaleZMaxChanged(float InValue)
{
	FoliageSettingsPtr->ScaleMaxZ = InValue;

	FoliageSettingsPtr->ScaleMinZ = FMath::Min(FoliageSettingsPtr->ScaleMinZ, InValue);
}

float SFoliageEditMeshDisplayItem::GetScaleZMax() const
{
	return FoliageSettingsPtr->ScaleMaxZ;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsScaleZLockedChecked() const
{
	return FoliageSettingsPtr->LockScaleZ;
}

void SFoliageEditMeshDisplayItem::OnScaleZLocked(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->LockScaleZ = InState == ESlateCheckBoxState::Checked;
}

void SFoliageEditMeshDisplayItem::OnZOffsetReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyZOffset = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsZOffsetReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyZOffset? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnZOffsetMin(float InValue)
{
	FoliageSettingsPtr->ZOffsetMin = InValue;

	FoliageSettingsPtr->ZOffsetMax = FMath::Max(FoliageSettingsPtr->ZOffsetMax, InValue);
}

float SFoliageEditMeshDisplayItem::GetZOffsetMin() const
{
	return FoliageSettingsPtr->ZOffsetMin;
}

void SFoliageEditMeshDisplayItem::OnZOffsetMax(float InValue)
{
	FoliageSettingsPtr->ZOffsetMax = InValue;

	FoliageSettingsPtr->ZOffsetMin = FMath::Min(FoliageSettingsPtr->ZOffsetMin, InValue);
}

float SFoliageEditMeshDisplayItem::GetZOffsetMax() const
{
	return FoliageSettingsPtr->ZOffsetMax;
}

void SFoliageEditMeshDisplayItem::OnRandomPitchReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyRandomPitchAngle = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsRandomPitchReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyRandomPitchAngle? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnRandomPitchChanged(float InValue)
{
	FoliageSettingsPtr->RandomPitchAngle = InValue;
}

float SFoliageEditMeshDisplayItem::GetRandomPitch() const
{
	return FoliageSettingsPtr->RandomPitchAngle;
}

void SFoliageEditMeshDisplayItem::OnGroundSlopeReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyGroundSlope = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsGroundSlopeReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyGroundSlope? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnGroundSlopeChanged(float InValue)
{
	FoliageSettingsPtr->GroundSlope = InValue;
}

float SFoliageEditMeshDisplayItem::GetGroundSlope() const
{
	return FoliageSettingsPtr->GroundSlope;
}

void SFoliageEditMeshDisplayItem::OnHeightReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyHeight = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsHeightReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyHeight? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnHeightMinChanged(float InValue)
{
	FoliageSettingsPtr->HeightMin = InValue;

	FoliageSettingsPtr->HeightMax = FMath::Max(FoliageSettingsPtr->HeightMax, InValue);
}

float SFoliageEditMeshDisplayItem::GetHeightMin() const
{
	return FoliageSettingsPtr->HeightMin;
}

void SFoliageEditMeshDisplayItem::OnHeightMaxChanged(float InValue)
{
	FoliageSettingsPtr->HeightMax = InValue;

	FoliageSettingsPtr->HeightMin = FMath::Min(FoliageSettingsPtr->HeightMin, InValue);
}

float SFoliageEditMeshDisplayItem::GetHeightMax() const
{
	return FoliageSettingsPtr->HeightMax;
}

void SFoliageEditMeshDisplayItem::OnLandscapeLayerReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyLandscapeLayer = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsLandscapeLayerReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyLandscapeLayer? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnLandscapeLayerChanged(const FText& InValue)
{
	FoliageSettingsPtr->LandscapeLayer = FName(  *InValue.ToString() );
}

FText SFoliageEditMeshDisplayItem::GetLandscapeLayer() const
{
	return FText::FromName(FoliageSettingsPtr->LandscapeLayer);
}

void SFoliageEditMeshDisplayItem::OnCollisionWithWorld(ESlateCheckBoxState::Type InState)
{
	if(InState == ESlateCheckBoxState::Checked)
	{
		FoliageSettingsPtr->CollisionWithWorld = true;
	}
	else
	{
		FoliageSettingsPtr->CollisionWithWorld = false;
	}
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsCollisionWithWorldChecked() const
{
	return FoliageSettingsPtr->CollisionWithWorld? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

EVisibility SFoliageEditMeshDisplayItem::IsCollisionWithWorldVisible() const
{
	return FoliageSettingsPtr->CollisionWithWorld? EVisibility::Visible : EVisibility::Collapsed;
}

void SFoliageEditMeshDisplayItem::OnCollisionWithWorldReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyCollisionWithWorld = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsCollisionWithWorldReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyCollisionWithWorld? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnCollisionScaleXChanged(float InValue)
{
	FoliageSettingsPtr->CollisionScale.X = InValue;
}

void SFoliageEditMeshDisplayItem::OnCollisionScaleYChanged(float InValue)
{
	FoliageSettingsPtr->CollisionScale.Y = InValue;
}

void SFoliageEditMeshDisplayItem::OnCollisionScaleZChanged(float InValue)
{
	FoliageSettingsPtr->CollisionScale.Z = InValue;
}

float SFoliageEditMeshDisplayItem::GetCollisionScaleX() const
{
	return FoliageSettingsPtr->CollisionScale.X;
}

float SFoliageEditMeshDisplayItem::GetCollisionScaleY() const
{
	return FoliageSettingsPtr->CollisionScale.Y;
}

float SFoliageEditMeshDisplayItem::GetCollisionScaleZ() const
{
	return FoliageSettingsPtr->CollisionScale.Z;
}

void SFoliageEditMeshDisplayItem::OnVertexColorMask(ESlateCheckBoxState::Type InState, FoliageVertexColorMask Mask)
{
	FoliageSettingsPtr->VertexColorMask = Mask;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsVertexColorMaskChecked(FoliageVertexColorMask Mask) const
{
	return FoliageSettingsPtr->VertexColorMask == Mask ?  ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnVertexColorMaskThresholdChanged(float InValue)
{
	FoliageSettingsPtr->VertexColorMaskThreshold = InValue;
}

float SFoliageEditMeshDisplayItem::GetVertexColorMaskThreshold() const
{
	return FoliageSettingsPtr->VertexColorMaskThreshold;
}

EVisibility SFoliageEditMeshDisplayItem::IsVertexColorMaskThresholdVisible() const
{
	return (FoliageSettingsPtr->VertexColorMask == FOLIAGEVERTEXCOLORMASK_Disabled) ? EVisibility::Collapsed : EVisibility::Visible;
}

void SFoliageEditMeshDisplayItem::OnVertexColorMaskInvert(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->VertexColorMaskInvert = (InState == ESlateCheckBoxState::Checked);
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsVertexColorMaskInvertChecked() const
{
	return FoliageSettingsPtr->VertexColorMaskInvert ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEditMeshDisplayItem::OnVertexColorMaskReapply(ESlateCheckBoxState::Type InState)
{
	FoliageSettingsPtr->ReapplyVertexColorMask = InState == ESlateCheckBoxState::Checked? true : false;
}

ESlateCheckBoxState::Type SFoliageEditMeshDisplayItem::IsVertexColorMaskReapplyChecked() const
{
	return FoliageSettingsPtr->ReapplyVertexColorMask? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

FReply SFoliageEditMeshDisplayItem::OnMouseDownSelection( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FoliageSettingsPtr->IsSelected = !FoliageSettingsPtr->IsSelected;
	return FReply::Handled();
}

const FSlateBrush* SFoliageEditMeshDisplayItem::GetSaveSettingsBrush() const
{
	if(FoliageSettingsPtr->GetOuter()->IsA(UPackage::StaticClass()))
	{
		return FEditorStyle::GetBrush(TEXT("FoliageEditMode.DeleteItem"));
	}

	return FEditorStyle::GetBrush(TEXT("FoliageEditMode.SaveSettings"));
}

#undef LOCTEXT_NAMESPACE
