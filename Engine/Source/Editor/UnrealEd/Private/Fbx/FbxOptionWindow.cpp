// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "FbxOptionWindow.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "FBXOption"

DECLARE_DELEGATE_OneParam( FOnImportTypeChanged, EFBXImportType );

class SImportTypeButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SImportTypeButton )
	:	_ImportType(FBXIT_StaticMesh)
	,	_OnSelectionChanged()
	{}
		SLATE_ARGUMENT( EFBXImportType, ImportType )
		SLATE_EVENT( FOnImportTypeChanged, OnSelectionChanged )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		CurrentChoice = InArgs._ImportType;
		OnSelectionChanged = InArgs._OnSelectionChanged;

		this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight() 
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot() .FillWidth(1)
				[
					CreateRadioButton( LOCTEXT("ImportTypeButton_StaticMesh", "Static Mesh").ToString(), FBXIT_StaticMesh )
				]
				+SHorizontalBox::Slot() .FillWidth(1)
				[
					CreateRadioButton( LOCTEXT("ImportTypeButton_SkeletalMesh", "Skeletal Mesh").ToString(), FBXIT_SkeletalMesh )
				]
				+SHorizontalBox::Slot() .FillWidth(1)
				[
					CreateRadioButton( LOCTEXT("ImportTypeButton_Animation", "Animation").ToString(), FBXIT_Animation )
				]
			]
		];
	}

private:

	TSharedRef<SWidget> CreateRadioButton( const FString& RadioText, EFBXImportType RadioButtonChoice )
	{
		return
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "RadioButton")
			.IsChecked( this, &SImportTypeButton::IsRadioChecked, RadioButtonChoice )
			.OnCheckStateChanged( this, &SImportTypeButton::OnRadioChanged, RadioButtonChoice )
			[
				SNew(STextBlock).Text(RadioText)
			];
	}

	ESlateCheckBoxState::Type IsRadioChecked( EFBXImportType ButtonId ) const
	{
		return (CurrentChoice == ButtonId)
			? ESlateCheckBoxState::Checked
			: ESlateCheckBoxState::Unchecked;
	}

	void OnRadioChanged( ESlateCheckBoxState::Type NewRadioState, EFBXImportType RadioThatChanged )
	{
		if (NewRadioState == ESlateCheckBoxState::Checked)
		{
			CurrentChoice = RadioThatChanged;

			if (OnSelectionChanged.IsBound())
			{
				OnSelectionChanged.Execute(CurrentChoice);
			}
		}
	}

	EFBXImportType CurrentChoice;
	FOnImportTypeChanged OnSelectionChanged;
};

DECLARE_DELEGATE_OneParam( FOnAnimImportLengthOptionChanged, EFBXAnimationLengthImportType );

class SAnimImportLengthOption : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAnimImportLengthOption )
		:	_AnimationLengthOption(FBXALIT_ExportedTime)
		,	_OnSelectionChanged()
		,	_OnTextCommittedRange1()
		,	_OnTextCommittedRange2()
	{}
		SLATE_ARGUMENT( EFBXAnimationLengthImportType, AnimationLengthOption )
		SLATE_EVENT( FOnAnimImportLengthOptionChanged, OnSelectionChanged )
		SLATE_EVENT( FOnTextCommitted,	OnTextCommittedRange1)
		SLATE_EVENT( FOnTextCommitted,	OnTextCommittedRange2)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		CurrentChoice = InArgs._AnimationLengthOption;
		OnSelectionChanged = InArgs._OnSelectionChanged;

		this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight() .Padding(3)
			[
				CreateRadioButton( LOCTEXT("AnimImportLengthOption_ExportTime", "Exported Time").ToString(), FBXALIT_ExportedTime )
			]
			+SVerticalBox::Slot().AutoHeight() .Padding(3)
			[
				CreateRadioButton( LOCTEXT("AnimImportLengthOption_AnimTime", "Animated Time").ToString(), FBXALIT_AnimatedKey )
			]
			+SVerticalBox::Slot().AutoHeight() .Padding(3)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot().AutoWidth() .Padding(2)
				[
					CreateRadioButton( LOCTEXT("AnimImportLengthOption_SetRange", "Set Range").ToString(), FBXALIT_SetRange )
				]

				+SHorizontalBox::Slot().AutoWidth() .Padding(2) 
				[
					SNew(SEditableTextBox)
					.MinDesiredWidth(20)
					.OnTextCommitted(InArgs._OnTextCommittedRange1)
					.IsEnabled(this, &SAnimImportLengthOption::CanEnterAnimationRange)
				]

				+SHorizontalBox::Slot().AutoWidth() .Padding(2)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RangeSeparator", "-"))
				]
				+SHorizontalBox::Slot().AutoWidth() .Padding(2)
				[
					SNew(SEditableTextBox)
					.MinDesiredWidth(20)
					.OnTextCommitted(InArgs._OnTextCommittedRange2)
					.IsEnabled(this, &SAnimImportLengthOption::CanEnterAnimationRange)
				]

				+SHorizontalBox::Slot().AutoWidth() .Padding(2)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AnimImportLengthOption_InFrameNumber", " In Frame Number").ToString())
				]
			]
		];
	}

