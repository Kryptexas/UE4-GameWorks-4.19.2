// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/////////////////////////////////////////////////////
// STiledLandcapeImportDlg
// 
class STiledLandcapeImportDlg
	: public SCompoundWidget
	, public FGCObject
{
public:	
	/** */		
	struct FTileImportConfiguration
	{
		int32 Resolutuion;
		
		int32 NumComponents;
		int32 NumSectionsPerComponent;
		int32 NumQuadsPerSection;
	};

	struct FLandscapeImportLayerData
	{
		FString			LayerName;
		TArray<FString> WeighmapFileList;
		bool			bBlend;
	};

	SLATE_BEGIN_ARGS( STiledLandcapeImportDlg )
		{}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, TSharedPtr<SWindow> InParentWindow);

	/** */
	const FTiledLandscapeImportSettings& GetImportSettings() const;

private:
	/** FGCObject interface*/
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	/** */
	TSharedRef<SWidget> CreateLandscapeMaterialPicker();
	FText GetLandscapeMaterialName() const;

	/** */
	TSharedRef<SWidget> HandleTileConfigurationComboBoxGenarateWidget(TSharedPtr<FTileImportConfiguration> InItem) const;
	FText				GetTileConfigurationText() const;
	
	/** */
	TSharedRef<ITableRow> OnGenerateWidgetForLayerDataListView(TSharedPtr<FLandscapeImportLayerData> InLayerData, const TSharedRef<STableViewBase>& OwnerTable);

	/** */
	TOptional<float> GetScaleX() const;
	TOptional<float> GetScaleY() const;
	TOptional<float> GetScaleZ() const;
	void OnSetScale(float InValue, ETextCommit::Type CommitType, int32 InAxis);

	/** */
	TOptional<int32> GetTileOffsetX() const;
	TOptional<int32> GetTileOffsetY() const;
	void SetTileOffsetX(int32 InValue);
	void SetTileOffsetY(int32 InValue);
	
	/** */
	void OnSetImportConfiguration(TSharedPtr<FTileImportConfiguration> InTileConfig, ESelectInfo::Type SelectInfo);

	/** */
	bool IsImportEnabled() const;
		
	/** */
	FReply OnClickedSelectHeightmapTiles();
	FReply OnClickedSelectWeighmapTiles(TSharedPtr<FLandscapeImportLayerData> InLayerData);
	FReply OnClickedImport();
	FReply OnClickedCancel();

	/** */
	void OnLandscapeMaterialChanged(const FAssetData& AssetData);

	/** */
	FText GetImportSummaryText() const;
	FText GetWeightmapCountText(TSharedPtr<FLandscapeImportLayerData> InLayerData) const; 

	/** */
	void SetPossibleConfigurationsForResolution(int32 TargetResolutuion);
	
	/** */
	void GenerateAllPossibleTileConfigurations();

	/** */
	FText GenerateConfigurationText(int32 NumComponents, int32 NumSectionsPerComponent,	int32 NumQuadsPerSection) const;

	/** */
	void UpdateLandscapeLayerList();

private:
	/** */
	TSharedPtr<SWindow> ParentWindow;
		
	/** */
	TSharedPtr<SComboBox<TSharedPtr<FTileImportConfiguration>>> TileConfigurationComboBox;
	
	/** */
	TSharedPtr<SComboButton> LandscapeMaterialComboButton;

	TSharedPtr<SListView<TSharedPtr<FLandscapeImportLayerData>>>	LayerDataListView;
	TArray<TSharedPtr<FLandscapeImportLayerData>>					LayerDataList;		

	
	/** */
	FTiledLandscapeImportSettings ImportSettings;
	
	/** */
	FIntRect TotalLandscapeRect;	
	
	TArray<FTileImportConfiguration> AllConfigurations;
	TArray<TSharedPtr<FTileImportConfiguration>> ActiveConfigurations;
};

