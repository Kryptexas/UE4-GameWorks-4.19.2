// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Slate.h"
#include "LODSettingsWindow.h"
#include "LODUtilities.h"
#include "MeshUtilities.h"
#include "FbxMeshUtils.h"
#include "SAssetSearchBox.h"

#define LOCTEXT_NAMESPACE "SkeletalMeshSimplification"

/** 
* Window for adding and removing LODs. LOD chains can also be generated with Simplygon
*/

class SDlgSkeletalMeshLODSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDlgSkeletalMeshLODSettings) {}

		/** Window in which this widget resides */
		SLATE_ATTRIBUTE(TSharedPtr<SWindow>, ParentWindow)
		
		/** The Update Context this tool is associated with. */
		SLATE_ARGUMENT(FSkeletalMeshUpdateContext* , UpdateContextPtr)

	SLATE_END_ARGS()


	/**
	 * Construct this widget
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs)
	{
		IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
		bCanReduce = MeshUtilities.GetMeshReductionInterface() != NULL;

		UpdateContext = *(InArgs._UpdateContextPtr);
		LODCount = 1;

		// Set this widget as focused, to allow users to hit ESC to cancel.
		ParentWindow = InArgs._ParentWindow.Get();
		ParentWindow->SetWidgetToFocusOnActivate(SharedThis(this));
	
		GenerateCurrentLODsWidget();

		// Constructs the LOD chain generation widget. Without Simplygon this returns SNullWidget
		GenerateLODChainWidget();

		this->ChildSlot
			[
				SNew(SScrollBox)
				+SScrollBox::Slot()
				.Padding(5)
				[
					CurrentLODsWidget.ToSharedRef()
				]

				+SScrollBox::Slot()
				.Padding(5)
				[
					LODChainWidget.ToSharedRef()
				]
			];
	};

	/** Returns the number of LODs that should be generated. */
	int32 GetLODCount() const
	{
		return LODCount;
	}

	/** Returns the Simplygon optimization settings. The number of settings in the array
	*   is determined by the 'Number of LODs' slider.
	*/
	TArray<FSkeletalMeshOptimizationSettings> GetLODSettings() const 
	{
		TArray<FSkeletalMeshOptimizationSettings> Settings;
		for(int32 LODIdx = 0; LODIdx < LODCount; ++LODIdx)
		{
			Settings.Add(LevelOfDetailSettings[LODIdx]->GetLODSettings());
		}
		return Settings;
	}