private:

	TSharedRef<SWidget> CreateRadioButton( const FString& RadioText, EFBXAnimationLengthImportType RadioButtonChoice )
	{
		return
			SNew(SCheckBox)
			.Style(FEditorStyle::Get(), "RadioButton")
			.IsChecked( this, &SAnimImportLengthOption::IsRadioChecked, RadioButtonChoice )
			.OnCheckStateChanged( this, &SAnimImportLengthOption::OnRadioChanged, RadioButtonChoice )
			[
				SNew(STextBlock).Text(RadioText)
			];
	}

	ESlateCheckBoxState::Type IsRadioChecked( EFBXAnimationLengthImportType ButtonId ) const
	{
		return (CurrentChoice == ButtonId)
			? ESlateCheckBoxState::Checked
			: ESlateCheckBoxState::Unchecked;
	}

	bool CanEnterAnimationRange() const { return CurrentChoice == FBXALIT_SetRange; }

	void OnRadioChanged( ESlateCheckBoxState::Type NewRadioState, EFBXAnimationLengthImportType RadioThatChanged )
	{
		if (NewRadioState == ESlateCheckBoxState::Checked)
		{
			CurrentChoice = RadioThatChanged;

			if (OnSelectionChanged.IsBound())
			{
				OnSelectionChanged.Execute(CurrentChoice);
			}
		}
	}

	EFBXAnimationLengthImportType CurrentChoice;
	FOnAnimImportLengthOptionChanged OnSelectionChanged;
};

void SFbxOptionWindow::Construct(const FArguments& InArgs)
{
	ImportUI = InArgs._ImportUI;
	WidgetWindow = InArgs._WidgetWindow;

	check (ImportUI);

	UStaticMesh::GetLODGroups(StaticMeshLODGroupNames);
	UStaticMesh::GetLODGroupsDisplayNames(StaticMeshLODGroupDisplayNames);

	for (int32 GroupIndex = 0; GroupIndex < StaticMeshLODGroupNames.Num(); ++GroupIndex)
	{
		StaticMeshLODGroups.Add(MakeShareable(new FString(StaticMeshLODGroupDisplayNames[GroupIndex].ToString())));
	}

	bReimport = InArgs._ForcedImportType.IsSet();

	// Force the import type
	if( bReimport )
	{
		ImportUI->MeshTypeToImport = InArgs._ForcedImportType.GetValue();
	}

	// clear Import UI bug
	ImportUI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
	ImportUI->AnimSequenceImportData->StartFrame = 0;
	ImportUI->AnimSequenceImportData->EndFrame = 0;

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
		
			SNew(SVerticalBox)

			// first 3 radio box for staticmesh/skeletalmesh/animation		
			+SVerticalBox::Slot().AutoHeight() .Padding(5)
			[
				SNew(STextBlock)
				.Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
				.Text(InArgs._FullPath)
			]

			// first 3 radio box for staticmesh/skeletalmesh/animation		
			+SVerticalBox::Slot().AutoHeight() .Padding(5)
			[
				SNew(SImportTypeButton)
				.ImportType(ImportUI->MeshTypeToImport)
				// Do not allow users to change the import type if it is forced to a specific type
				.IsEnabled( !bReimport )
				.OnSelectionChanged(this, &SFbxOptionWindow::SetImportType)
			]

			+SVerticalBox::Slot().AutoHeight() .Padding(5)
			[
				SNew(SSeparator)
			]

			+SVerticalBox::Slot().AutoHeight() .Padding(5)
			.Expose(CustomBox)

			+SVerticalBox::Slot().AutoHeight() .Padding(5)
			[
				SNew(SSeparator)
			]

			+SVerticalBox::Slot().AutoHeight() .Padding(5) .HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot() .FillWidth(1)
				[
					SAssignNew( ImportButton, SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("FbxOptionWindow_Import", "Import").ToString())
					.IsEnabled(this, &SFbxOptionWindow::CanImport)
					.OnClicked_Raw( this, &SFbxOptionWindow::OnImport )
				]

				+SHorizontalBox::Slot() .FillWidth(1)
				[
					SNew(SButton) .HAlign(HAlign_Center)
					.Text(LOCTEXT("FbxOptionWindow_Cancel", "Cancel").ToString())
					.OnClicked_Raw( this, &SFbxOptionWindow::OnCancel )
				]
			]
		]
	];

	if(WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->SetWidgetToFocusOnActivate(ImportButton);
	}

	RefreshWindow();
}

