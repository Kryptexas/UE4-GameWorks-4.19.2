// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_OrientationDriver.h"
#include "CompilerResultsLog.h"
#include "PropertyEditing.h"
#include "AnimationCustomVersion.h"
#include "ContentBrowserModule.h"
#include "Animation/PoseAsset.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_OrientationDriver::UAnimGraphNode_OrientationDriver(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

FText UAnimGraphNode_OrientationDriver::GetTooltipText() const
{
	return LOCTEXT("UAnimGraphNode_OrientationDriver_ToolTip", "Interpolate parameters between defined orientation poses.");
}

FText UAnimGraphNode_OrientationDriver::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_OrientationDriver::GetControllerDescription() const
{
	return LOCTEXT("OrientationDriver", "Orientation Driver");
}

void UAnimGraphNode_OrientationDriver::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{

}

void UAnimGraphNode_OrientationDriver::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, USkeletalMeshComponent * PreviewSkelMeshComp) const
{
	// Get the node we are debugging
	FAnimNode_OrientationDriver* DebugNode = (FAnimNode_OrientationDriver*)FindDebugAnimNode(PreviewSkelMeshComp);

	if (DebugNode == nullptr)
	{
		return;
	}

	const int MarginX = 60;
	const int XDelta = 10;

	int PosY = 60;
	const int YDelta = 20;

	// Display bone we are watching
	FText InterpItemText = FText::Format(LOCTEXT("BoneFormat", "Bone: {0}"), FText::FromName(DebugNode->SourceBone.BoneName));
	FCanvasTextItem InterpItem(FVector2D(MarginX, PosY), InterpItemText, GEngine->GetSmallFont(), FLinearColor::White);
	Canvas.DrawItem(InterpItem);
	PosY += YDelta;

	// List each pose with weight
	int32 PoseIndex = 0;
	for (const FOrientationDriverPose& Pose : DebugNode->Poses)
	{
		FString PoseItemString = FString::Printf(TEXT("%d : %f"), PoseIndex, Pose.PoseWeight);
		FCanvasTextItem PoseItem(FVector2D(MarginX + XDelta, PosY), FText::FromString(PoseItemString), GEngine->GetSmallFont(), FLinearColor::White);
		Canvas.DrawItem(PoseItem);
		PosY += YDelta;

		PoseIndex++;
	}

	PosY += YDelta;

	// Parameters header
	FCanvasTextItem ParamsHeaderItem(FVector2D(MarginX, PosY), LOCTEXT("Parameters", "Parameters"), GEngine->GetSmallFont(), FLinearColor::White);
	Canvas.DrawItem(ParamsHeaderItem);
	PosY += YDelta;

	// Iterate over Parameters
	for (const FOrientationDriverParam& Param : DebugNode->ResultParamSet.Params)
	{
		FString ParamItemString = FString::Printf(TEXT("%s : %f"), *Param.ParameterName.ToString(), Param.ParameterValue);
		FCanvasTextItem ParamItem(FVector2D(MarginX + XDelta, PosY), FText::FromString(ParamItemString), GEngine->GetSmallFont(), FLinearColor::White);
		Canvas.DrawItem(ParamItem);
		PosY += YDelta;
	}
}


void UAnimGraphNode_OrientationDriver::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.SourceBone.BoneName) == INDEX_NONE)
	{
		MessageLog.Warning(*LOCTEXT("NoSourceBone", "@@ - You must pick a source bone as the Driver joint").ToString(), this);
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}


void UAnimGraphNode_OrientationDriver::OnPoseAssetSelected(const FAssetData& AssetData)
{

}

TSharedRef<SWidget> UAnimGraphNode_OrientationDriver::BuildPoseAssetPicker()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UPoseAsset::StaticClass()->GetFName());
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateUObject(this, &UAnimGraphNode_OrientationDriver::OnPoseAssetSelected);
	AssetPickerConfig.bAllowNullSelection = true;
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.bFocusSearchBoxWhenOpened = true;
	AssetPickerConfig.bShowBottomToolbar = false;
	AssetPickerConfig.SelectionMode = ESelectionMode::Single;

	return 
		SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(768)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];
}

void UAnimGraphNode_OrientationDriver::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& ImportCategory = DetailBuilder.EditCategory("Import Driver Setup");

	ImportCategory.AddCustomRow(LOCTEXT("ImportRow", "Import"))
	[
		SNew(SComboButton)
		.ContentPadding(3)
		.OnGetMenuContent(FOnGetContent::CreateUObject(this, &UAnimGraphNode_OrientationDriver::BuildPoseAssetPicker))
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ImportPosesFromPoseAsset ", "Import Poses From PoseAsset"))
			.ToolTipText(LOCTEXT("ImportPosesFromPoseAssetTooltip", "Import target poses from a PoseAsset"))
		]
	];
}


#undef LOCTEXT_NAMESPACE