private:

	/** Creates the Slate Widget for Current LOD section. LOD Import functionality will 
	*	be disabled if FBX support is not present 
	*/
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void GenerateCurrentLODsWidget()
	{
		// Standard paddings
		const FMargin BorderPadding(6, 6);
		const FMargin ButtonPadding(4, 4);
		const FMargin SmallPadding(1);

		if(!CurrentLODsWidget.IsValid())
		{
			SAssignNew(CurrentLODsWidget, SVerticalBox);
		}

		TSharedPtr<SVerticalBox> WidgetContent = SNew(SVerticalBox);
		//Base LOD level information is always displayed. No options for LOD Mesh Import or LOD removal.
		WidgetContent->AddSlot()
			.AutoHeight()
			.Padding(5)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(.2f,.2f,.2f,.2f))
				.BorderImage(FEditorStyle::GetBrush("SkeletalMeshEditor.GroupSection"))
				[
					// Section Header
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(BorderPadding)
					[
						SNew(SHorizontalBox)

						
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("BaseLODLevel", "Base LOD"))
							.Font(FEditorStyle::GetFontStyle("ExpandableArea.TitleFont"))
							.ShadowOffset(FVector2D(1.0f, 1.0f))
						]
					]

					// LOD Statistics (Num Vertices, Num Tris)	
					+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(SmallPadding)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(BorderPadding)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text( FText::Format( LOCTEXT("Triangles_MeshSimplification", "Triangles: {0}"), FText::AsNumber( GetNumTriangles(0) ) ) )
							]

							+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(BorderPadding)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text( FText::Format( LOCTEXT("Vertices_MeshSimplification", "Vertices: {0}"),  FText::AsNumber( GetNumVertices(0) ) ) )
								]
						]
				]
			];

		// Create information panel for each LOD level.
		FSkeletalMeshResource* SkelMeshResource = UpdateContext.SkeletalMesh->GetImportedResource();
		for(int32 LODIdx = 1; LODIdx < SkelMeshResource->LODModels.Num(); ++LODIdx)
		{
			WidgetContent->AddSlot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(.2f,.2f,.2f,.2f))
					.BorderImage(FEditorStyle::GetBrush("SkeletalMeshEditor.GroupSection"))
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1)
						[
							// Section Header
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(BorderPadding)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text( FText::Format( LOCTEXT("LODLevel", "LOD {0}"), FText::AsNumber( LODIdx ) ) )
									.Font(FEditorStyle::GetFontStyle("ExpandableArea.TitleFont"))
									.ShadowOffset(FVector2D(1.0f, 1.0f))
								]
							]

							// LOD Statistics (Num Vertices, Num Tris)	
							+SVerticalBox::Slot()
								.AutoHeight()
								.Padding(SmallPadding)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(BorderPadding)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text( FText::Format( LOCTEXT("Triangles_MeshSimplification", "Triangles: {0}"), FText::AsNumber( GetNumTriangles(LODIdx) ) ) )
									]

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(BorderPadding)
									[
										SNew(STextBlock).Text( FText::Format( LOCTEXT("Vertices_MeshSimplification", "Vertices: {0}"), FText::AsNumber( GetNumVertices(LODIdx) ) ) )
									]
								]

						]

						+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SVerticalBox)

								// Import LOD Button. Only visible when FBX support is present.
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(ButtonPadding)
								[
									SNew(SButton)
									.Text(LOCTEXT("ImportLOD", "Import LOD..."))
									.OnClicked(this, &SDlgSkeletalMeshLODSettings::OnImportLOD, LODIdx)
								]

								// Remove LOD button. Always present.
								+ SVerticalBox::Slot()
								.VAlign(VAlign_Center)
								.FillHeight(1.0f)
								.Padding(ButtonPadding)
								[
									SNew(SButton)
									.Text(LOCTEXT("RemoveLOD", "Remove LOD"))
									.OnClicked(this, &SDlgSkeletalMeshLODSettings::OnRemoveLOD, LODIdx)
								]
							]
					]
				];
		}

		const int32 NextAvailableLODLevel = SkelMeshResource->LODModels.Num();
		CurrentLODsWidget->ClearChildren();
		CurrentLODsWidget->AddSlot()
			.AutoHeight()
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("CurrentLODs", "Current LODs"))
				.BodyContent()
				[
					SNew(SVerticalBox)

					// LOD Level Information
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						WidgetContent.ToSharedRef()
					]

					// Import LOD Button. Only visible when FBX support is present.
					+SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SButton)
							.Text( FText::Format( LOCTEXT("ImportNthLOD", "Import LOD {0}"), FText::AsNumber( NextAvailableLODLevel ) ) )
							.Visibility(this, &SDlgSkeletalMeshLODSettings::GetImportNextLODVisibility)
							.OnClicked(this, &SDlgSkeletalMeshLODSettings::OnImportLOD, NextAvailableLODLevel)
						]
				]
			];

	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/** Creates the Slate widget for the LOD chain generation panel  */
	void GenerateLODChainWidget()
	{
		// If Simplygon support is not present this widget is not be displayed.
		if(!CanReduce())
		{
			LODChainWidget = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNullWidget::NullWidget
			];

			return;
		}

		// Number of LODs slider.
		const uint32 MinAllowedLOD = 1;
		TSharedRef<SVerticalBox> LODChainContent = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NumberOfLODs", "Number of LODs").ToString())
				]

				+ SHorizontalBox::Slot()
					.FillWidth(1.5f)
					.Padding(8.0f , 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Center)
					[

						SNew(SSpinBox<int32>)
						.Value(this, &SDlgSkeletalMeshLODSettings::GetLODCount)
						.OnValueChanged(this, &SDlgSkeletalMeshLODSettings::OnLODCountChanged)
						.OnValueCommitted(this, &SDlgSkeletalMeshLODSettings::OnLODCountCommitted)
						.MinValue(MinAllowedLOD)
						.MaxValue(MAX_MESH_LODS)
					]
			];

		// Create the SSkeletalSimplificationOptions widget (the optimization settings) for each LOD level. A skeletal mesh can have a maximum of three LOD levels beyond the base.
		float DefaultQuality = 100.0f;
		for(int32 LODIdx = 0; LODIdx < MAX_MESH_LODS; ++LODIdx)
		{
			int32 LODLevel = LODIdx + 1;
			DefaultQuality -= 25.0f;

			LevelOfDetailSettings[LODIdx] = SNew(SSkeletalSimplificationOptions)
												.DefaultQuality(DefaultQuality)
												.Skeleton(UpdateContext.SkeletalMesh->Skeleton)
												.DesiredLOD(LODLevel);
			FString AreaTitle = FString::Printf(*LOCTEXT("LODLevel", "LOD %d").ToString(), LODLevel);
			
			LODChainContent->AddSlot()
				.AutoHeight()
				.Padding(5.0)
				[
					SNew(SExpandableArea)
					.BorderBackgroundColor(FLinearColor(.2f,.2f,.2f,.2f))
					.BorderImage(FEditorStyle::GetBrush("SkeletalMeshEditor.GroupSection"))
					.Visibility( this, &SDlgSkeletalMeshLODSettings::GetLODVisibility, LODLevel )
					.AreaTitle(AreaTitle)
					.InitiallyCollapsed(true)
					.Padding(10.0)
					.BodyContent()
					[
						LevelOfDetailSettings[LODIdx].ToSharedRef()
					]
				];
		}

		// Add the 'Generate' Button
		LODChainContent->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(this, &SDlgSkeletalMeshLODSettings::GetGenerateButtonText)
				.OnClicked(this, &SDlgSkeletalMeshLODSettings::OnGenerateButtonClick)
			];

		// Finally, we attach this widget to the parent
		LODChainWidget = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SExpandableArea)
				.AreaTitle(LOCTEXT("GenerateLODChain", "Generate LOD Chain").ToString())
				.InitiallyCollapsed(false)
				.BodyContent()
				[
					LODChainContent
				]
			];
	}

	/** Callback for clicking on the Remove LOD button. */
	FReply OnRemoveLOD(int32 LODLevel)
	{
		FLODUtilities::RemoveLOD(UpdateContext, LODLevel);

		// Update the Custom LODs info panel
		GenerateCurrentLODsWidget();

		return FReply::Handled();
	}

	/** Callback for clicking on the Import or Reimport LOD button */
	FReply OnImportLOD(int32 LODLevel)
	{
		FbxMeshUtils::ImportMeshLODDialog( UpdateContext.SkeletalMesh, LODLevel );

		// Update the Custom LODs info panel
		GenerateCurrentLODsWidget();

		return FReply::Handled();
	}

	/** Called when 'Generate' button is pressed. */
	FReply OnGenerateButtonClick()
	{
		//Simplify Mesh
		TArray<FSkeletalMeshOptimizationSettings> Settings = GetLODSettings();
		FLODUtilities::SimplifySkeletalMesh(UpdateContext, Settings);

		//Update Current LODs information
		GenerateCurrentLODsWidget();

		return FReply::Handled();
	}

	/** Returns text for the 'Generate' Button. */
	FString GetGenerateButtonText() const
	{
		return (LODCount >= 2) ? LOCTEXT("GenerateLODChain", "Generate LOD Chain").ToString() : LOCTEXT("GenerateLOD", "Generate LOD").ToString();
	}

	/** Callback function whenever the LOD count spinbox is changed. */
	void OnLODCountChanged(int32 NewValue)
	{
		LODCount = FMath::Clamp<int32>(NewValue, 0, MAX_MESH_LODS);
	}

	/** Callback function whenever the LOD count spinbox value is committed. */
	void OnLODCountCommitted(int32 InValue, ETextCommit::Type CommitInfo)
	{
		OnLODCountChanged(InValue);
	}

	/** Determines the visibility of the LOD options. */
	EVisibility GetLODVisibility(int32 LODIdx) const
	{
		return (LODCount >= LODIdx) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	/** Queries the visibility of the Import LOD button based on how many LOD levels currently exist */
	EVisibility GetImportNextLODVisibility() const
	{
		// Only show the Import LOD button if there are LOD levels that have not yet been used.
		if(UpdateContext.SkeletalMesh->GetImportedResource()->LODModels.Num() <= MAX_MESH_LODS)
		{
			return EVisibility::All;
		}
		else
		{
			return EVisibility::Collapsed;
		}
	}

	/** Returns the number of vertices in a LOD given the LOD index. */
	int32 GetNumVertices(int32 LODIdx)
	{
		check(LODIdx < UpdateContext.SkeletalMesh->GetImportedResource()->LODModels.Num());
		return UpdateContext.SkeletalMesh->GetImportedResource()->LODModels[LODIdx].NumVertices;
	}

	/** Returns the number of triangles in a LOD given the LOD index. */
	int32 GetNumTriangles(int32 LODIdx)
	{
		check(LODIdx < UpdateContext.SkeletalMesh->GetImportedResource()->LODModels.Num());
		return UpdateContext.SkeletalMesh->GetImportedResource()->LODModels[LODIdx].NumVertices / 3;
	}

	/** Helper function to decide if a tool or menu is available due to the availability of Simplygon. */
	bool CanReduce() const
	{
		return bCanReduce;
	}

private:

	/** Enum specifying the maximum number of additional LOD levels beyond the base LOD. */
	enum EMaxLODCount
	{
		MAX_MESH_LODS = 3
	};

	/** The Update Context for the Skeletal mesh that is currently being modified. */
	FSkeletalMeshUpdateContext UpdateContext;

	/** Pointer to the window which holds this Widget, required for modal control */
	TSharedPtr<SWindow> ParentWindow;

	/** A pointer to the LOD Chain Generation widget */
	TSharedPtr<SVerticalBox> LODChainWidget;

	/** A pointer to the Custom LODs widget */
	TSharedPtr<SVerticalBox> CurrentLODsWidget;

	/** Slate interface for each LOD level. There is a maximum of 3 LODs beyond base. */
	TSharedPtr<SSkeletalSimplificationOptions> LevelOfDetailSettings[MAX_MESH_LODS];

	/** The number of LODs that should be generated. */
	int32 LODCount;

	/** True if a valid mesh reduction interface is available. */
	bool bCanReduce;
};


