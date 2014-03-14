// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StaticMeshEditorModule.h"
#include "StaticMeshEditorTools.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "TargetPlatform.h"
#include "StaticMeshResources.h"
#include "StaticMeshEditor.h"
#include "PropertyCustomizationHelpers.h"
#include "PhysicsEngine/BodySetup.h"
#include "FbxMeshUtils.h"

const int32 DefaultHullCount = 4;
const int32 DefaultVertsPerHull = 12;
const int32 MaxHullCount = 24;
const int32 MinHullCount = 1;
const int32 MaxVertsPerHullCount = 32;
const int32 MinVertsPerHullCount = 6;

#define LOCTEXT_NAMESPACE "StaticMeshEditor"
DEFINE_LOG_CATEGORY_STATIC(LogStaticMeshEditorTools,Log,All);

FStaticMeshDetails::FStaticMeshDetails( class FStaticMeshEditor& InStaticMeshEditor )
	: StaticMeshEditor( InStaticMeshEditor )
{}

FStaticMeshDetails::~FStaticMeshDetails()
{
}

void FStaticMeshDetails::CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& LODSettingsCategory = DetailBuilder.EditCategory( "LodSettings", LOCTEXT("LodSettingsCategory", "LOD Settings").ToString() );
	IDetailCategoryBuilder& StaticMeshCategory = DetailBuilder.EditCategory( "StaticMesh", LOCTEXT("StaticMeshGeneralSettings", "Static Mesh Settings").ToString() );
	DetailBuilder.EditCategory( "Navigation", TEXT(""), ECategoryPriority::Uncommon );

	LevelOfDetailSettings = MakeShareable( new FLevelOfDetailSettingsLayout( StaticMeshEditor ) );

	LevelOfDetailSettings->AddToDetailsPanel( DetailBuilder );

	
	TSharedRef<IPropertyHandle> BodyProp = DetailBuilder.GetProperty("BodySetup");
	BodyProp->MarkHiddenByCustomization();

	static TArray<FName> HiddenBodyInstanceProps;

	if( HiddenBodyInstanceProps.Num() == 0 )
	{
		HiddenBodyInstanceProps.Add("DefaultInstance");
		HiddenBodyInstanceProps.Add("BoneName");
		HiddenBodyInstanceProps.Add("PhysicsType");
		HiddenBodyInstanceProps.Add("bConsiderForBounds");
		HiddenBodyInstanceProps.Add("CollisionReponse");
	}

	uint32 NumChildren = 0;
	BodyProp->GetNumChildren( NumChildren );

	TSharedPtr<IPropertyHandle> BodyPropObject;

	if( NumChildren == 1 )
	{
		// This is an edit inline new property so the first child is the object instance for the edit inline new.  The instance contains the child we want to display
		BodyPropObject = BodyProp->GetChildHandle( 0 );

		NumChildren = 0;
		BodyPropObject->GetNumChildren( NumChildren );

		for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
		{
			TSharedPtr<IPropertyHandle> ChildProp = BodyPropObject->GetChildHandle(ChildIndex);
			if( ChildProp.IsValid() && ChildProp->GetProperty() && !HiddenBodyInstanceProps.Contains(ChildProp->GetProperty()->GetFName()) )
			{
				StaticMeshCategory.AddProperty( ChildProp );
			}
		}
	}
}

void SConvexDecomposition::Construct(const FArguments& InArgs)
{
	StaticMeshEditorPtr = InArgs._StaticMeshEditorPtr;

	CurrentMaxHullCount = DefaultHullCount;
	CurrentMaxVertsPerHullCount = DefaultVertsPerHull;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 16.0f, 0.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text( LOCTEXT("MaxNumHulls_ConvexDecomp", "Max Hulls").ToString() )
			]

			+SHorizontalBox::Slot()
			.FillWidth(3.0f)
			[
				SAssignNew(MaxHull, SSpinBox<int32>)
				.MinValue(MinHullCount)
				.MaxValue(MaxHullCount)
				.Value( this, &SConvexDecomposition::GetHullCount )
				.OnValueCommitted( this, &SConvexDecomposition::OnHullCountCommitted )
				.OnValueChanged( this, &SConvexDecomposition::OnHullCountChanged )
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 8.0f, 0.0f, 16.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text( LOCTEXT("MaxHullVerts_ConvexDecomp", "Max Hull Verts").ToString() )
			]

			+SHorizontalBox::Slot()
				.FillWidth(3.0f)
				[
					SAssignNew(MaxVertsPerHull, SSpinBox<int32>)
					.MinValue(MinVertsPerHullCount)
					.MaxValue(MaxVertsPerHullCount)
					.Value( this, &SConvexDecomposition::GetVertsPerHullCount )
					.OnValueCommitted( this, &SConvexDecomposition::OnVertsPerHullCountCommitted )
					.OnValueChanged( this, &SConvexDecomposition::OnVertsPerHullCountChanged )
				]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SButton)
				.Text( LOCTEXT("Apply_ConvexDecomp", "Apply").ToString() )
				.OnClicked(this, &SConvexDecomposition::OnApplyDecomp)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SButton)
				.Text( LOCTEXT("Defaults_ConvexDecomp", "Defaults").ToString() )
				.OnClicked(this, &SConvexDecomposition::OnDefaults)
			]
		]
	];
}

bool FStaticMeshDetails::IsApplyNeeded() const
{
	return LevelOfDetailSettings.IsValid() && LevelOfDetailSettings->IsApplyNeeded();
}

void FStaticMeshDetails::ApplyChanges()
{
	if( LevelOfDetailSettings.IsValid() )
	{
		LevelOfDetailSettings->ApplyChanges();
	}
}

SConvexDecomposition::~SConvexDecomposition()
{

}

FReply SConvexDecomposition::OnApplyDecomp()
{
	StaticMeshEditorPtr.Pin()->DoDecomp(CurrentMaxHullCount, CurrentMaxVertsPerHullCount);

	return FReply::Handled();
}

FReply SConvexDecomposition::OnDefaults()
{
	CurrentMaxHullCount = DefaultHullCount;
	CurrentMaxVertsPerHullCount = DefaultVertsPerHull;

	return FReply::Handled();
}

void SConvexDecomposition::OnHullCountCommitted(int32 InNewValue, ETextCommit::Type CommitInfo)
{
	OnHullCountChanged(InNewValue);
}

void SConvexDecomposition::OnHullCountChanged(int32 InNewValue)
{
	CurrentMaxHullCount = InNewValue;
}

int32 SConvexDecomposition::GetHullCount() const
{
	return CurrentMaxHullCount;
}
void SConvexDecomposition::OnVertsPerHullCountCommitted(int32 InNewValue,  ETextCommit::Type CommitInfo)
{
	OnVertsPerHullCountChanged(InNewValue);
}

void SConvexDecomposition::OnVertsPerHullCountChanged(int32 InNewValue)
{
	CurrentMaxVertsPerHullCount = InNewValue;
}

int32 SConvexDecomposition::GetVertsPerHullCount() const
{
	return CurrentMaxVertsPerHullCount;
}

static UEnum& GetFeatureImportanceEnum()
{
	static FName FeatureImportanceName(TEXT("EMeshFeatureImportance::Off"));
	static UEnum* FeatureImportanceEnum = NULL;
	if (FeatureImportanceEnum == NULL)
	{
		UEnum::LookupEnumName(FeatureImportanceName, &FeatureImportanceEnum);
		check(FeatureImportanceEnum);
	}
	return *FeatureImportanceEnum;
}

static void FillEnumOptions(TArray<TSharedPtr<FString> >& OutStrings, UEnum& InEnum)
{
	for (int32 EnumIndex = 0; EnumIndex < InEnum.NumEnums() - 1; ++EnumIndex)
	{
		OutStrings.Add(MakeShareable(new FString(InEnum.GetEnumName(EnumIndex))));
	}
}

FMeshBuildSettingsLayout::FMeshBuildSettingsLayout( TSharedRef<FLevelOfDetailSettingsLayout> InParentLODSettings )
	: ParentLODSettings( InParentLODSettings )
{

}

FMeshBuildSettingsLayout::~FMeshBuildSettingsLayout()
{
}

void FMeshBuildSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	NodeRow.NameContent()
	[
		SNew( STextBlock )
		.Text( LOCTEXT("MeshBuildSettings", "Build Settings").ToString() )
		.Font( IDetailLayoutBuilder::GetDetailFont() )
	];
}

void FMeshBuildSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	{
		ChildrenBuilder.AddChildContent( LOCTEXT("RecomputeNormals", "Recompute Normals").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.Text(LOCTEXT("RecomputeNormals", "Recompute Normals").ToString())
		
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FMeshBuildSettingsLayout::ShouldRecomputeNormals)
			.OnCheckStateChanged(this, &FMeshBuildSettingsLayout::OnRecomputeNormalsChanged)
		];
	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("RecomputeTangents", "Recompute Tangents").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("RecomputeTangents", "Recompute Tangents").ToString())
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FMeshBuildSettingsLayout::ShouldRecomputeTangents)
			.OnCheckStateChanged(this, &FMeshBuildSettingsLayout::OnRecomputeTangentsChanged)
		];
	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("RemoveDegenerates", "Remove Degenerates").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("RemoveDegenerates", "Remove Degenerates").ToString())
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FMeshBuildSettingsLayout::ShouldRemoveDegenerates)
			.OnCheckStateChanged(this, &FMeshBuildSettingsLayout::OnRemoveDegeneratesChanged)
		];
	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("UseFullPrecisionUVs", "Use Full Precision UVs").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("UseFullPrecisionUVs", "Use Full Precision UVs").ToString())
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FMeshBuildSettingsLayout::ShouldUseFullPrecisionUVs)
			.OnCheckStateChanged(this, &FMeshBuildSettingsLayout::OnUseFullPrecisionUVsChanged)
		];
	}

	{
		ChildrenBuilder.AddChildContent(LOCTEXT("BuildScale", "Build Scale").ToString())
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("BuildScale", "Build Scale").ToString())
			]
		.ValueContent()
			[
				SNew(SNumericEntryBox<float>)
				.AllowSpin(true)
				.ToolTipText( LOCTEXT("BuildScale_ToolTip", "The local scale applied when building the mesh") )
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.MinValue(0.0f)
				.MaxValue(TOptional<float>())
				.MaxSliderValue(TOptional<float>())
				.MinSliderValue(TOptional<float>())
				.Value( this, &FMeshBuildSettingsLayout::GetBuildScale)
				.OnValueChanged(this, &FMeshBuildSettingsLayout::OnBuildScaleChanged)
			];
	}

		
	{
		ChildrenBuilder.AddChildContent( LOCTEXT("ApplyChanges", "Apply Changes").ToString() )
			.ValueContent()
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.OnClicked(this, &FMeshBuildSettingsLayout::OnApplyChanges)
				.IsEnabled(ParentLODSettings.Pin().ToSharedRef(), &FLevelOfDetailSettingsLayout::IsApplyNeeded)
				[
					SNew( STextBlock )
					.Text(LOCTEXT("ApplyChanges", "Apply Changes").ToString())
					.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
			];
	}
}

void FMeshBuildSettingsLayout::UpdateSettings(const FMeshBuildSettings& InSettings)
{
	BuildSettings = InSettings;
}

FReply FMeshBuildSettingsLayout::OnApplyChanges()
{
	if( ParentLODSettings.IsValid() )
	{
		ParentLODSettings.Pin()->ApplyChanges();
	}
	return FReply::Handled();
}

ESlateCheckBoxState::Type FMeshBuildSettingsLayout::ShouldRecomputeNormals() const
{
	return BuildSettings.bRecomputeNormals ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

ESlateCheckBoxState::Type FMeshBuildSettingsLayout::ShouldRecomputeTangents() const
{
	return BuildSettings.bRecomputeTangents ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

ESlateCheckBoxState::Type FMeshBuildSettingsLayout::ShouldRemoveDegenerates() const
{
	return BuildSettings.bRemoveDegenerates ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

ESlateCheckBoxState::Type FMeshBuildSettingsLayout::ShouldUseFullPrecisionUVs() const
{
	return BuildSettings.bUseFullPrecisionUVs ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

TOptional<float> FMeshBuildSettingsLayout::GetBuildScale() const
{
	return BuildSettings.BuildScale;
}

void FMeshBuildSettingsLayout::OnRecomputeNormalsChanged(ESlateCheckBoxState::Type NewState)
{
	BuildSettings.bRecomputeNormals = (NewState == ESlateCheckBoxState::Checked) ? true : false;
}

void FMeshBuildSettingsLayout::OnRecomputeTangentsChanged(ESlateCheckBoxState::Type NewState)
{
	BuildSettings.bRecomputeTangents = (NewState == ESlateCheckBoxState::Checked) ? true : false;
}

void FMeshBuildSettingsLayout::OnRemoveDegeneratesChanged(ESlateCheckBoxState::Type NewState)
{
	BuildSettings.bRemoveDegenerates = (NewState == ESlateCheckBoxState::Checked) ? true : false;
}

void FMeshBuildSettingsLayout::OnUseFullPrecisionUVsChanged(ESlateCheckBoxState::Type NewState)
{
	BuildSettings.bUseFullPrecisionUVs = (NewState == ESlateCheckBoxState::Checked) ? true : false;
}

void FMeshBuildSettingsLayout::OnBuildScaleChanged( float NewScale )
{
	BuildSettings.BuildScale = NewScale;
}

FMeshReductionSettingsLayout::FMeshReductionSettingsLayout( TSharedRef<FLevelOfDetailSettingsLayout> InParentLODSettings )
	: ParentLODSettings( InParentLODSettings )
{
	FillEnumOptions(ImportanceOptions, GetFeatureImportanceEnum());
}

FMeshReductionSettingsLayout::~FMeshReductionSettingsLayout()
{
}

void FMeshReductionSettingsLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow  )
{
	NodeRow.NameContent()
	[
		SNew( STextBlock )
		.Text( LOCTEXT("MeshReductionSettings", "Reduction Settings").ToString() )
		.Font( IDetailLayoutBuilder::GetDetailFont() )
	];
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FMeshReductionSettingsLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	{
		ChildrenBuilder.AddChildContent( LOCTEXT("PercentTriangles", "Percent Triangles").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("PercentTriangles", "Percent Triangles").ToString())
		]
		.ValueContent()
		[
			SNew(SSpinBox<float>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0.0f)
			.MaxValue(100.0f)
			.Value(this, &FMeshReductionSettingsLayout::GetPercentTriangles)
			.OnValueChanged(this, &FMeshReductionSettingsLayout::OnPercentTrianglesChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("MaxDeviation", "Max Deviation").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("MaxDeviation", "Max Deviation").ToString())
		]
		.ValueContent()
		[
			SNew(SSpinBox<float>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0.0f)
			.MaxValue(1000.0f)
			.Value(this, &FMeshReductionSettingsLayout::GetMaxDeviation)
			.OnValueChanged(this, &FMeshReductionSettingsLayout::OnMaxDeviationChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("Silhouette_MeshSimplification", "Silhouette").ToString() )
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("Silhouette_MeshSimplification", "Silhouette").ToString() )
		]
		.ValueContent()
		[
			SAssignNew(SilhouetteCombo, STextComboBox)
			//.Font( IDetailLayoutBuilder::GetDetailFont() )
			.ContentPadding(0)
			.OptionsSource(&ImportanceOptions)
			.InitiallySelectedItem(ImportanceOptions[ReductionSettings.SilhouetteImportance])
			.OnSelectionChanged(this, &FMeshReductionSettingsLayout::OnSilhouetteImportanceChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("Texture_MeshSimplification", "Texture").ToString() )
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("Texture_MeshSimplification", "Texture").ToString() )
		]
		.ValueContent()
		[
			SAssignNew( TextureCombo, STextComboBox )
			//.Font( IDetailLayoutBuilder::GetDetailFont() )
			.ContentPadding(0)
			.OptionsSource( &ImportanceOptions )
			.InitiallySelectedItem(ImportanceOptions[ReductionSettings.TextureImportance])
			.OnSelectionChanged(this, &FMeshReductionSettingsLayout::OnTextureImportanceChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("Shading_MeshSimplification", "Shading").ToString() )
		.NameContent()
		[
			SNew( STextBlock )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text( LOCTEXT("Shading_MeshSimplification", "Shading").ToString() )
		]
		.ValueContent()
		[
			SAssignNew( ShadingCombo, STextComboBox )
			//.Font( IDetailLayoutBuilder::GetDetailFont() )
			.ContentPadding(0)
			.OptionsSource( &ImportanceOptions )
			.InitiallySelectedItem(ImportanceOptions[ReductionSettings.ShadingImportance])
			.OnSelectionChanged(this, &FMeshReductionSettingsLayout::OnShadingImportanceChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("WeldingThreshold", "Welding Threshold").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("WeldingThreshold", "Welding Threshold").ToString())
		]
		.ValueContent()
		[
			SNew(SSpinBox<float>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0.0f)
			.MaxValue(10.0f)
			.Value(this, &FMeshReductionSettingsLayout::GetWeldingThreshold)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("RecomputeNormals", "Recompute Normals").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("RecomputeNormals", "Recompute Normals").ToString())

		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FMeshReductionSettingsLayout::ShouldRecalculateNormals)
			.OnCheckStateChanged(this, &FMeshReductionSettingsLayout::OnRecalculateNormalsChanged)
		];
	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("HardEdgeAngle", "Hard Edge Angle").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("HardEdgeAngle", "Hard Edge Angle").ToString())
		]
		.ValueContent()
		[
			SNew(SSpinBox<float>)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.MinValue(0.0f)
			.MaxValue(180.0f)
			.Value(this, &FMeshReductionSettingsLayout::GetHardAngleThreshold)
			.OnValueChanged(this, &FMeshReductionSettingsLayout::OnHardAngleThresholdChanged)
		];

	}

	{
		ChildrenBuilder.AddChildContent( LOCTEXT("ApplyChanges", "Apply Changes").ToString() )
			.ValueContent()
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.OnClicked(this, &FMeshReductionSettingsLayout::OnApplyChanges)
				.IsEnabled(ParentLODSettings.Pin().ToSharedRef(), &FLevelOfDetailSettingsLayout::IsApplyNeeded)
				[
					SNew( STextBlock )
					.Text(LOCTEXT("ApplyChanges", "Apply Changes").ToString())
					.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
			];
	}

	SilhouetteCombo->SetSelectedItem(ImportanceOptions[ReductionSettings.SilhouetteImportance]);
	TextureCombo->SetSelectedItem(ImportanceOptions[ReductionSettings.TextureImportance]);
	ShadingCombo->SetSelectedItem(ImportanceOptions[ReductionSettings.ShadingImportance]);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

