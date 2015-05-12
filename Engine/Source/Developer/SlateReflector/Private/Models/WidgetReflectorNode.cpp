// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateReflectorPrivatePCH.h"
#include "WidgetReflectorNode.h"
#include "ReflectionMetadata.h"

#define LOCTEXT_NAMESPACE "WidgetReflectorNode"

/**
 * -----------------------------------------------------------------------------
 * UWidgetReflectorNodeBase
 * -----------------------------------------------------------------------------
 */
void UWidgetReflectorNodeBase::Initialize(const FArrangedWidget& InWidgetGeometry)
{
	Geometry = InWidgetGeometry.Geometry;
	Tint = FLinearColor(1.0f, 1.0f, 1.0f);
}

TSharedPtr<SWidget> UWidgetReflectorNodeBase::GetLiveWidget() const
{
	return nullptr;
}

FText UWidgetReflectorNodeBase::GetWidgetType() const
{
	return FText::GetEmpty();
}

FText UWidgetReflectorNodeBase::GetWidgetVisibilityText() const
{
	return FText::GetEmpty();
}

FText UWidgetReflectorNodeBase::GetWidgetReadableLocation() const
{
	return FText::GetEmpty();
}

FString UWidgetReflectorNodeBase::GetWidgetFile() const
{
	return FString();
}

int32 UWidgetReflectorNodeBase::GetWidgetLineNumber() const
{
	return 0;
}

FName UWidgetReflectorNodeBase::GetWidgetAssetName() const
{
	return NAME_None;
}

FVector2D UWidgetReflectorNodeBase::GetWidgetDesiredSize() const
{
	return FVector2D::ZeroVector;
}

FSlateColor UWidgetReflectorNodeBase::GetWidgetForegroundColor() const
{
	return FSlateColor::UseForeground();
}

FString UWidgetReflectorNodeBase::GetWidgetAddress() const
{
	return FString::Printf(TEXT("0x%p"), static_cast<const void*>(nullptr));
}

bool UWidgetReflectorNodeBase::GetWidgetEnabled() const
{
	return false;
}

const FGeometry& UWidgetReflectorNodeBase::GetGeometry() const
{
	return Geometry;
}

const FLinearColor& UWidgetReflectorNodeBase::GetTint() const
{
	return Tint;
}

void UWidgetReflectorNodeBase::SetTint(const FLinearColor& InTint)
{
	Tint = InTint;
}

void UWidgetReflectorNodeBase::AddChildNode(UWidgetReflectorNodeBase* InChildNode)
{
	ChildNodes.Add(InChildNode);
}

const TArray<UWidgetReflectorNodeBase*>& UWidgetReflectorNodeBase::GetChildNodes() const
{
	return ChildNodes;
}


/**
 * -----------------------------------------------------------------------------
 * ULiveWidgetReflectorNode
 * -----------------------------------------------------------------------------
 */
void ULiveWidgetReflectorNode::Initialize(const FArrangedWidget& InWidgetGeometry)
{
	Super::Initialize(InWidgetGeometry);

	Widget = InWidgetGeometry.Widget;
}

TSharedPtr<SWidget> ULiveWidgetReflectorNode::GetLiveWidget() const
{
	return Widget.Pin();
}

FText ULiveWidgetReflectorNode::GetWidgetType() const
{
	return FWidgetReflectorNodeUtils::GetWidgetType(Widget.Pin());
}

FText ULiveWidgetReflectorNode::GetWidgetVisibilityText() const
{
	return FWidgetReflectorNodeUtils::GetWidgetVisibilityText(Widget.Pin());
}

FText ULiveWidgetReflectorNode::GetWidgetReadableLocation() const
{
	return FWidgetReflectorNodeUtils::GetWidgetReadableLocation(Widget.Pin());
}

FString ULiveWidgetReflectorNode::GetWidgetFile() const
{
	return FWidgetReflectorNodeUtils::GetWidgetFile(Widget.Pin());
}

int32 ULiveWidgetReflectorNode::GetWidgetLineNumber() const
{
	return FWidgetReflectorNodeUtils::GetWidgetLineNumber(Widget.Pin());
}

FName ULiveWidgetReflectorNode::GetWidgetAssetName() const
{
	return FWidgetReflectorNodeUtils::GetWidgetAssetName(Widget.Pin());
}

FVector2D ULiveWidgetReflectorNode::GetWidgetDesiredSize() const
{
	return FWidgetReflectorNodeUtils::GetWidgetDesiredSize(Widget.Pin());
}

FSlateColor ULiveWidgetReflectorNode::GetWidgetForegroundColor() const
{
	return FWidgetReflectorNodeUtils::GetWidgetForegroundColor(Widget.Pin());
}

FString ULiveWidgetReflectorNode::GetWidgetAddress() const
{
	return FWidgetReflectorNodeUtils::GetWidgetAddress(Widget.Pin());
}

bool ULiveWidgetReflectorNode::GetWidgetEnabled() const
{
	return FWidgetReflectorNodeUtils::GetWidgetEnabled(Widget.Pin());
}