//////////////////////////////////////////////
//	FDlgSkeletalMeshLODSettings

FDlgSkeletalMeshLODSettings::FDlgSkeletalMeshLODSettings( struct FSkeletalMeshUpdateContext& UpdateContext )
{
	if (FSlateApplication::IsInitialized())
	{
		DialogWindow = SNew(SWindow)
			.Title( LOCTEXT("LevelOfDetailSettings", "Level Of Detail Settings") )
			.SupportsMinimize(false) .SupportsMaximize(false)
			.ClientSize( FVector2D(500, 400) );

		TSharedPtr<SBorder> DialogWrapper = 
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
			[
				SAssignNew(DialogWidget, SDlgSkeletalMeshLODSettings)
				.ParentWindow(DialogWindow)
				.UpdateContextPtr(&UpdateContext)
			];

		DialogWindow->SetContent(DialogWrapper.ToSharedRef());
	}
}

void  FDlgSkeletalMeshLODSettings::ShowModal()
{
	GEditor->EditorAddModalWindow(DialogWindow.ToSharedRef());
}

///////////////////////////////////////////////////
//// SSkeletalSimplificationOptions

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSkeletalSimplificationOptions::Construct(const FArguments& InArgs)
{
	WeldingThreshold = 0.1f;
	HardEdgeAngleThreshold = 60.0f;
	Quality = InArgs._DefaultQuality;
	MaxBonesPerVertex = 4.0f;

	Skeleton = InArgs._Skeleton;
	check (Skeleton);

	LODSettingIndex = INDEX_NONE;

	PopulatePossibleSuggestions(Skeleton);
	LODSettingIndex = InArgs._DesiredLOD-1;

	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("Off_Option", "Off").ToString())));
	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("Lowest_Option", "Lowest").ToString())));
	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("Low_Option", "Low").ToString())));
	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("Normal_Option", "Normal").ToString())));
	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("High_Option", "High").ToString())));
	QualityOptions.Add(MakeShareable(new FString(LOCTEXT("Highest_Option", "Highest").ToString())));

	SimplificationTypeOptions.Add(MakeShareable(new FString(LOCTEXT("NumberOfTriangles", "Number of Triangles").ToString())));
	SimplificationTypeOptions.Add(MakeShareable(new FString(LOCTEXT("MaxDeviation", "Max Deviation").ToString())));

	this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DesiredQuality_MeshSimplification", "Desired Quality").ToString())	
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("QualityReductionMethod", "Simplification Type").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(200.0f)
				[
					SAssignNew(SimplificationTypeCombo, STextComboBox)
					.OptionsSource(&SimplificationTypeOptions)
					.InitiallySelectedItem(SimplificationTypeOptions[1])
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SAssignNew(QualityLabel, STextBlock)
					.Text(*SimplificationTypeOptions[1])
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(100.0f)
					.Value(this, &SSkeletalSimplificationOptions::GetQualityValue)
					.OnValueChanged(this, &SSkeletalSimplificationOptions::OnQualityChanged)
					.OnValueCommitted(this, &SSkeletalSimplificationOptions::OnQualityCommitted)
					.Visibility(this, &SSkeletalSimplificationOptions::IsMaxDeviationVisible)
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(100.0f)
					.Value(this, &SSkeletalSimplificationOptions::GetQualityValue)
					.OnValueChanged(this, &SSkeletalSimplificationOptions::OnQualityChanged)
					.OnValueCommitted(this, &SSkeletalSimplificationOptions::OnQualityCommitted)
					.Visibility(this, &SSkeletalSimplificationOptions::IsNumTrianglesVisible)
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 32.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Silhouette_MeshSimplification", "Silhouette").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SAssignNew(SilhouetteCombo, STextComboBox)
					.OptionsSource(&QualityOptions)
					.InitiallySelectedItem(QualityOptions[3])
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Texture_MeshSimplification", "Texture").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SAssignNew(TextureCombo, STextComboBox)
					.OptionsSource(&QualityOptions)
					.InitiallySelectedItem(QualityOptions[3])
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Shading_MeshSimplification", "Shading").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SAssignNew(ShadingCombo, STextComboBox)
					.OptionsSource(&QualityOptions)
					.InitiallySelectedItem(QualityOptions[3])
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Skinning_MeshSimplification", "Skinning").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SAssignNew(SkinningCombo, STextComboBox)
					.OptionsSource(&QualityOptions)
					.InitiallySelectedItem(QualityOptions[3])
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RepairOptions", "Repair Options").ToString())	
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("WeldingThreshold", "Welding Threshold").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(10.0f)
					.Value(WeldingThreshold)
					.OnValueChanged(this, &SSkeletalSimplificationOptions::OnWeldingThresholdChanged)
					.OnValueCommitted(this, &SSkeletalSimplificationOptions::OnWeldingThresholdCommitted)

				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(32.0f, 8.0f, 0.0f, 0.0f)
			[
				SAssignNew(RecomputeNormalsCheckbox, SCheckBox)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RecomputeNormals", "Recompute Normals").ToString())
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("HardEdgeAngleThreshold", "Hard Edge Angle Threshold").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				[
					SNew(SSpinBox<float>)
					.MinValue(0.0f)
					.MaxValue(10.0f)
					.Value(HardEdgeAngleThreshold)
					.OnValueChanged(this, &SSkeletalSimplificationOptions::OnHardEdgeAngleThresholdChanged)
					.OnValueCommitted(this, &SSkeletalSimplificationOptions::OnHardEdgeAngleThresholdCommitted)
					.IsEnabled(this, &SSkeletalSimplificationOptions::IsHardEdgeAndleThresholdEnabled)
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SkeletonSimplification", "Skeleton Simplification").ToString())	
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SkeletonSimplification_BoneReduction", "Bones to Remove").ToString())
					.ToolTipText(LOCTEXT("SkeletonSimplification_BoneReductionToolTip", "You can choose these bones in skeletaon tree as well.").ToString())
				]

				+SHorizontalBox::Slot()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				.FillWidth(1.5f)
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.MaxWidth(100.f)
					[
						SNew(SAssetSearchBox)
						.HintText(NSLOCTEXT( "SkeletalSimplificationOptions", "Hint Text", "Type Bone Name..." ) )
						.OnTextCommitted( this, &SSkeletalSimplificationOptions::OnSearchBoxCommitted )
						.PossibleSuggestions( this, &SSkeletalSimplificationOptions::GetSearchSuggestions )
						.OnTextChanged(this, &SSkeletalSimplificationOptions::OnTextChanged )
						.DelayChangeNotificationsWhileTyping( true )
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.Text(FText::FromString(TEXT("+")))
						.OnClicked(this, &SSkeletalSimplificationOptions::OnAddBonesToRemove)
					]
				]
			]

			+SVerticalBox::Slot()
			.Padding(40.0f, 8.0f, 0.0f, 0.0f)
			.MaxHeight(200.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
 					SNew(STextBlock)					
 					.Text(FString(TEXT("")))
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				[
					SNew(SScrollBox)
					+SScrollBox::Slot()
					.Padding(5)
					[
						SAssignNew(BonesToRemoveListWidget, SVerticalBox)
					]
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(32.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MaxBonesPerVertex", "Max Bones Per Vertex").ToString())
				]

				+SHorizontalBox::Slot()
				.FillWidth(1.5f)
				.MaxWidth(100.0f)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SSpinBox<int32>)
					.MinValue(1)
					.MaxValue(4)
					.Value(MaxBonesPerVertex)
					.OnValueChanged(this, &SSkeletalSimplificationOptions::OnMaxBonesPerVertexChanged)
					.OnValueCommitted(this, &SSkeletalSimplificationOptions::OnMaxBonesPerVertexCommitted)
				]
			]
			
		];

	// need to refresh bones to remove to reflect initial state
	RefreshBonesToRemove();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SSkeletalSimplificationOptions::~SSkeletalSimplificationOptions()
{
}