const FMeshReductionSettings& FMeshReductionSettingsLayout::GetSettings() const
{
	return ReductionSettings;
}

void FMeshReductionSettingsLayout::UpdateSettings(const FMeshReductionSettings& InSettings)
{
	ReductionSettings = InSettings;
}

FReply FMeshReductionSettingsLayout::OnApplyChanges()
{
	if( ParentLODSettings.IsValid() )
	{
		ParentLODSettings.Pin()->ApplyChanges();
	}
	return FReply::Handled();
}

float FMeshReductionSettingsLayout::GetPercentTriangles() const
{
	return ReductionSettings.PercentTriangles * 100.0f; // Display fraction as percentage.
}

float FMeshReductionSettingsLayout::GetMaxDeviation() const
{
	return ReductionSettings.MaxDeviation;
}

float FMeshReductionSettingsLayout::GetWeldingThreshold() const
{
	return ReductionSettings.WeldingThreshold;
}

ESlateCheckBoxState::Type FMeshReductionSettingsLayout::ShouldRecalculateNormals() const
{
	return ReductionSettings.bRecalculateNormals ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

float FMeshReductionSettingsLayout::GetHardAngleThreshold() const
{
	return ReductionSettings.HardAngleThreshold;
}

void FMeshReductionSettingsLayout::OnPercentTrianglesChanged(float NewValue)
{
	// Percentage -> fraction.
	ReductionSettings.PercentTriangles = NewValue * 0.01f;
}

void FMeshReductionSettingsLayout::OnMaxDeviationChanged(float NewValue)
{
	ReductionSettings.MaxDeviation = NewValue;
}

void FMeshReductionSettingsLayout::OnWeldingThresholdChanged(float NewValue)
{
	ReductionSettings.WeldingThreshold = NewValue;
}

void FMeshReductionSettingsLayout::OnRecalculateNormalsChanged(ESlateCheckBoxState::Type NewValue)
{
	ReductionSettings.bRecalculateNormals = NewValue == ESlateCheckBoxState::Checked;
}

void FMeshReductionSettingsLayout::OnHardAngleThresholdChanged(float NewValue)
{
	ReductionSettings.HardAngleThreshold = NewValue;
}

void FMeshReductionSettingsLayout::OnSilhouetteImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	ReductionSettings.SilhouetteImportance = (EMeshFeatureImportance::Type)ImportanceOptions.Find(NewValue);
}

void FMeshReductionSettingsLayout::OnTextureImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	ReductionSettings.TextureImportance = (EMeshFeatureImportance::Type)ImportanceOptions.Find(NewValue);
}

void FMeshReductionSettingsLayout::OnShadingImportanceChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	ReductionSettings.ShadingImportance = (EMeshFeatureImportance::Type)ImportanceOptions.Find(NewValue);
}

FMeshSectionSettingsLayout::~FMeshSectionSettingsLayout()
{
}

UStaticMesh& FMeshSectionSettingsLayout::GetStaticMesh() const
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	return *StaticMesh;
}

void FMeshSectionSettingsLayout::AddToCategory( IDetailCategoryBuilder& CategoryBuilder )
{
	FMaterialListDelegates MaterialListDelegates; 
	MaterialListDelegates.OnGetMaterials.BindSP(this, &FMeshSectionSettingsLayout::GetMaterials);
	MaterialListDelegates.OnMaterialChanged.BindSP(this, &FMeshSectionSettingsLayout::OnMaterialChanged);
	MaterialListDelegates.OnGenerateCustomNameWidgets.BindSP( this, &FMeshSectionSettingsLayout::OnGenerateNameWidgetsForMaterial);
	MaterialListDelegates.OnGenerateCustomMaterialWidgets.BindSP(this, &FMeshSectionSettingsLayout::OnGenerateWidgetsForMaterial);
	MaterialListDelegates.OnResetMaterialToDefaultClicked.BindSP(this, &FMeshSectionSettingsLayout::OnResetMaterialToDefaultClicked);

	CategoryBuilder.AddCustomBuilder( MakeShareable( new FMaterialList( CategoryBuilder.GetParentLayout(), MaterialListDelegates ) ) );
}

void FMeshSectionSettingsLayout::GetMaterials(IMaterialListBuilder& ListBuilder)
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	FStaticMeshRenderData* RenderData = StaticMesh.RenderData;
	if (RenderData && RenderData->LODResources.IsValidIndex(LODIndex))
	{
		FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
		int32 NumSections = LOD.Sections.Num();
		for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
		{
			FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SectionIndex);
			UMaterialInterface* SectionMaterial = StaticMesh.GetMaterial(Info.MaterialIndex);
			if (SectionMaterial == NULL)
			{
				SectionMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
			}
			ListBuilder.AddMaterial(SectionIndex, SectionMaterial, /*bCanBeReplaced=*/ true);
		}
	}
}

void FMeshSectionSettingsLayout::OnMaterialChanged(UMaterialInterface* NewMaterial, UMaterialInterface* PrevMaterial, int32 SlotIndex, bool bReplaceAll)
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	check(StaticMesh.RenderData);
	FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SlotIndex);
	if (LODIndex == 0)
	{
		check(Info.MaterialIndex == SlotIndex);
		check(StaticMesh.Materials.IsValidIndex(SlotIndex));
		StaticMesh.Materials[SlotIndex] = NewMaterial;
	}
	else
	{
		int32 NumBaseSections = StaticMesh.RenderData->LODResources[0].Sections.Num();
		if (Info.MaterialIndex < NumBaseSections)
		{
			// The LOD's section was using the same material as in the base LOD (common case).
			Info.MaterialIndex = StaticMesh.Materials.Add(NewMaterial);
			StaticMesh.SectionInfoMap.Set(LODIndex, SlotIndex, Info);
		}
		else
		{
			// The LOD's section was already overriding the base LOD material.
			StaticMesh.Materials[Info.MaterialIndex] = NewMaterial;
		}
	}
	CallPostEditChange();
}

TSharedRef<SWidget> FMeshSectionSettingsLayout::OnGenerateNameWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex)
{
	return
		SNew(SCheckBox)
		.IsChecked(this, &FMeshSectionSettingsLayout::IsSectionSelected, SlotIndex)
		.OnCheckStateChanged(this, &FMeshSectionSettingsLayout::OnSectionSelectedChanged, SlotIndex)
		.ToolTipText(LOCTEXT("Highlight_ToolTip", "Highlights this section in the viewport").ToString())
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.ColorAndOpacity( FLinearColor( 0.4f, 0.4f, 0.4f, 1.0f) )
			.Text(LOCTEXT("Highlight", "Highlight").ToString())

		];
}