void SFbxOptionWindow::RefreshWindow()
{
	TSharedPtr<SVerticalBox> VerticalBox;

	(*CustomBox)
	[
		SAssignNew( VerticalBox, SVerticalBox )
	];

	if (ImportUI->MeshTypeToImport == FBXIT_StaticMesh)
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			ConstructStaticMeshBasic()
		];

		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew( SExpandableArea )
			.InitiallyCollapsed(true)
			.AreaTitle( LOCTEXT("SFbxOptionWindow_StaticAdvanced", "Advanced").ToString() )
			.BodyContent()
			[
				SNew (SVerticalBox)

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructStaticMeshAdvanced()
				]

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructMaterialOption()
				]

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructMiscOption()
				]
			]
		];
	}
	else if (ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh)
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			ConstructSkeletonOptionForMesh()
		];

		VerticalBox->AddSlot()
		.AutoHeight()
		[
			ConstructSkeletalMeshBasic()
		];


		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew( SExpandableArea )
			.InitiallyCollapsed(true)
			.AreaTitle( LOCTEXT("SFbxOptionWindow_SkeletalAdvanced", "Advanced").ToString() )
			.BodyContent()
			[
				SNew (SVerticalBox)

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructSkeletalMeshAdvanced()
				]

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructMaterialOption()
				]

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructMiscOption()
				]
			]
		];
	}
	else 	
	{
		VerticalBox->AddSlot()
		.AutoHeight()
		[
			ConstructSkeletonOptionForAnim()
		];

		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew( SExpandableArea )
			.InitiallyCollapsed(true)
			.AreaTitle( LOCTEXT("SFbxOptionWindow_DefaultAdvanced", "Advanced").ToString() )
			.BodyContent()
			[
				SNew (SVerticalBox)

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructAnimationOption()
				]

				+SVerticalBox::Slot().AutoHeight()
				[
					ConstructMiscOption()
				]
			]
		];
	}
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructMiscOption()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text( LOCTEXT("FbxOptionWindow_Misc", "Misc").ToString() )
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bOverrideFullName? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetGeneral_OverrideFullName)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_NameOverride", "Override FullName").ToString())
		]
	];

	return NewBox;
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructNormalImportOptions()
{
	NormalImportOptions.Empty();
	for( int32 i = 0; i < FBXNIM_MAX; ++i )
	{
		NormalImportOptions.Add( TSharedPtr<EFBXNormalImportMethod>() );
	}

	NormalImportOptions[FBXNIM_ComputeNormals] = MakeShareable( new EFBXNormalImportMethod( FBXNIM_ComputeNormals ) );
	NormalImportOptions[FBXNIM_ImportNormals] = MakeShareable( new EFBXNormalImportMethod( FBXNIM_ImportNormals ) );
	NormalImportOptions[FBXNIM_ImportNormalsAndTangents] = MakeShareable( new EFBXNormalImportMethod( FBXNIM_ImportNormalsAndTangents ) );

	return 
	SNew( SHorizontalBox )
	+SHorizontalBox::Slot()
	.AutoWidth()
	.Padding(2)
	[
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew( STextBlock )
			.Text(LOCTEXT("FBXOptionWindow_NormalImport", "Normals").ToString())
		]
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(4)
		[
			SNew( SComboBox< TSharedPtr<EFBXNormalImportMethod> > )
			.ContentPadding(1.f)
			.ToolTipText(LOCTEXT("FBXOptionWindow_NormalInputMethod", "Options for importing normals and tangents").ToString())
			.OptionsSource( &NormalImportOptions )
			.InitiallySelectedItem( NormalImportOptions[ GetCurrentNormalImportMethod() ] )
			.OnSelectionChanged( this, &SFbxOptionWindow::OnNormalImportMethodChanged )
			.OnGenerateWidget( this, &SFbxOptionWindow::OnGenerateWidgetForComboItem )
			[
				SNew( STextBlock )
				.Text( this, &SFbxOptionWindow::OnGenerateTextForImportMethod, FBXNIM_MAX )
			]
		]
	];
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructStaticMeshBasic()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth()
			.Padding(2)
			.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_StaticMeshLODGroup", "LOD Group").ToString())
		]

		+SHorizontalBox::Slot()
			.Padding(2)
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
		[
			SNew(STextComboBox)
			.OptionsSource(&StaticMeshLODGroups)
			.InitiallySelectedItem(StaticMeshLODGroups[StaticMeshLODGroupNames.Find(NAME_None)])
			.OnSelectionChanged(this, &SFbxOptionWindow::SetStaticMesh_LODGroup)
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		ConstructNormalImportOptions()
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() 
			.Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bCombineMeshes? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetStaticMesh_CombineMeshes)
		]

		+SHorizontalBox::Slot()
			.Padding(2)
			.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_CombineMeshes", "Combine Meshes").ToString())
		]
	];


	return NewBox;
}
TSharedRef<SWidget> SFbxOptionWindow::ConstructStaticMeshAdvanced()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text( LOCTEXT("FbxOptionWindow_StaticMesh", "Mesh").ToString())
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->StaticMeshImportData->bImportMeshLODs? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetStaticMesh_ImportMeshLODs)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportMeshLODs", "Import Mesh LODs").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->StaticMeshImportData->bReplaceVertexColors? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetStaticMesh_ReplaceVertexColor)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ReplaceVertexColors", "Replace Vertex Colors").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->StaticMeshImportData->bRemoveDegenerates? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetStaticMesh_RemoveDegenerates)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_RemoveDegeneates", "Remove Degenerates").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->StaticMeshImportData->bOneConvexHullPerUCX? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetStaticMesh_OneConvexHullPerUCX)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_OneConvexHullPerUCX", "One Convex Hull Per UCX").ToString())
		]
	];

	return NewBox;
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructSkeletalMeshBasic()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot()
	.AutoHeight()
	.Padding(2)
	[
		ConstructNormalImportOptions()
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bImportMorphTargets? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ImportMorphTargets)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportMorphTargets", "Import Morph Targets").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bImportAnimations? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ImportAnimation)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportAnimation", "Import Animation").ToString())
		]

		+SHorizontalBox::Slot().AutoWidth(). Padding(2)
		[
			SNew(SEditableTextBox)
			.IsReadOnly(false)
			.ToolTipText(LOCTEXT("FbxOptionWindow_ImportAnimationToolTip", "Type animation name if you're importing animation").ToString())
			.Text( this, &SFbxOptionWindow::GetAnimationName)
			.MinDesiredWidth(50)
			.OnTextCommitted(this, &SFbxOptionWindow::SetSkeletalMesh_AnimationName)
			.IsEnabled(this, &SFbxOptionWindow::CanEnterAnimationName)
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bUpdateSkeletonReferencePose? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_UpdateSkeletonRefPose)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_UpdateSkeletonRefPose", "Update Skeleton Reference Pose").ToString())
		]
	];

	return NewBox;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SWidget> SFbxOptionWindow::ConstructSkeletalMeshAdvanced()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text(LOCTEXT("FbxOptionWindow_SkeletalMesh", "Mesh").ToString())
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bImportMeshLODs? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ImportMeshLODs)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportSkeltalMeshLODs", "Import Mesh LODs").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bImportRigidMesh? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ImportRigidMesh)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportRigidMesh", "Import Rigid Mesh").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bUseT0AsRefPose? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_UseT0AsRefPose)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_UseRefPose", "Use T0As Ref Pose").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bPreserveSmoothingGroups? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ReserveSmoothingGroups)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_PreserveSmoothingGroup", "Preserve Smoothing Group").ToString())
		]
	];


	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bKeepOverlappingVertices? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_KeepOverlappingVertices)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_KeepOverlappingVertices", "Keep Overlapping Vertices").ToString())
		]
	];
	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->SkeletalMeshImportData->bImportMeshesInBoneHierarchy? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_ImportMeshesInBoneHierarchy)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportMeshesInBoneHierarchy", "Import Meshes in Bone Hierarchy").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text(LOCTEXT("FbxOptionWindow_PhysicsAsset", "Collision").ToString())
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bCreatePhysicsAsset? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_CreatePhysicsAsset)
			.IsEnabled(!bReimport)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_CreatePhysicsAsset", "Create PhysicsAsset").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew (SVerticalBox)

		+SVerticalBox::Slot() .Padding(2) 
		[
			SNew(STextBlock)
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
			.Text( LOCTEXT("FbxOptionWindow_SelectPhysicsAsset", "Select PhysicsAsset").ToString() )
			.IsEnabled(this, &SFbxOptionWindow::ShouldShowPhysicsAssetPicker)
		]

		+SVerticalBox::Slot() .Padding(2) 
		[
			SAssignNew(PhysicsAssetPickerComboButton, SComboButton)
			.ContentPadding(1.f)
			.OnGetMenuContent( this, &SFbxOptionWindow::MakePhysicsAssetPickerMenu )
			.HasDownArrow(true)
			.ToolTipText(LOCTEXT("FBXOption", "Pick an skeleton from a popup menu").ToString())
			.IsEnabled(this, &SFbxOptionWindow::ShouldShowPhysicsAssetPicker)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &SFbxOptionWindow::GetPhysicsAssetDisplay)
			]
		]
	];

	return NewBox;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SFbxOptionWindow::ConstructMaterialOption()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text( LOCTEXT("FbxOptionWindow_Material", "Material").ToString() )
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bImportMaterials? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetMaterial_ImportMaterials)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportMaterials", "Import Materials").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bImportTextures? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetMaterial_ImportTextures)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_ImportTextures", "Import Textures").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->TextureImportData->bInvertNormalMaps? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetMaterial_InvertNormalMaps)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_InvertNormalMaps", "Invert Normal Maps").ToString())
		]
	];

	return NewBox;
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructAnimationOption()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox);

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12 ) )
		.Text( LOCTEXT("FbxOptionWindow_Animation", "Animation").ToString() )
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bUseDefaultSampleRate? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetSkeletalMesh_UseDefaultSampleRate)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_UseDefaultSampleRate", "Use Default Sample Rate").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked(ImportUI->bPreserveLocalTransform? true: false)
			.OnCheckStateChanged(this, &SFbxOptionWindow::SetAnimation_ReserveLocalTransform)
		]

		+SHorizontalBox::Slot().AutoWidth() .Padding(2)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FbxOptionWindow_PreserveLocalTransform", "Preserve Local Transform").ToString())
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
		.Text( LOCTEXT("FbxOptionWindow_AnimationLength", "Animation Length").ToString() )
		.ShadowOffset(FVector2D(1, 1))
	];

	NewBox->AddSlot() .Padding(4)
	[
		SNew(SAnimImportLengthOption)
			.AnimationLengthOption(ImportUI->AnimSequenceImportData->AnimationLength)
			.OnSelectionChanged(this, &SFbxOptionWindow::SetAnimLengthOption)
			.OnTextCommittedRange1(this, &SFbxOptionWindow::SetAnimationRangeStart)
			.OnTextCommittedRange2(this, &SFbxOptionWindow::SetAnimationRangeEnd)
	];

	return NewBox;
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructSkeletonOptionForMesh()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox)
		.ToolTip(IDocumentation::Get()->CreateToolTip(FText::FromString("Pick a skeleton for this mesh"), NULL, FString("Shared/Editors/Persona"), FString("Skeleton")));

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16 ) )
			.Text( LOCTEXT("FbxOptionWindow_SelectSkeletonForMesh", "Select Skeleton").ToString() )
			.ShadowOffset(FVector2D(2, 2))
		]

		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			IDocumentation::Get()->CreateAnchor(FString("Engine/Animation/Skeleton"))
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
		.Text( LOCTEXT("FbxOptionWindow_AutoCreateSkeleton", "If none is selected, it will create skeleton for this.").ToString() )
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SAssignNew(SkeletonPickerComboButton, SComboButton)
		.ContentPadding(1.f)
		.OnGetMenuContent( this, &SFbxOptionWindow::MakeSkeletonPickerMenu )
		.HasDownArrow(true)
		.IsEnabled( !bReimport )
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &SFbxOptionWindow::GetSkeletonDisplay)
		]
	];

	return NewBox;
}