/**
 * -----------------------------------------------------------------------------
 * USnapshotWidgetReflectorNode
 * -----------------------------------------------------------------------------
 */
void USnapshotWidgetReflectorNode::Initialize(const FArrangedWidget& InWidgetGeometry)
{
	Super::Initialize(InWidgetGeometry);

	CachedWidgetType = FWidgetReflectorNodeUtils::GetWidgetType(InWidgetGeometry.Widget);
	CachedWidgetVisibilityText = FWidgetReflectorNodeUtils::GetWidgetVisibilityText(InWidgetGeometry.Widget);
	CachedWidgetReadableLocation = FWidgetReflectorNodeUtils::GetWidgetReadableLocation(InWidgetGeometry.Widget);
	CachedWidgetFile = FWidgetReflectorNodeUtils::GetWidgetFile(InWidgetGeometry.Widget);
	CachedWidgetLineNumber = FWidgetReflectorNodeUtils::GetWidgetLineNumber(InWidgetGeometry.Widget);
	CachedWidgetAssetName = FWidgetReflectorNodeUtils::GetWidgetAssetName(InWidgetGeometry.Widget);
	CachedWidgetDesiredSize = FWidgetReflectorNodeUtils::GetWidgetDesiredSize(InWidgetGeometry.Widget);
	CachedWidgetForegroundColor = FWidgetReflectorNodeUtils::GetWidgetForegroundColor(InWidgetGeometry.Widget);
	CachedWidgetAddress = FWidgetReflectorNodeUtils::GetWidgetAddress(InWidgetGeometry.Widget);
	CachedWidgetEnabled = FWidgetReflectorNodeUtils::GetWidgetEnabled(InWidgetGeometry.Widget);
}

FText USnapshotWidgetReflectorNode::GetWidgetType() const
{
	return CachedWidgetType;
}

FText USnapshotWidgetReflectorNode::GetWidgetVisibilityText() const
{
	return CachedWidgetVisibilityText;
}

FText USnapshotWidgetReflectorNode::GetWidgetReadableLocation() const
{
	return CachedWidgetReadableLocation;
}

FString USnapshotWidgetReflectorNode::GetWidgetFile() const
{
	return CachedWidgetFile;
}

int32 USnapshotWidgetReflectorNode::GetWidgetLineNumber() const
{
	return CachedWidgetLineNumber;
}

FName USnapshotWidgetReflectorNode::GetWidgetAssetName() const
{
	return CachedWidgetAssetName;
}

FVector2D USnapshotWidgetReflectorNode::GetWidgetDesiredSize() const
{
	return CachedWidgetDesiredSize;
}

FSlateColor USnapshotWidgetReflectorNode::GetWidgetForegroundColor() const
{
	return CachedWidgetForegroundColor;
}

FString USnapshotWidgetReflectorNode::GetWidgetAddress() const
{
	return CachedWidgetAddress;
}

bool USnapshotWidgetReflectorNode::GetWidgetEnabled() const
{
	return CachedWidgetEnabled;
}


/**
 * -----------------------------------------------------------------------------
 * FWidgetReflectorNodeUtils
 * -----------------------------------------------------------------------------
 */
ULiveWidgetReflectorNode* FWidgetReflectorNodeUtils::NewLiveNode(const FArrangedWidget& InWidgetGeometry)
{
	return Cast<ULiveWidgetReflectorNode>(NewNode(ULiveWidgetReflectorNode::StaticClass(), InWidgetGeometry));
}

ULiveWidgetReflectorNode* FWidgetReflectorNodeUtils::NewLiveNodeTreeFrom(const FArrangedWidget& InWidgetGeometry)
{
	return Cast<ULiveWidgetReflectorNode>(NewNodeTreeFrom(ULiveWidgetReflectorNode::StaticClass(), InWidgetGeometry));
}

USnapshotWidgetReflectorNode* FWidgetReflectorNodeUtils::NewSnapshotNode(const FArrangedWidget& InWidgetGeometry)
{
	return Cast<USnapshotWidgetReflectorNode>(NewNode(USnapshotWidgetReflectorNode::StaticClass(), InWidgetGeometry));
}

USnapshotWidgetReflectorNode* FWidgetReflectorNodeUtils::NewSnapshotNodeTreeFrom(const FArrangedWidget& InWidgetGeometry)
{
	return Cast<USnapshotWidgetReflectorNode>(NewNodeTreeFrom(USnapshotWidgetReflectorNode::StaticClass(), InWidgetGeometry));
}

UWidgetReflectorNodeBase* FWidgetReflectorNodeUtils::NewNode(const TSubclassOf<UWidgetReflectorNodeBase>& InNodeClass, const FArrangedWidget& InWidgetGeometry)
{
	UWidgetReflectorNodeBase* NewNodeInstance = NewObject<UWidgetReflectorNodeBase>(GetTransientPackage(), *InNodeClass);
	NewNodeInstance->Initialize(InWidgetGeometry);
	return NewNodeInstance;
}