TSharedRef<SWidget> FMeshSectionSettingsLayout::OnGenerateWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex)
{
	return SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SCheckBox)
				.IsChecked(this, &FMeshSectionSettingsLayout::DoesSectionCastShadow, SlotIndex)
				.OnCheckStateChanged(this, &FMeshSectionSettingsLayout::OnSectionCastShadowChanged, SlotIndex)
			[
				SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
					.Text(LOCTEXT("CastShadow", "Cast Shadow").ToString())
			]
		]
		+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,2,0,0)
		[
			SNew(SCheckBox)
				.IsChecked(this, &FMeshSectionSettingsLayout::DoesSectionCollide, SlotIndex)
				.OnCheckStateChanged(this, &FMeshSectionSettingsLayout::OnSectionCollisionChanged, SlotIndex)
			[
				SNew(STextBlock)
					.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
					.Text(LOCTEXT("EnableCollision", "Enable Collision").ToString())
			]
		];
}

void FMeshSectionSettingsLayout::OnResetMaterialToDefaultClicked(UMaterialInterface* Material, int32 SlotIndex)
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	if (LODIndex == 0)
	{
		check(StaticMesh.Materials.IsValidIndex(SlotIndex));
		StaticMesh.Materials[SlotIndex] = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	else
	{
		// Reset this LOD's section to use the material in the corresponding section of LOD0.
		StaticMesh.SectionInfoMap.Remove(LODIndex, SlotIndex);
	}
	CallPostEditChange();
}

ESlateCheckBoxState::Type FMeshSectionSettingsLayout::DoesSectionCastShadow(int32 SectionIndex) const
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SectionIndex);
	return Info.bCastShadow ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FMeshSectionSettingsLayout::OnSectionCastShadowChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex)
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SectionIndex);
	Info.bCastShadow = (NewState == ESlateCheckBoxState::Checked) ? true : false;
	StaticMesh.SectionInfoMap.Set(LODIndex, SectionIndex, Info);
	CallPostEditChange();
}

ESlateCheckBoxState::Type FMeshSectionSettingsLayout::DoesSectionCollide(int32 SectionIndex) const
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SectionIndex);
	return Info.bEnableCollision ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FMeshSectionSettingsLayout::OnSectionCollisionChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex)
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	FMeshSectionInfo Info = StaticMesh.SectionInfoMap.Get(LODIndex, SectionIndex);
	Info.bEnableCollision = (NewState == ESlateCheckBoxState::Checked) ? true : false;
	StaticMesh.SectionInfoMap.Set(LODIndex, SectionIndex, Info);
	CallPostEditChange();
}

ESlateCheckBoxState::Type FMeshSectionSettingsLayout::IsSectionSelected(int32 SectionIndex) const
{
	ESlateCheckBoxState::Type State = ESlateCheckBoxState::Unchecked;
	UStaticMeshComponent* Component = StaticMeshEditor.GetStaticMeshComponent();
	if (Component)
	{
		State = Component->SelectedEditorSection == SectionIndex ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return State;
}

void FMeshSectionSettingsLayout::OnSectionSelectedChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex)
{
	UStaticMeshComponent* Component = StaticMeshEditor.GetStaticMeshComponent();
	if (Component)
	{
		if (NewState == ESlateCheckBoxState::Checked)
		{
			Component->SelectedEditorSection = SectionIndex;
		}
		else if (NewState == ESlateCheckBoxState::Unchecked)
		{
			Component->SelectedEditorSection = INDEX_NONE;
		}
		Component->MarkRenderStateDirty();
		StaticMeshEditor.RefreshViewport();
	}
}

void FMeshSectionSettingsLayout::CallPostEditChange()
{
	UStaticMesh& StaticMesh = GetStaticMesh();
	StaticMesh.Modify();
	StaticMesh.PostEditChange();
	if(StaticMesh.BodySetup)
	{
		StaticMesh.BodySetup->CreatePhysicsMeshes();
	}
	StaticMeshEditor.RefreshViewport();
}

/////////////////////////////////
// FLevelOfDetailSettingsLayout
/////////////////////////////////

FLevelOfDetailSettingsLayout::FLevelOfDetailSettingsLayout( FStaticMeshEditor& InStaticMeshEditor )
	: StaticMeshEditor( InStaticMeshEditor )
{
	LODGroupNames.Reset();
	UStaticMesh::GetLODGroups(LODGroupNames);
	for (int32 GroupIndex = 0; GroupIndex < LODGroupNames.Num(); ++GroupIndex)
	{
		LODGroupOptions.Add(MakeShareable(new FString(LODGroupNames[GroupIndex].GetPlainNameString())));
	}

	for (int32 i = 0; i < MAX_STATIC_MESH_LODS; ++i)
	{
		LODDistances[i] = 0.0f;
		bBuildSettingsExpanded[i] = false;
		bReductionSettingsExpanded[i] = false;
		bSectionSettingsExpanded[i] = (i == 0);
	}

	LODCount = StaticMeshEditor.GetStaticMesh()->GetNumLODs();

	UpdateLODNames();
}

/** Returns true if automatic mesh reduction is available. */
static bool IsAutoMeshReductionAvailable()
{
	static bool bAutoMeshReductionAvailable = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities").GetMeshReductionInterface() != NULL;
	return bAutoMeshReductionAvailable;
}

void FLevelOfDetailSettingsLayout::AddToDetailsPanel( IDetailLayoutBuilder& DetailBuilder )
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();

	IDetailCategoryBuilder& LODSettingsCategory =
		DetailBuilder.EditCategory( "LodSettings", LOCTEXT("LodSettingsCategory", "LOD Settings").ToString() );

	

	LODSettingsCategory.AddCustomRow( LOCTEXT("LODGroup", "LOD Group").ToString() )
	.NameContent()
	[
		SNew(STextBlock)
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.Text(LOCTEXT("LODGroup", "LOD Group").ToString())
	]
	.ValueContent()
	[
		SNew(STextComboBox)
		.ContentPadding(0)
		.OptionsSource(&LODGroupOptions)
		.InitiallySelectedItem(LODGroupOptions[LODGroupNames.Find(StaticMesh->LODGroup)])
		.OnSelectionChanged(this, &FLevelOfDetailSettingsLayout::OnLODGroupChanged)
	];
	
	LODSettingsCategory.AddCustomRow( LOCTEXT("LODImport", "LOD Import").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
			.Text(LOCTEXT("LODImport", "LOD Import").ToString())
		]
	.ValueContent()
		[
			SNew(STextComboBox)
			.ContentPadding(0)
			.OptionsSource(&LODNames)
			.InitiallySelectedItem(LODNames[0])
			.OnSelectionChanged(this, &FLevelOfDetailSettingsLayout::OnImportLOD)
		];

	// Add Number of LODs slider.
	const int32 MinAllowedLOD = 1;
	LODSettingsCategory.AddCustomRow( LOCTEXT("NumberOfLODs", "Number of LODs").ToString() )
	.NameContent()
	[
		SNew(STextBlock)
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.Text(LOCTEXT("NumberOfLODs", "Number of LODs").ToString())
	]
	.ValueContent()
	[
		SNew(SSpinBox<int32>)
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.Value(this, &FLevelOfDetailSettingsLayout::GetLODCount)
		.OnValueChanged(this, &FLevelOfDetailSettingsLayout::OnLODCountChanged)
		.OnValueCommitted(this, &FLevelOfDetailSettingsLayout::OnLODCountCommitted)
		.MinValue(MinAllowedLOD)
		.MaxValue(MAX_STATIC_MESH_LODS)
		.ToolTipText(this, &FLevelOfDetailSettingsLayout::GetLODCountTooltip)
		.IsEnabled(IsAutoMeshReductionAvailable())
	];

	// Auto LOD distance check box.
	LODSettingsCategory.AddCustomRow( LOCTEXT("AutoComputeLOD", "Auto Compute LOD Distances").ToString() )
	.NameContent()
	[
		SNew(STextBlock)
		.Font( IDetailLayoutBuilder::GetDetailFont() )
		.Text(LOCTEXT("AutoComputeLOD", "Auto compute LOD Distances").ToString())
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(this, &FLevelOfDetailSettingsLayout::IsAutoLODChecked)
		.OnCheckStateChanged(this, &FLevelOfDetailSettingsLayout::OnAutoLODChanged)
	];

	LODSettingsCategory.AddCustomRow( LOCTEXT("ApplyChanges", "Apply Changes").ToString() )
	.ValueContent()
	.HAlign(HAlign_Left)
	[
		SNew(SButton)
		.OnClicked(this, &FLevelOfDetailSettingsLayout::OnApply)
		.IsEnabled(this, &FLevelOfDetailSettingsLayout::IsApplyNeeded)
		[
			SNew( STextBlock )
			.Text(LOCTEXT("ApplyChanges", "Apply Changes").ToString())
			.Font( DetailBuilder.GetDetailFont() )
		]
	];

	bool bAdvanced = true;
	// Allowed pixel error.
	LODSettingsCategory.AddCustomRow( LOCTEXT("AllowedPixelError", "Auto Distance Error").ToString(), bAdvanced )
	.NameContent()
	[
		SNew(STextBlock)
		.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
		.Text(LOCTEXT("AllowedPixelError", "Auto Distance Error").ToString())
	]
	.ValueContent()
	[
		SNew(SSpinBox<float>)
		.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
		.MinValue(1.0f)
		.MaxValue(100.0f)
		.MinSliderValue(1.0f)
		.MaxSliderValue(5.0f)
		.Value(this, &FLevelOfDetailSettingsLayout::GetPixelError)
		.OnValueChanged(this, &FLevelOfDetailSettingsLayout::OnPixelErrorChanged)
		.IsEnabled(this, &FLevelOfDetailSettingsLayout::IsAutoLODEnabled)
	];

	AddLODLevelCategories( DetailBuilder );
}


