// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/////////////////////////////////////////////////////
// STiledLandcapeImportDlg
// 
class STiledLandcapeImportDlg
	: public SCompoundWidget
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


	SLATE_BEGIN_ARGS( STiledLandcapeImportDlg )
		{}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, TSharedPtr<SWindow> InParentWindow);

	/** */
	const FTiledLandscapeImportSettings& GetImportSettings() const;

private:
	TSharedRef<SWidget> HandleTileConfigurationComboBoxGenarateWidget(TSharedPtr<FTileImportConfiguration> InItem) const;
	FText				GetTileConfigurationText() const;

	/** */
	TOptional<float> GetScaleX() const;
	TOptional<float> GetScaleY() const;
	TOptional<float> GetScaleZ() const;
	void OnSetScale(float InValue, ETextCommit::Type CommitType, int32 InAxis);

	/** */
	void OnSetImportConfiguration(TSharedPtr<FTileImportConfiguration> InTileConfig, ESelectInfo::Type SelectInfo);

	/** */
	bool IsImportEnabled() const;
		
	/** */
	FReply OnClickedSelectTiles();
	FReply OnClickedImport();
	FReply OnClickedCancel();

	/** */
	FText GetImportSummaryText() const;

	/** */
	void SetPossibleConfigurationsForResolution(int32 TargetResolutuion);
	
	/** */
	void GenerateAllPossibleTileConfigurations();

	/** */
	FText GenerateConfigurationText(int32 NumComponents, int32 NumSectionsPerComponent,	int32 NumQuadsPerSection) const;

private:
	/** */
	TSharedPtr<SWindow> ParentWindow;
	
	/** */
	TSharedPtr<SComboBox<TSharedPtr<FTileImportConfiguration>>> TileConfigurationComboBox;
	
	/** */
	FTiledLandscapeImportSettings	ImportSettings;
	
	TArray<FTileImportConfiguration> AllConfigurations;
	TArray<TSharedPtr<FTileImportConfiguration>> ActiveConfigurations;
};