UWidgetReflectorNodeBase* FWidgetReflectorNodeUtils::NewNodeTreeFrom(const TSubclassOf<UWidgetReflectorNodeBase>& InNodeClass, const FArrangedWidget& InWidgetGeometry)
{
	UWidgetReflectorNodeBase* NewNodeInstance = NewNode(InNodeClass, InWidgetGeometry);

	FArrangedChildren ArrangedChildren(EVisibility::All);
	InWidgetGeometry.Widget->ArrangeChildren(InWidgetGeometry.Geometry, ArrangedChildren);
	
	for (int32 WidgetIndex = 0; WidgetIndex < ArrangedChildren.Num(); ++WidgetIndex)
	{
		// Note that we include both visible and invisible children!
		NewNodeInstance->AddChildNode(NewNodeTreeFrom(InNodeClass, ArrangedChildren[WidgetIndex]));
	}

	return NewNodeInstance;
}

void FWidgetReflectorNodeUtils::FindLiveWidgetPath(const TArray<UWidgetReflectorNodeBase*>& CandidateNodes, const FWidgetPath& WidgetPathToFind, TArray<UWidgetReflectorNodeBase*>& SearchResult, int32 NodeIndexToFind)
{
	if (NodeIndexToFind < WidgetPathToFind.Widgets.Num())
	{
		const FArrangedWidget& WidgetToFind = WidgetPathToFind.Widgets[NodeIndexToFind];

		for (int32 NodeIndex = 0; NodeIndex < CandidateNodes.Num(); ++NodeIndex)
		{
			if (CandidateNodes[NodeIndex]->GetWidgetAddress() == GetWidgetAddress(WidgetPathToFind.Widgets[NodeIndexToFind].Widget))
			{
				SearchResult.Add(CandidateNodes[NodeIndex]);
				FindLiveWidgetPath(CandidateNodes[NodeIndex]->GetChildNodes(), WidgetPathToFind, SearchResult, NodeIndexToFind + 1);
			}
		}
	}
}

FText FWidgetReflectorNodeUtils::GetWidgetType(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? FText::FromString(InWidget->GetTypeAsString()) : FText::GetEmpty();
}

FText FWidgetReflectorNodeUtils::GetWidgetVisibilityText(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? FText::FromString(InWidget->GetVisibility().ToString()) : FText::GetEmpty();
}

FText FWidgetReflectorNodeUtils::GetWidgetReadableLocation(const TSharedPtr<SWidget>& InWidget)
{
	FText ReadableLocationText;

	if (InWidget.IsValid())
	{
		// UMG widgets have meta-data to help track them
		TSharedPtr<FReflectionMetaData> MetaData = InWidget->GetMetaData<FReflectionMetaData>();
		if (MetaData.IsValid() && MetaData->Asset.Get() != nullptr)
		{
			const FText AssetNameText = FText::FromName(MetaData->Asset->GetFName());
			const FText WidgetNameText = FText::FromName(MetaData->Name);
			ReadableLocationText = FText::Format(LOCTEXT("AssetReadableLocationFmt", "{0} [{1}]"), AssetNameText, WidgetNameText);
		}
		else
		{
			ReadableLocationText = FText::FromString(InWidget->GetReadableLocation());
		}
	}
	
	return ReadableLocationText;
}

FString FWidgetReflectorNodeUtils::GetWidgetFile(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? InWidget->GetCreatedInLocation().GetPlainNameString() : FString();
}

int32 FWidgetReflectorNodeUtils::GetWidgetLineNumber(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? InWidget->GetCreatedInLocation().GetNumber() : 0;
}

FName FWidgetReflectorNodeUtils::GetWidgetAssetName(const TSharedPtr<SWidget>& InWidget)
{
	FName AssetName;

	if (InWidget.IsValid())
	{
		// UMG widgets have meta-data to help track them
		TSharedPtr<FReflectionMetaData> MetaData = InWidget->GetMetaData<FReflectionMetaData>();
		if (MetaData.IsValid() && MetaData->Asset.Get() != nullptr)
		{
			AssetName = MetaData->Asset->GetFName();
		}
	}

	return AssetName;
}

FVector2D FWidgetReflectorNodeUtils::GetWidgetDesiredSize(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? InWidget->GetDesiredSize() : FVector2D::ZeroVector;
}

FString FWidgetReflectorNodeUtils::GetWidgetAddress(const TSharedPtr<SWidget>& InWidget)
{
	return FString::Printf(TEXT("0x%p"), InWidget.Get());
}

FSlateColor FWidgetReflectorNodeUtils::GetWidgetForegroundColor(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? InWidget->GetForegroundColor() : FSlateColor::UseForeground();
}

bool FWidgetReflectorNodeUtils::GetWidgetEnabled(const TSharedPtr<SWidget>& InWidget)
{
	return (InWidget.IsValid()) ? InWidget->IsEnabled() : false;
}

#undef LOCTEXT_NAMESPACE