void FLevelOfDetailSettingsLayout::AddLODLevelCategories( IDetailLayoutBuilder& DetailBuilder )
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	
	if( StaticMesh )
	{
		const int32 StaticMeshLODCount = StaticMesh->GetNumLODs();
		FStaticMeshRenderData* RenderData = StaticMesh->RenderData;


		// Create information panel for each LOD level.
		for(int32 LODIndex = 0; LODIndex < StaticMeshLODCount; ++LODIndex)
		{
			if (IsAutoMeshReductionAvailable())
			{
				ReductionSettingsWidgets[LODIndex] = MakeShareable( new FMeshReductionSettingsLayout( AsShared() ) );
			}

			if (LODIndex < StaticMesh->SourceModels.Num())
			{
				FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[LODIndex];
				if (ReductionSettingsWidgets[LODIndex].IsValid())
				{
					ReductionSettingsWidgets[LODIndex]->UpdateSettings(SrcModel.ReductionSettings);
				}

				if (SrcModel.RawMeshBulkData->IsEmpty() == false)
				{
					BuildSettingsWidgets[LODIndex] = MakeShareable( new FMeshBuildSettingsLayout( AsShared() ) );
					BuildSettingsWidgets[LODIndex]->UpdateSettings(SrcModel.BuildSettings);
				}

				LODDistances[LODIndex] = SrcModel.LODDistance;
			}
			else if (LODIndex > 0)
			{
				if (ReductionSettingsWidgets[LODIndex].IsValid() && ReductionSettingsWidgets[LODIndex-1].IsValid())
				{
					FMeshReductionSettings ReductionSettings = ReductionSettingsWidgets[LODIndex-1]->GetSettings();
					// By default create LODs with half the triangles of the previous LOD.
					ReductionSettings.PercentTriangles *= 0.5f;
					ReductionSettingsWidgets[LODIndex]->UpdateSettings(ReductionSettings);
				}

				if (LODDistances[LODIndex] <= LODDistances[LODIndex-1])
				{
					const float DefaultLODDistance = 1000.0f;
					LODDistances[LODIndex] = LODDistances[LODIndex-1] + DefaultLODDistance;
				}
			}

			FString CategoryName = FString(TEXT("LOD"));
			CategoryName.AppendInt( LODIndex );

			FString LODLevelString = FText::Format( LOCTEXT("LODLevel", "LOD{0}"), FText::AsNumber( LODIndex ) ).ToString();

			IDetailCategoryBuilder& LODCategory = DetailBuilder.EditCategory( *CategoryName, LODLevelString, ECategoryPriority::Important );

			LODCategory.HeaderContent
			(
				SNew( SBox )
				.HAlign( HAlign_Right )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.Padding( FMargin( 5.0f, 0.0f ) )
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
						.Text(this, &FLevelOfDetailSettingsLayout::GetLODDistanceTitle, LODIndex)
						.Visibility( LODIndex > 0 ? EVisibility::Visible : EVisibility::Collapsed )
					]
					+ SHorizontalBox::Slot()
					.Padding( FMargin( 5.0f, 0.0f ) )
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
						.Text( FText::Format( LOCTEXT("Triangles_MeshSimplification", "Triangles: {0}"), FText::AsNumber( StaticMeshEditor.GetNumTriangles(LODIndex) ) ) )
					]
					+ SHorizontalBox::Slot()
					.Padding( FMargin( 5.0f, 0.0f ) )
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
						.Text( FText::Format( LOCTEXT("Vertices_MeshSimplification", "Vertices: {0}"), FText::AsNumber( StaticMeshEditor.GetNumVertices(LODIndex) ) ) )
					]
				]
			);
			
			
			SectionSettingsWidgets[ LODIndex ] = MakeShareable( new FMeshSectionSettingsLayout( StaticMeshEditor, LODIndex ) );
			SectionSettingsWidgets[ LODIndex ]->AddToCategory( LODCategory );

			if (LODIndex > 0)
			{
				LODCategory.AddCustomRow( LOCTEXT("Distance", "Distance").ToString() )
				.NameContent()
				[
					SNew(STextBlock)
					.Font( IDetailLayoutBuilder::GetDetailFont() )
					.Text(LOCTEXT("Distance", "Distance").ToString())
				]
				.ValueContent()
				[
					SNew(SSpinBox<float>)
					.Font( IDetailLayoutBuilder::GetDetailFont() )
					.MinValue(0.0f)
					.MaxValue(WORLD_MAX)
					.SliderExponent(2.0f)
					.Value(this, &FLevelOfDetailSettingsLayout::GetLODDistance, LODIndex)
					.OnValueChanged(this, &FLevelOfDetailSettingsLayout::OnLODDistanceChanged, LODIndex)
					.OnValueCommitted(this, &FLevelOfDetailSettingsLayout::OnLODDistanceCommitted, LODIndex)
					.IsEnabled(this, &FLevelOfDetailSettingsLayout::CanChangeLODDistance)
				];
			}

			if (BuildSettingsWidgets[LODIndex].IsValid())
			{
				LODCategory.AddCustomBuilder( BuildSettingsWidgets[LODIndex].ToSharedRef() );
			}

			if( ReductionSettingsWidgets[LODIndex].IsValid() )
			{
				LODCategory.AddCustomBuilder( ReductionSettingsWidgets[LODIndex].ToSharedRef() );
			}

		}
	}

}


FLevelOfDetailSettingsLayout::~FLevelOfDetailSettingsLayout()
{
}

int32 FLevelOfDetailSettingsLayout::GetLODCount() const
{
	return LODCount;
}

float FLevelOfDetailSettingsLayout::GetLODDistance(int32 LODIndex) const
{
	check(LODIndex > 0 && LODIndex < MAX_STATIC_MESH_LODS);
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	float Distance = LODDistances[FMath::Clamp<int32>(LODIndex,0,MAX_STATIC_MESH_LODS-1)];
	if (StaticMesh->bAutoComputeLODDistance)
	{
		Distance = StaticMesh->RenderData->LODDistance[LODIndex];
		if (Distance == FLT_MAX)
		{
			Distance = 0.0f;
		}
	}
	else if (StaticMesh->SourceModels.IsValidIndex(LODIndex))
	{
		Distance = StaticMesh->SourceModels[LODIndex].LODDistance;
	}
	return Distance;
}

FText FLevelOfDetailSettingsLayout::GetLODDistanceTitle(int32 LODIndex) const
{
	return FText::Format( LOCTEXT("Distance_MeshSimplification", "Distance: {0}"), FText::AsNumber( LODIndex > 0 ? GetLODDistance( LODIndex ) : 0.0f ) );
}

bool FLevelOfDetailSettingsLayout::CanChangeLODDistance() const
{
	return !IsAutoLODEnabled();
}