TSharedRef<SWidget> SFbxOptionWindow::ConstructSkeletonOptionForAnim()
{
	TSharedRef<SVerticalBox> NewBox = SNew(SVerticalBox)
		.ToolTip(IDocumentation::Get()->CreateToolTip(FText::FromString("Pick a skeleton for this mesh"), NULL, FString("Shared/Editors/Persona"), FString("Skeleton")));;
	

	NewBox->AddSlot().AutoHeight() .Padding(2) 
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 16 ) )
			.Text( LOCTEXT("FbxOptionWindow_SelectSkeletonForAnim", "Select Skeleton").ToString() )
			.ShadowOffset(FVector2D(2, 2))
		]
		
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			IDocumentation::Get()->CreateAnchor(FString("Engine/Animation/Skeleton"))
		]
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SNew(STextBlock)
		.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10 ) )
		.Text( LOCTEXT("FbxOptionWindow_NeedToSelectSkeletonForAnimation", "You need to select skeleton for the animation.").ToString() )
	];

	NewBox->AddSlot().AutoHeight() .Padding(2)
	[
		SAssignNew(SkeletonPickerComboButton, SComboButton)
		.ContentPadding(1.f)
		.OnGetMenuContent( this, &SFbxOptionWindow::MakeSkeletonPickerMenu )
		.HasDownArrow(true)
		.IsEnabled( !bReimport )
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &SFbxOptionWindow::GetSkeletonDisplay)
		]
	];

	return NewBox;
}