void SSkeletalSimplificationOptions::OnQualityChanged(float InValue)
{
	Quality = InValue;
}

void SSkeletalSimplificationOptions::OnQualityCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnQualityChanged(InValue);
}

float SSkeletalSimplificationOptions::GetQualityValue() const
{
	return Quality;
}

void SSkeletalSimplificationOptions::OnWeldingThresholdChanged(float InValue)
{
	WeldingThreshold = InValue;
}

void SSkeletalSimplificationOptions::OnWeldingThresholdCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnWeldingThresholdChanged(InValue);
}

void SSkeletalSimplificationOptions::OnHardEdgeAngleThresholdChanged(float InValue)
{
	HardEdgeAngleThreshold = InValue;
}

void SSkeletalSimplificationOptions::OnHardEdgeAngleThresholdCommitted(float InValue, ETextCommit::Type CommitInfo)
{
	OnHardEdgeAngleThresholdChanged(InValue);
}


EVisibility SSkeletalSimplificationOptions::IsMaxDeviationVisible() const
{
	if(SimplificationTypeOptions.Find(SimplificationTypeCombo->GetSelectedItem()) == SMOT_MaxDeviation)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

EVisibility SSkeletalSimplificationOptions::IsNumTrianglesVisible() const
{
	if(SimplificationTypeOptions.Find(SimplificationTypeCombo->GetSelectedItem()) == SMOT_NumOfTriangles)
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}



void SSkeletalSimplificationOptions::OnMaxBonesPerVertexChanged(int32 InValue)
{
	MaxBonesPerVertex = InValue;
}

void SSkeletalSimplificationOptions::OnMaxBonesPerVertexCommitted(int32 InValue, ETextCommit::Type CommitInfo)
{
	OnMaxBonesPerVertexChanged(InValue);
}

void SSkeletalSimplificationOptions::OnSimplificationTypeSelectionChanged(TSharedPtr<FString> InValue, ESelectInfo::Type /*SelectInfo*/)
{
	QualityLabel->SetText(*InValue);
}

bool SSkeletalSimplificationOptions::IsHardEdgeAndleThresholdEnabled() const
{
	return RecomputeNormalsCheckbox->IsChecked();
}

FSkeletalMeshOptimizationSettings SSkeletalSimplificationOptions::GetLODSettings() const
{
	// @todo UE4: When the skeletal mesh editor is eventually converted to Slate this code should be shared between the Lod Manager and Simplification windows (See Static Mesh Editor)
	// Grab settings from the UI.
	float DesiredQuality = Quality;
	SkeletalMeshOptimizationImportance SilhouetteImportance = (SkeletalMeshOptimizationImportance) QualityOptions.Find(SilhouetteCombo->GetSelectedItem());
	SkeletalMeshOptimizationImportance TextureImportance = (SkeletalMeshOptimizationImportance) QualityOptions.Find(TextureCombo->GetSelectedItem());
	SkeletalMeshOptimizationImportance ShadingImportance = (SkeletalMeshOptimizationImportance) QualityOptions.Find(ShadingCombo->GetSelectedItem());
	SkeletalMeshOptimizationImportance SkinningImportance =  (SkeletalMeshOptimizationImportance) QualityOptions.Find(SkinningCombo->GetSelectedItem());

	FSkeletalMeshOptimizationSettings Settings;
	Settings.ReductionMethod = (SkeletalMeshOptimizationType)SimplificationTypeOptions.Find(SimplificationTypeCombo->GetSelectedItem());

	//Given the current reduction method set the max deviation or reduction ratio.
	if(Settings.ReductionMethod == SMOT_MaxDeviation)
	{
		Settings.MaxDeviationPercentage = (100.0f - DesiredQuality) / 2000.0f;
	}
	else
	{
		Settings.NumOfTrianglesPercentage = DesiredQuality / 100.0f;
	}

	Settings.SilhouetteImportance = SilhouetteImportance;
	Settings.TextureImportance = TextureImportance;
	Settings.ShadingImportance = ShadingImportance;
	Settings.WeldingThreshold = WeldingThreshold;
	Settings.bRecalcNormals = RecomputeNormalsCheckbox->IsChecked();
	Settings.NormalsThreshold = HardEdgeAngleThreshold;

	return Settings;
}

void SSkeletalSimplificationOptions::AddBonesToRemove(const FString & NewBonesToRemove)
{
	if ( NewBonesToRemove.IsEmpty() == false && PossibleSuggestions.Contains(NewBonesToRemove) )
	{
		int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(*NewBonesToRemove);
		if (BoneIndex != INDEX_NONE)
		{
			Skeleton->RemoveBoneFromLOD(LODSettingIndex+1, BoneIndex);
		}
		RefreshBonesToRemove();
	}

}

FReply SSkeletalSimplificationOptions::OnAddBonesToRemove() 
{
	AddBonesToRemove(CurrentBoneName.ToString());
	return FReply::Handled();
}

void SSkeletalSimplificationOptions::RefreshBonesToRemove()
{
	if (Skeleton->BoneReductionSettingsForLODs.Num() > LODSettingIndex)
	{
		TArray< FName > BonesToRemoveForDisplay;

		const TArray<FName> & BonesToRemove = Skeleton->BoneReductionSettingsForLODs[LODSettingIndex].BonesToRemove;

		// filter out only the parent, not all children
		int32 TotalNumOfBonesToRemove = BonesToRemove.Num();
		const FReferenceSkeleton & RefSkeleton = Skeleton->GetReferenceSkeleton();
		for (int32 I=0; I<TotalNumOfBonesToRemove; ++I)
		{
			bool bNoParent = true;
			FName BoneName = BonesToRemove[I];
			int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
			int32 ParentBoneIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
			for (int32 J=0; J<TotalNumOfBonesToRemove; ++J)
			{
				int32 CurrentBoneIndex = RefSkeleton.FindBoneIndex(BonesToRemove[J]);
				if (CurrentBoneIndex == ParentBoneIndex)
				{
					bNoParent = false;
					break;
				}
			}

			// if no parent, we mark that as to display
			if (bNoParent)
			{
				BonesToRemoveForDisplay.AddUnique(BoneName);
			}
		}

		BonesToRemoveListWidget->ClearChildren();

		for (auto Iter = BonesToRemoveForDisplay.CreateIterator(); Iter; ++Iter)
		{
			BonesToRemoveListWidget->AddSlot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.MaxWidth(150.f)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(Iter->ToString())
					.ShadowOffset(FVector2D(1.0f, 1.0f))
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FString(TEXT("X")))
					.OnClicked(this, &SSkeletalSimplificationOptions::OnRemoveFromBonesList, *Iter)
				]
			];
		}
	}
}

void SSkeletalSimplificationOptions::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	AddBonesToRemove(InSearchText.ToString());
}

TArray<FString> SSkeletalSimplificationOptions::GetSearchSuggestions() const
{
	return PossibleSuggestions;
}

FReply SSkeletalSimplificationOptions::OnRemoveFromBonesList(const FName Name)
{
	int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(Name);
	if (BoneIndex != INDEX_NONE)
	{
		Skeleton->AddBoneToLOD(LODSettingIndex+1, BoneIndex);
	}
	RefreshBonesToRemove();
	return FReply::Handled();
}

void SSkeletalSimplificationOptions::PopulatePossibleSuggestions(const USkeleton * InSkeleton)
{
	if( InSkeleton )
	{
		const TArray<FMeshBoneInfo>& Bones = InSkeleton->GetReferenceSkeleton().GetRefBoneInfo();
		for( auto BoneIt=Bones.CreateConstIterator(); BoneIt; ++BoneIt )
		{
			PossibleSuggestions.Add(BoneIt->Name.ToString());
		}
	}
}

void SSkeletalSimplificationOptions::OnTextChanged( const FText& CurrentText )
{
	CurrentBoneName = CurrentText;
}

#undef LOCTEXT_NAMESPACE