void FLevelOfDetailSettingsLayout::OnLODDistanceChanged(float NewValue, int32 LODIndex)
{
	check(LODIndex > 0 && LODIndex < MAX_STATIC_MESH_LODS);
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	if (!StaticMesh->bAutoComputeLODDistance)
	{
		// First propagate any changes from the source models to our local scratch.
		for (int32 i = 0; i < StaticMesh->SourceModels.Num(); ++i)
		{
			LODDistances[i] = StaticMesh->SourceModels[i].LODDistance;
		}

		// Update LOD distances for all further LODs.
		float Delta = NewValue - LODDistances[LODIndex];
		LODDistances[LODIndex] = NewValue;
		for (int32 i = LODIndex + 1; i < MAX_STATIC_MESH_LODS; ++i)
		{
			LODDistances[i] += Delta;
		}

		// Enforce that LODs don't swap in before the previous LOD.
		const float MinLODDistance = 100.0f;
		for (int32 i = 1; i < MAX_STATIC_MESH_LODS; ++i)
		{
			float MinDistance = LODDistances[i-1] + MinLODDistance;
			LODDistances[i] = FMath::Max(LODDistances[i], MinDistance);
		}

		// Push changes immediately.
		for (int32 i = 0; i < MAX_STATIC_MESH_LODS; ++i)
		{
			if (StaticMesh->SourceModels.IsValidIndex(i))
			{
				StaticMesh->SourceModels[i].LODDistance = LODDistances[i];
			}
			if (StaticMesh->RenderData
				&& StaticMesh->RenderData->LODResources.IsValidIndex(i))
			{
				StaticMesh->RenderData->LODDistance[i] = LODDistances[i];
			}
		}

		// Reregister static mesh components using this mesh.
		{
			FStaticMeshComponentRecreateRenderStateContext ReregisterContext(StaticMesh,false);
			StaticMesh->Modify();
		}

		StaticMeshEditor.RefreshViewport();
	}
}

void FLevelOfDetailSettingsLayout::OnLODDistanceCommitted(float NewValue, ETextCommit::Type CommitType, int32 LODIndex)
{
	OnLODDistanceChanged(NewValue, LODIndex);
}

void FLevelOfDetailSettingsLayout::UpdateLODNames()
{
	LODNames.Empty();
	LODNames.Add( MakeShareable( new FString( LOCTEXT("BaseLOD", "Base LOD").ToString() ) ) );
	for(int32 LODLevelID = 1; LODLevelID < LODCount; ++LODLevelID)
	{
		LODNames.Add( MakeShareable( new FString( FText::Format( NSLOCTEXT("LODSettingsLayout", "LODLevel_Reimport", "Reimport LOD Level {0}"), FText::AsNumber( LODLevelID ) ).ToString() ) ) );
	}
	LODNames.Add( MakeShareable( new FString( FText::Format( NSLOCTEXT("LODSettingsLayout", "LODLevel_Import", "Import LOD Level {0}"), FText::AsNumber( LODCount ) ).ToString() ) ) );
}

void FLevelOfDetailSettingsLayout::OnBuildSettingsExpanded(bool bIsExpanded, int32 LODIndex)
{
	check(LODIndex >= 0 && LODIndex < MAX_STATIC_MESH_LODS);
	bBuildSettingsExpanded[LODIndex] = bIsExpanded;
}

void FLevelOfDetailSettingsLayout::OnReductionSettingsExpanded(bool bIsExpanded, int32 LODIndex)
{
	check(LODIndex >= 0 && LODIndex < MAX_STATIC_MESH_LODS);
	bReductionSettingsExpanded[LODIndex] = bIsExpanded;
}

void FLevelOfDetailSettingsLayout::OnSectionSettingsExpanded(bool bIsExpanded, int32 LODIndex)
{
	check(LODIndex >= 0 && LODIndex < MAX_STATIC_MESH_LODS);
	bSectionSettingsExpanded[LODIndex] = bIsExpanded;
}

void FLevelOfDetailSettingsLayout::OnLODGroupChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	int32 GroupIndex = LODGroupOptions.Find(NewValue);
	FName NewGroup = LODGroupNames[GroupIndex];
	if (StaticMesh->LODGroup != NewGroup)
	{
		StaticMesh->Modify();
		StaticMesh->LODGroup = NewGroup;
		EAppReturnType::Type DialogResult = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format( LOCTEXT("ApplyDefaultLODSettings", "Overwrite settings with the defaults from LOD group '{0}'?"), FText::FromString( **NewValue ) )
			);
		if (DialogResult == EAppReturnType::Yes)
		{
			const ITargetPlatform* Platform = GetTargetPlatformManagerRef().GetRunningTargetPlatform();
			check(Platform);
			const FStaticMeshLODGroup& GroupSettings = Platform->GetStaticMeshLODSettings().GetLODGroup(NewGroup);

			// Set the number of LODs to the default.
			LODCount = GroupSettings.GetDefaultNumLODs();
			if (StaticMesh->SourceModels.Num() > LODCount)
			{
				int32 NumToRemove = StaticMesh->SourceModels.Num() - LODCount;
				StaticMesh->SourceModels.RemoveAt(LODCount, NumToRemove);
			}
			while (StaticMesh->SourceModels.Num() < LODCount)
			{
				new(StaticMesh->SourceModels) FStaticMeshSourceModel();
			}
			check(StaticMesh->SourceModels.Num() == LODCount);

			// Set reduction settings to the defaults.
			for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
			{
				StaticMesh->SourceModels[LODIndex].ReductionSettings = GroupSettings.GetDefaultSettings(LODIndex);
			}
			StaticMesh->bAutoComputeLODDistance = true;
			StaticMesh->LightMapResolution = GroupSettings.GetDefaultLightMapResolution();
		}
		StaticMesh->PostEditChange();
		StaticMeshEditor.RefreshTool();
	}
}

bool FLevelOfDetailSettingsLayout::IsAutoLODEnabled() const
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	return StaticMesh->bAutoComputeLODDistance;
}

ESlateCheckBoxState::Type FLevelOfDetailSettingsLayout::IsAutoLODChecked() const
{
	return IsAutoLODEnabled() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FLevelOfDetailSettingsLayout::OnAutoLODChanged(ESlateCheckBoxState::Type NewState)
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	StaticMesh->Modify();
	StaticMesh->bAutoComputeLODDistance = (NewState == ESlateCheckBoxState::Checked) ? true : false;
	if (!StaticMesh->bAutoComputeLODDistance)
	{
		if (StaticMesh->SourceModels.IsValidIndex(0))
		{
			StaticMesh->SourceModels[0].LODDistance = 0.0f;
		}
		for (int32 LODIndex = 1; LODIndex < StaticMesh->SourceModels.Num(); ++LODIndex)
		{
			StaticMesh->SourceModels[LODIndex].LODDistance = StaticMesh->RenderData->LODDistance[LODIndex];
		}
	}
	StaticMesh->PostEditChange();
	StaticMeshEditor.RefreshTool();
}

float FLevelOfDetailSettingsLayout::GetPixelError() const
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	return StaticMesh->AutoLODPixelError;
}

void FLevelOfDetailSettingsLayout::OnPixelErrorChanged(float NewValue)
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);
	{
		FStaticMeshComponentRecreateRenderStateContext ReregisterContext(StaticMesh,false);
		StaticMesh->AutoLODPixelError = NewValue;
		StaticMesh->RenderData->ResolveSectionInfo(StaticMesh);
		StaticMesh->Modify();
	}
	StaticMeshEditor.RefreshViewport();
}

void FLevelOfDetailSettingsLayout::OnImportLOD(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	int32 LODIndex = 0;
	if( LODNames.Find(NewValue, LODIndex) && LODIndex > 0 )
	{
		UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
		check(StaticMesh);
		FbxMeshUtils::ImportMeshLODDialog(StaticMesh,LODIndex);
		StaticMeshEditor.RefreshTool();
	}

}

bool FLevelOfDetailSettingsLayout::IsApplyNeeded() const
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);

	if (StaticMesh->SourceModels.Num() != LODCount)
	{
		return true;
	}

	for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
	{
		FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[LODIndex];
		if (BuildSettingsWidgets[LODIndex].IsValid()
			&& SrcModel.BuildSettings != BuildSettingsWidgets[LODIndex]->GetSettings())
		{
			return true;
		}
		if (ReductionSettingsWidgets[LODIndex].IsValid()
			&& SrcModel.ReductionSettings != ReductionSettingsWidgets[LODIndex]->GetSettings())
		{
			return true;
		}
	}

	return false;
}

