// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "Widgets/Views/SListView.h"

class IDetailLayoutBuilder;
class UAnimGraphNode_PoseDriver;
class IPropertyHandle;
class FPoseDriverDetails;
struct FPoseDriverTarget;

/** Entry in backing list for target list widget */
struct FPDD_TargetInfo
{
	int32 TargetIndex;

	/** Static function for creating a new item, but ensures that you can only have a TSharedRef to one */
	static TSharedRef<FPDD_TargetInfo> Make(int32 InTargetIndex)
	{
		return MakeShareable(new FPDD_TargetInfo(InTargetIndex));
	}

protected:
	/** Hidden constructor, always use Make above */
	FPDD_TargetInfo(int32 InTargetIndex)
		: TargetIndex(InTargetIndex)
	{}
};

/** Type of target list widget */
typedef SListView< TSharedPtr<FPDD_TargetInfo> > SPDD_TargetListType;


/** Widget for displaying info on a paricular target */
class SPDD_TargetRow : public SMultiColumnTableRow< TSharedPtr<FPDD_TargetInfo> >
{
public:

	SLATE_BEGIN_ARGS(SPDD_TargetRow) {}
		SLATE_ARGUMENT(int32, TargetIndex)
		SLATE_ARGUMENT(TWeakPtr<FPoseDriverDetails>, PoseDriverDetails)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	/** Return underlying FPoseDriverTarget this widget represents */
	FPoseDriverTarget& GetTarget() const;
	/** If we are editing rotation or translation */
	bool IsEditingRotation() const;
	/** Call when target is modified, so change is propagated to preview instance */
	void NotifyTargetChanged();
	/** Get current weight of this target in preview */
	float GetTargetWeight() const;

	int32 GetTransRotWidgetIndex() const;
	TOptional<float> GetTranslationX() const;
	TOptional<float> GetTranslationY() const;
	TOptional<float> GetTranslationZ() const;
	TOptional<float> GetRotationRoll() const;
	TOptional<float> GetRotationPitch() const;
	TOptional<float> GetRotationYaw() const;
	TOptional<float> GetScale() const;
	FText GetTargetTitleText() const;
	FText GetTargetWeightText() const;
	FSlateColor GetWeightBarColor() const;

	void SetTranslationX(float NewX);
	void SetTranslationY(float NewY);
	void SetTranslationZ(float NewZ);
	void SetRotationRoll(float NewRoll);
	void SetRotationPitch(float NewPitch);
	void SetRotationYaw(float NewYaw);
	void SetScale(float NewScale);
	void SetDrivenNameText(const FText& NewText, ETextCommit::Type CommitType);

	void OnTargetExpansionChanged(bool bExpanded);
	FText GetDrivenNameText() const;
	void OnDrivenNameChanged(TSharedPtr<FName> NewName, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> MakeDrivenNameWidget(TSharedPtr<FName> InItem);

	/** Remove this target from  */
	void RemoveTarget();

	/** Pointer back to owning customization */
	TWeakPtr<FPoseDriverDetails> PoseDriverDetailsPtr;

	/** Index in the nodes PoseTargets array that we are drawing */
	int32 TargetIndex;
};


/** Details customization for PoseDriver node */
class FPoseDriverDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// Begin IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization interface

	FReply ClickedOnCopyFromPoseAsset();
	bool CopyFromPoseAssetIsEnabled() const;
	FReply ClickedOnAutoScaleFactors();
	bool AutoScaleFactorsIsEnabled() const;
	FReply ClickedAddTarget();
	TSharedRef<ITableRow> GenerateTargetRow(TSharedPtr<FPDD_TargetInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	void OnTargetSelectionChanged(TSharedPtr<FPDD_TargetInfo> InInfo, ESelectInfo::Type SelectInfo);
	void OnPoseAssetChanged();

	/** Remove a target from node */
	void RemoveTarget(int32 TargetIndex);

	/** Set the currently selected Target */
	void SelectTarget(int32 TargetIndex);

	/** Util to get the first selected PoseDriver node */
	UAnimGraphNode_PoseDriver* GetFirstSelectedPoseDriver() const;

	/**  Refresh list of TargetInfos, mirroring PoseTargets list on node */
	void UpdateTargetInfosList();

	/** Update list of options for targets to drive (use by combo box) */
	void UpdateDrivenNameOptions();


	/** Cached array of selected objects */
	TArray< TWeakObjectPtr<UObject> > SelectedObjectsList;
	/** Array source for target list */
	TArray< TSharedPtr<FPDD_TargetInfo> > TargetInfos;
	/** List of things a target can drive (curves or morphs), used by combo box */
	TArray< TSharedPtr<FName> > DrivenNameOptions;
	/** Target list widget */
	TSharedPtr<SPDD_TargetListType> TargetListWidget;
	/** Property handle to node */
	TSharedPtr<IPropertyHandle> NodePropHandle;
};