FString SFbxOptionWindow::GetPhysicsAssetDisplay() const
{
	if ( ImportUI->PhysicsAsset )
	{
		return ImportUI->PhysicsAsset->GetName();
	}

	return LOCTEXT("NoPhysicsAssetToDisplay", "None").ToString();
}

FString SFbxOptionWindow::GetSkeletonDisplay() const
{
	if ( ImportUI->Skeleton )
	{
		return ImportUI->Skeleton->GetName();
	}

	return LOCTEXT("NoSkeletonToDisplay", "None").ToString();
}

void SFbxOptionWindow::SetImportType(EFBXImportType ImportType)
{
	ImportUI->MeshTypeToImport = ImportType;

	RefreshWindow();
}

// data set functions
void SFbxOptionWindow::SetGeneral_OverrideFullName(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bOverrideFullName = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ImportMeshLODs(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bImportMeshLODs = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ImportMorphTargets(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bImportMorphTargets = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_UpdateSkeletonRefPose(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bUpdateSkeletonReferencePose = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ImportAnimation(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bImportAnimations = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ImportRigidMesh(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bImportRigidMesh = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_UseDefaultSampleRate(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bUseDefaultSampleRate = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_UseT0AsRefPose(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bUseT0AsRefPose = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ReserveSmoothingGroups(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bPreserveSmoothingGroups = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_KeepOverlappingVertices(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bKeepOverlappingVertices = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_ImportMeshesInBoneHierarchy(ESlateCheckBoxState::Type NewType)
{
	ImportUI->SkeletalMeshImportData->bImportMeshesInBoneHierarchy = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_CreatePhysicsAsset(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bCreatePhysicsAsset = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetStaticMesh_ImportMeshLODs(ESlateCheckBoxState::Type NewType)
{
	ImportUI->StaticMeshImportData->bImportMeshLODs = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetStaticMesh_LODGroup(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	int32 GroupIndex = StaticMeshLODGroups.Find(NewValue);
	ImportUI->StaticMeshImportData->StaticMeshLODGroup = StaticMeshLODGroupNames[GroupIndex];
}

void SFbxOptionWindow::SetStaticMesh_CombineMeshes(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bCombineMeshes = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetStaticMesh_ReplaceVertexColor(ESlateCheckBoxState::Type NewType)
{
	ImportUI->StaticMeshImportData->bReplaceVertexColors = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetStaticMesh_RemoveDegenerates(ESlateCheckBoxState::Type NewType)
{
	ImportUI->StaticMeshImportData->bRemoveDegenerates = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetStaticMesh_OneConvexHullPerUCX(ESlateCheckBoxState::Type NewType)
{
	ImportUI->StaticMeshImportData->bOneConvexHullPerUCX = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetMaterial_ImportMaterials(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bImportMaterials = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetMaterial_ImportTextures(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bImportTextures = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetMaterial_InvertNormalMaps(ESlateCheckBoxState::Type NewType)
{
	ImportUI->TextureImportData->bInvertNormalMaps = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

void SFbxOptionWindow::SetSkeletalMesh_AnimationName(const FText& Name, ETextCommit::Type /*CommitInfo*/ )
{
	ImportUI->AnimationName = Name.ToString();
}

void SFbxOptionWindow::SetAnimLengthOption(EFBXAnimationLengthImportType AnimLengthOption)
{
	ImportUI->AnimSequenceImportData->AnimationLength = AnimLengthOption;
}

void SFbxOptionWindow::SetAnimationRangeStart(const FText& Name, ETextCommit::Type /*CommitInfo*/ )
{
	ImportUI->AnimSequenceImportData->StartFrame = FCString::Atoi( *Name.ToString() );

	if (ImportUI->AnimSequenceImportData->StartFrame >= ImportUI->AnimSequenceImportData->EndFrame)
	{
		ErrorMessage = LOCTEXT("FbxOptionWindow_InvalidStartFrame", "Invalid StartFrame").ToString();
	}
	else
	{
		// @fixme
		// this error message doesn't work well globally, -i.e. if the message wasn't for this, but for something else
		// we need better error feed back message thing here
		ErrorMessage = TEXT("");
	}
}

void SFbxOptionWindow::SetAnimationRangeEnd(const FText& Name, ETextCommit::Type /*CommitInfo*/)
{
	ImportUI->AnimSequenceImportData->EndFrame = FCString::Atoi(*Name.ToString());

	if (ImportUI->AnimSequenceImportData->StartFrame >= ImportUI->AnimSequenceImportData->EndFrame)
	{
		ErrorMessage = LOCTEXT("FbxOptionWindow_InvalidEndFrame", "Invalid EndFrame").ToString();
	}
	else
	{
		// @fixme
		// this error message doesn't work well globally, -i.e. if the message wasn't for this, but for something else
		// we need better error feed back message thing here
		ErrorMessage = TEXT("");
	}
}

void SFbxOptionWindow::SetAnimation_ReserveLocalTransform(ESlateCheckBoxState::Type NewType)
{
	ImportUI->bPreserveLocalTransform = (NewType == ESlateCheckBoxState::Checked)? 1:0;
}

TSharedRef<SWidget> SFbxOptionWindow::MakeSkeletonPickerMenu()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(USkeleton::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SFbxOptionWindow::OnAssetSelectedFromSkeletonPicker);
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.ThumbnailScale = 0.0f;

	return SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];
}

void SFbxOptionWindow::OnAssetSelectedFromSkeletonPicker(const FAssetData& AssetData)
{
	// @todo Set the content reference
	if (SkeletonPickerComboButton.IsValid())
	{
		SkeletonPickerComboButton->SetIsOpen(false);
		ImportUI->Skeleton = Cast<USkeleton>(AssetData.GetAsset());
	}
}

TSharedRef<SWidget> SFbxOptionWindow::MakePhysicsAssetPickerMenu()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UPhysicsAsset::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SFbxOptionWindow::OnAssetSelectedFromPhysicsAssetPicker);
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.ThumbnailScale = 0.0f;

	return SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];
}

void SFbxOptionWindow::OnAssetSelectedFromPhysicsAssetPicker(const FAssetData& AssetData)
{
	// @todo Set the content reference
	if (PhysicsAssetPickerComboButton.IsValid())
	{
		PhysicsAssetPickerComboButton->SetIsOpen(false);
		ImportUI->PhysicsAsset = Cast<UPhysicsAsset>(AssetData.GetAsset());
	}
}

TSharedRef<SWidget> SFbxOptionWindow::OnGenerateWidgetForComboItem( TSharedPtr<EFBXNormalImportMethod> ImportMethod )
{
	return SNew( STextBlock )
	.Text( OnGenerateTextForImportMethod( *ImportMethod ) )
	.ToolTipText( OnGenerateToolTipForImportMethod( *ImportMethod ) );
}

FString SFbxOptionWindow::OnGenerateTextForImportMethod( EFBXNormalImportMethod ImportMethod ) const
{
	if( ImportMethod == FBXNIM_MAX )
	{
		ImportMethod = GetCurrentNormalImportMethod();
	}

	if( ImportMethod == FBXNIM_ComputeNormals ) 
	{	
		return LOCTEXT("FBXOptions_CalculateNormals", "Calculate Normals").ToString();
	}
	else if( ImportMethod == FBXNIM_ImportNormals )
	{
		return LOCTEXT("FBXOptions_ImportNormals", "Import Normals").ToString();
	}
	else
	{
		return LOCTEXT("FBXOptions_ImportNormalsAndTangents", "Import Normals and Tangents").ToString();
	}
}

FString SFbxOptionWindow::OnGenerateToolTipForImportMethod( EFBXNormalImportMethod ImportMethod ) const
{
	if( ImportMethod == FBXNIM_ComputeNormals ) 
	{	
		return LOCTEXT("FBXOptions_CalculateNormalsToolTip", "Let Unreal calculate normals and tangents. Ignores normals in the fbx file").ToString();
	}
	else if( ImportMethod == FBXNIM_ImportNormals )
	{
		return LOCTEXT("FBXOptions_ImportNormalsToolTip", "Import normals found in the fbx file.  Tangents are calculated by Unreal").ToString();
	}
	else
	{
		return LOCTEXT("FBXOptions_ImportNormalsAndTangentsToolTip", "Import Normals and Tangents found in the fbx file instead of computing them").ToString();
	}
}

void SFbxOptionWindow::OnNormalImportMethodChanged( TSharedPtr<EFBXNormalImportMethod> NewMethod, ESelectInfo::Type SelectionType )
{
	if ( ImportUI->MeshTypeToImport == FBXIT_StaticMesh )
	{
		ImportUI->StaticMeshImportData->NormalImportMethod = *NewMethod;
	}
	else if ( ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh )
	{
		ImportUI->SkeletalMeshImportData->NormalImportMethod = *NewMethod;
	}
	else
	{
		// Invalid mode
	}
}

bool SFbxOptionWindow::CanImport()  const
{
	// do test to see if we are ready to import

	if (ImportUI->MeshTypeToImport == FBXIT_Animation)
	{
		if (ImportUI->Skeleton == NULL)
		{
			return false;
		}
	}

	if (ImportUI->AnimSequenceImportData->AnimationLength == FBXALIT_SetRange)
	{
		if (ImportUI->AnimSequenceImportData->StartFrame > ImportUI->AnimSequenceImportData->EndFrame)
		{
			return false;
		}
	}

	return true;
}

bool SFbxOptionWindow::ShouldShowPhysicsAssetPicker() const
{
	return (!bReimport && ImportUI->bCreatePhysicsAsset == false);
}

EFBXNormalImportMethod SFbxOptionWindow::GetCurrentNormalImportMethod() const
{
	if ( ImportUI->MeshTypeToImport == FBXIT_StaticMesh )
	{
		return ImportUI->StaticMeshImportData->NormalImportMethod;
	}
	else if ( ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh )
	{
		return ImportUI->SkeletalMeshImportData->NormalImportMethod;
	}
	else
	{
		return FBXNIM_ComputeNormals;
	}
}

#undef LOCTEXT_NAMESPACE