void FLevelOfDetailSettingsLayout::ApplyChanges()
{
	UStaticMesh* StaticMesh = StaticMeshEditor.GetStaticMesh();
	check(StaticMesh);

	// Calling Begin and EndSlowTask are rather dangerous because they tick
	// Slate. Call them here and flush rendering commands to be sure!.

	FFormatNamedArguments Args;
	Args.Add( TEXT("StaticMeshName"), FText::FromString( StaticMesh->GetName() ) );
	GWarn->BeginSlowTask( FText::Format( LOCTEXT("ApplyLODChanges", "Applying changes to {StaticMeshName}..."), Args ), true );
	FlushRenderingCommands();

	StaticMesh->Modify();
	if (StaticMesh->SourceModels.Num() > LODCount)
	{
		int32 NumToRemove = StaticMesh->SourceModels.Num() - LODCount;
		StaticMesh->SourceModels.RemoveAt(LODCount, NumToRemove);
	}
	while (StaticMesh->SourceModels.Num() < LODCount)
	{
		new(StaticMesh->SourceModels) FStaticMeshSourceModel();
	}
	check(StaticMesh->SourceModels.Num() == LODCount);

	for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
	{
		FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[LODIndex];
		if (BuildSettingsWidgets[LODIndex].IsValid())
		{
			SrcModel.BuildSettings = BuildSettingsWidgets[LODIndex]->GetSettings();
		}
		if (ReductionSettingsWidgets[LODIndex].IsValid())
		{
			SrcModel.ReductionSettings = ReductionSettingsWidgets[LODIndex]->GetSettings();
		}

		if (LODIndex == 0)
		{
			SrcModel.LODDistance = 0.0f;
		}
		else
		{
			SrcModel.LODDistance = LODDistances[LODIndex];
			FStaticMeshSourceModel& PrevModel = StaticMesh->SourceModels[LODIndex-1];
			if (SrcModel.LODDistance <= PrevModel.LODDistance)
			{
				const float DefaultLODDistance = 1000.0f;
				SrcModel.LODDistance = PrevModel.LODDistance + DefaultLODDistance;
			}
		}
	}
	StaticMesh->PostEditChange();

	GWarn->EndSlowTask();

	StaticMeshEditor.RefreshTool();
}

FReply FLevelOfDetailSettingsLayout::OnApply()
{
	ApplyChanges();
	return FReply::Handled();
}

void FLevelOfDetailSettingsLayout::OnLODCountChanged(int32 NewValue)
{
	LODCount = FMath::Clamp<int32>(NewValue, 1, MAX_STATIC_MESH_LODS);

	UpdateLODNames();
}

void FLevelOfDetailSettingsLayout::OnLODCountCommitted(int32 InValue, ETextCommit::Type CommitInfo)
{
	OnLODCountChanged(InValue);
}

FText FLevelOfDetailSettingsLayout::GetLODCountTooltip() const
{
	if(IsAutoMeshReductionAvailable())
	{
		return LOCTEXT("LODCountTooltip", "The number of LODs for this static mesh. If auto mesh reduction is available, setting this number will determine the number of LOD levels to auto generate.");
	}

	return LOCTEXT("LODCountTooltip_Disabled", "Auto mesh reduction is unavailable! Please provide a mesh reduction interface such as Simplygon to use this feature or manually import LOD levels.");
}

/////////////////////////////////
// SGenerateUniqueUVs
/////////////////////////////////
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGenerateUniqueUVs::Construct(const FArguments& InArgs)
{
	StaticMeshEditorPtr = InArgs._StaticMeshEditorPtr;

	RefreshTool();

	MaxStretching = 0.5f;
	MaxCharts = 100.0f;
	MinSpacingBetweenCharts = 1.0f;

	this->ChildSlot
		[
			SNew(SVerticalBox)
			
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
					.Text( LOCTEXT("CreationMode", "Creation Mode").ToString() )
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SCheckBox)	
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &SGenerateUniqueUVs::IsCreationModeChecked, ECreationModeChoice::CreateNew )
						.OnCheckStateChanged( this, &SGenerateUniqueUVs::OnCreationModeChanged, CreateNew )
						[
							SNew(STextBlock)
								.Text( LOCTEXT("CreateNew", "Create New").ToString() )
						]
				]

				+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SCheckBox)	
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &SGenerateUniqueUVs::IsCreationModeChecked, ECreationModeChoice::UseChannel0 )
						.OnCheckStateChanged( this, &SGenerateUniqueUVs::OnCreationModeChanged, UseChannel0 )
						[
							SNew(STextBlock)
								.Text( LOCTEXT("LayoutUsing0Channel", "Layout using 0 channel").ToString() )
						]
				]
			]
			
			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.0f, 8.0f, 16.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(3.0f)
				[
					SNew(STextBlock)
						.Text( LOCTEXT("UVChannelSaveSelection", "UV channel to save results to:").ToString() )
				]

				+SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.FillWidth(1.0f)
				[
					SAssignNew(UVChannelCombo, STextComboBox)
						.OptionsSource(&UVChannels)
						.InitiallySelectedItem(UVChannels.Num() > 1? UVChannels[1] : UVChannels[0])
				]
				
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
					.Text( LOCTEXT("SelectGenerationMethod", "Select Generation Method").ToString() )
					.IsEnabled(this, &SGenerateUniqueUVs::IsCreateNew)
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 8.0f, 4.0f, 16.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(3.0f)
				[
					SNew(SCheckBox)	
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &SGenerateUniqueUVs::IsLimitModeChecked, ELimitModeChoice::Stretching )
						.OnCheckStateChanged( this, &SGenerateUniqueUVs::OnLimitModeChanged, ELimitModeChoice::Stretching )
						.IsEnabled(this, &SGenerateUniqueUVs::IsCreateNew)
					[
						SNew(STextBlock)
							.Text( LOCTEXT("LimitMaxStretching", "Limit maximum stretching").ToString() )
					]
				]

				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
				[
					SNew(SSpinBox<float>)
						.MinValue(0.0f)
						.MaxValue(1.0f)
						.Value(0.5f)
						.OnValueCommitted( this, &SGenerateUniqueUVs::OnMaxStretchingCommitted )
						.OnValueChanged( this, &SGenerateUniqueUVs::OnMaxStretchingChanged )
						.IsEnabled(this, &SGenerateUniqueUVs::IsStretchingLimit)
				]
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 8.0f, 8.0f, 16.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(3.0f)
				[
					SNew(SCheckBox)	
						.Style(FEditorStyle::Get(), "RadioButton")
						.IsChecked( this, &SGenerateUniqueUVs::IsLimitModeChecked, ELimitModeChoice::Charts )
						.OnCheckStateChanged( this, &SGenerateUniqueUVs::OnLimitModeChanged, ELimitModeChoice::Charts )
						.IsEnabled(this, &SGenerateUniqueUVs::IsCreateNew)
					[
						SNew(STextBlock)
							.Text( LOCTEXT("LimitMaxCharts", "Limit maximum number of charts").ToString() )
					]
				]

				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
				[
					SNew(SSpinBox<float>)
						.MinValue(0.0f)
						.MaxValue(100000.0f)
						.Value( this, &SGenerateUniqueUVs::GetMaxCharts )
						.OnValueCommitted( this, &SGenerateUniqueUVs::OnMaxChartsCommitted )
						.OnValueChanged( this, &SGenerateUniqueUVs::OnMaxChartsChanged )
						.IsEnabled(this, &SGenerateUniqueUVs::IsChartLimit)
				]
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 8.0f, 8.0f, 16.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(3.0f)
				[
					SNew(STextBlock)
						.Text( LOCTEXT("LimitChartSpacing", "Limit spacing between charts:").ToString() )
						.IsEnabled(this, &SGenerateUniqueUVs::IsCreateNew)
				]

				+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
				[
					SNew(SSpinBox<float>)
						.MinValue(0.0f)
						.MaxValue(100.0f)
						.Value(1.0f)
						.OnValueCommitted( this, &SGenerateUniqueUVs::OnMinSpacingCommitted )
						.OnValueChanged( this, &SGenerateUniqueUVs::OnMinSpacingChanged )
						.IsEnabled(this, &SGenerateUniqueUVs::IsCreateNew)
				]
			]

			+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding( 0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
					.Text( LOCTEXT("Apply", "Apply").ToString() )
					.OnClicked( this, &SGenerateUniqueUVs::OnApply )
			]
		];

	CurrentCreationModeChoice = CreateNew;
	CurrentLimitModeChoice = Stretching;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SGenerateUniqueUVs::~SGenerateUniqueUVs()
{

}

ESlateCheckBoxState::Type SGenerateUniqueUVs::IsCreationModeChecked( ECreationModeChoice ButtonId ) const
{
	return (CurrentCreationModeChoice == ButtonId)
		? ESlateCheckBoxState::Checked
		: ESlateCheckBoxState::Unchecked;
}

void SGenerateUniqueUVs::OnCreationModeChanged( ESlateCheckBoxState::Type NewRadioState, ECreationModeChoice RadioThatChanged )
{
	if (NewRadioState == ESlateCheckBoxState::Checked)
	{
		CurrentCreationModeChoice = RadioThatChanged;
	}
}

ESlateCheckBoxState::Type SGenerateUniqueUVs::IsLimitModeChecked( enum ELimitModeChoice ButtonId ) const
{
	return (CurrentLimitModeChoice == ButtonId)
	? ESlateCheckBoxState::Checked
	: ESlateCheckBoxState::Unchecked;
}

void SGenerateUniqueUVs::OnLimitModeChanged( ESlateCheckBoxState::Type NewRadioState, enum ELimitModeChoice RadioThatChanged )
{
	if (NewRadioState == ESlateCheckBoxState::Checked)
	{
		CurrentLimitModeChoice = RadioThatChanged;
	}
}

bool SGenerateUniqueUVs::IsCreateNew() const
{
	return CurrentCreationModeChoice == CreateNew;
}

bool SGenerateUniqueUVs::IsChartLimit() const
{
	return (CurrentLimitModeChoice == Charts) && IsCreateNew();
}

bool SGenerateUniqueUVs::IsStretchingLimit() const
{
	return (CurrentLimitModeChoice == Stretching) && IsCreateNew();
}

FReply SGenerateUniqueUVs::OnApply()
{
	int32 ChosenLODIndex = 0;
	int32 ChosenUVChannel = UVChannels.Find(UVChannelCombo->GetSelectedItem());

	bool bOnlyLayoutUVs = !IsCreateNew();
	UStaticMesh* StaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();

	if (StaticMesh->SourceModels.IsValidIndex(ChosenLODIndex)
		&& !StaticMesh->SourceModels[ChosenLODIndex].RawMeshBulkData->IsEmpty())
	{
		bool bStatus = false;
		uint32 Uint32MaxCharts = (uint32)MaxCharts;
		FText Error;
		GWarn->BeginSlowTask( NSLOCTEXT("UnrealEd", "GenerateUVsProgressText", "Generating unique UVs..."), true );
		{
			// Detach all instances of the static mesh while generating the UVs, then reattach them.
			FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(StaticMeshEditorPtr.Pin()->GetStaticMesh());

			// If the user has selected any edges in the static mesh editor, we'll create an array of chart UV
			// seam edge indices to pass along to the GenerateUVs function.
			TArray< int32 >* FalseEdgeIndicesPtr = NULL;
			TArray< int32 > FalseEdgeIndices;
			TSet< int32 >& SelectedEdgeIndices = StaticMeshEditorPtr.Pin()->GetSelectedEdges();

			if( SelectedEdgeIndices.Num() > 0 &&
				ChosenLODIndex == 0 )		// @todo: Support other LODs than LOD 0 (edge selection in SME needed)
			{
				for( TSet< int32 >::TIterator SelectionIt( SelectedEdgeIndices );
					SelectionIt;
					++SelectionIt )
				{
					const uint32 EdgeIndex = *SelectionIt;

					FalseEdgeIndices.Add( EdgeIndex );
				}

				FalseEdgeIndicesPtr = &FalseEdgeIndices;
			}

			FRawMesh RawMesh;
			StaticMesh->SourceModels[ChosenLODIndex].RawMeshBulkData->LoadRawMesh(RawMesh);

			//call the utility helper with the user supplied parameters
			{
				IMeshUtilities& MeshUtils = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	
				// Don't display "border spacing" option as there's usually no reason to add border padding
				float BorderSpacingPercent = 0;

				bool bUseMaxStretch = IsStretchingLimit();

				if(bOnlyLayoutUVs)
				{
					uint32 LightmapResolution = StaticMesh->LightMapResolution ? StaticMesh->LightMapResolution : 256;
					bStatus = MeshUtils.LayoutUVs(RawMesh, LightmapResolution, ChosenUVChannel, Error);
				}		
				else 
				{
					bStatus = MeshUtils.GenerateUVs(RawMesh, ChosenUVChannel, MinSpacingBetweenCharts, BorderSpacingPercent, bUseMaxStretch, FalseEdgeIndicesPtr, Uint32MaxCharts, MaxStretching, Error);
				}
			}

			if (bStatus)
			{
				StaticMesh->SourceModels[ChosenLODIndex].RawMeshBulkData->SaveRawMesh(RawMesh);
				StaticMesh->Build();
			}
		}
		GWarn->EndSlowTask();

		FText StatusMessage;
		if(bStatus)
		{
			FNumberFormattingOptions NumberOptions;
			NumberOptions.MinimumFractionalDigits = 2;
			NumberOptions.MaximumFractionalDigits = 2;

			FFormatNamedArguments Args;
			Args.Add( TEXT("MaxCharts"), Uint32MaxCharts );
			Args.Add( TEXT("MaxStretching"), FText::AsPercent( MaxStretching, &NumberOptions ) );
			StatusMessage = FText::Format( NSLOCTEXT("GenerateUVsWindow", "GenerateUniqueUVs_UVGenerationSuccessful", "Finished generating UVs; Charts: {MaxCharts}; Largest stretch: {MaxStretching}"), Args );
		}
		else if (!Error.IsEmpty())
		{
			StatusMessage = Error;
		}
		else
		{
			StatusMessage = NSLOCTEXT("GenerateUVsWindow", "GenerateUniqueUVs_UVGenerationFailed", "Mesh UV generation failed.");
		}

		FNotificationInfo NotificationInfo( StatusMessage );
		NotificationInfo.ExpireDuration = 3.0f;

		TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		if ( NotificationItem.IsValid() )
		{
			NotificationItem->SetCompletionState(bStatus ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	StaticMeshEditorPtr.Pin()->RefreshTool();
	RefreshTool();

	return FReply::Handled();
}

void SGenerateUniqueUVs::OnMaxStretchingChanged(float InValue)
{
	MaxStretching = InValue;
}

void SGenerateUniqueUVs::OnMaxStretchingCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnMaxStretchingChanged(InValue);
}

void SGenerateUniqueUVs::OnMaxChartsChanged(float InValue)
{
	MaxCharts = float(int32(InValue));
}

void SGenerateUniqueUVs::OnMaxChartsCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnMaxChartsChanged(InValue);
}

float SGenerateUniqueUVs::GetMaxCharts() const
{
	return MaxCharts;
}

void SGenerateUniqueUVs::OnMinSpacingChanged(float InValue)
{
	MinSpacingBetweenCharts = InValue;
}

void SGenerateUniqueUVs::OnMinSpacingCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnMinSpacingChanged(InValue);
}

void SGenerateUniqueUVs::RefreshUVChannelList()
{
	UStaticMesh* StaticMesh = StaticMeshEditorPtr.Pin()->GetStaticMesh();

	// Fill out the UV channels combo.
	UVChannels.Empty();
	for(int32 UVChannelID = 0; UVChannelID < FMath::Min(StaticMeshEditorPtr.Pin()->GetNumUVChannels() + 1, (int32)MAX_STATIC_TEXCOORDS); ++UVChannelID)
	{
		UVChannels.Add( MakeShareable( new FString( FText::Format( LOCTEXT("UVChannel_ID", "UV Channel {0}"), FText::AsNumber( UVChannelID ) ).ToString() ) ) );
	}

	if(UVChannelCombo.IsValid())
	{
		if(UVChannels.Num() > 1)
		{
			UVChannelCombo->SetSelectedItem(UVChannels[1]);
		}
		else
		{
			UVChannelCombo->SetSelectedItem(UVChannels[0]);
		}
	}
	
}

void SGenerateUniqueUVs::RefreshTool()
{
	RefreshUVChannelList();
}

#undef LOCTEXT_NAMESPACE
