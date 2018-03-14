// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorUndoClient.h"
#include "SCompoundWidget.h"
#include "DeclarativeSyntaxSupport.h"
#include "STableRow.h"
#include "STableViewBase.h"
#include "STreeView.h"
#include "NiagaraSystemViewModel.h"
#include "TickableEditorObject.h"
#include "WeakObjectPtrTemplates.h"
#include "NiagaraDataSet.h"
#include "SharedPointer.h"
#include "Map.h"
#include "SCheckBox.h"
#include "NiagaraParameterStore.h"

class SNiagaraSpreadsheetView : public SCompoundWidget, public FTickableEditorObject
{
public:
	SLATE_BEGIN_ARGS(SNiagaraSpreadsheetView)
	{}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraSystemViewModel> InSystemViewModel);
	virtual ~SNiagaraSpreadsheetView();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override {}
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	struct FieldInfo
	{
		uint32 FloatStartOffset;
		uint32 IntStartOffset;
		uint32 GlobalStartOffset;
		bool bFloat;
		bool bBoolean;
		TWeakObjectPtr<const UEnum> Enum;
	};

protected:

	enum EUITab : uint8
	{
		UIPerParticleUpdate = 0,
		UISystemUpdate,
		UIMax
	};

	void OnTabChanged(ECheckBoxState State,	EUITab Tab);
	ECheckBoxState GetTabCheckedState(EUITab Tab) const;
	EVisibility GetViewVisibility(EUITab Tab) const;

	void GenerateLayoutInfo(FNiagaraTypeLayoutInfo& Layout, const UScriptStruct* Struct, const UEnum* Enum, FName BaseName, TArray<FName>& PropertyNames, TArray<FieldInfo>& FieldInfo);

	EUITab TabState;

	struct CapturedUIData
	{
		CapturedUIData() : LastReadWriteId(-1), DataSet(nullptr), TargetUsage(ENiagaraScriptUsage::ParticleUpdateScript), bAwaitingFrame(false), LastCaptureTime(-FLT_MAX), bInputColumnsAreAttributes(true), bOutputColumnsAreAttributes(true)
		{ }

		TSharedPtr<SHeaderRow> OutputHeaderRow;
		TSharedPtr<SHeaderRow> InputHeaderRow;
		TSharedPtr < STreeView<TSharedPtr<int32> > > OutputsListView;
		TSharedPtr < STreeView<TSharedPtr<int32> > > InputsListView;
		TSharedPtr < SCheckBox > CheckBox;
		TArray< TSharedPtr<int32> > SupportedInputIndices;
		TArray< TSharedPtr<int32> > SupportedOutputIndices;
		int32 LastReadWriteId;
		FNiagaraDataSet* DataSet;
		FNiagaraParameterStore InputParams;
		TSharedPtr<TArray<FName> > SupportedInputFields;
		TSharedPtr<TArray<FName> > SupportedOutputFields;
		TSharedPtr<TMap<FName, FieldInfo> > InputFieldInfoMap;
		TSharedPtr<TMap<FName, FieldInfo> > OutputFieldInfoMap;
		ENiagaraScriptUsage TargetUsage;
		bool bAwaitingFrame;
		float LastCaptureTime;
		float TargetCaptureTime;
		FGuid LastCaptureHandleId;
		TWeakObjectPtr<UNiagaraEmitter> DataSource;
		TSharedPtr<SScrollBar> OutputHorizontalScrollBar;
		TSharedPtr<SScrollBar> OutputVerticalScrollBar;
		TSharedPtr<SScrollBar> InputHorizontalScrollBar;
		TSharedPtr<SScrollBar> InputVerticalScrollBar;
		TSharedPtr<SVerticalBox> Container;
		bool bInputColumnsAreAttributes;
		bool bOutputColumnsAreAttributes;
		FText ColumnName;
	};

	TArray<CapturedUIData> CaptureData;

	TSharedPtr<FNiagaraSystemViewModel> SystemViewModel;

	UEnum* ScriptEnum;

	void HandleTimeChange();
	
	void OnSequencerTimeChanged();

	FText LastCapturedInfoText() const;

	FReply OnCaptureRequestPressed();
	FReply OnCSVOutputPressed();

	bool CanCapture() const;
	bool IsPausedAtRightTimeOnRightHandle() const;

	void SelectedEmitterHandlesChanged();

	void ResetColumns(EUITab Tab);
	void ResetEntries(EUITab Tab);

	/**
	 * Function called when the currently selected event in the list of thread events changes
	 *
	 * @param Selection Currently selected event
	 * @param SelectInfo Provides context on how the selection changed
	 */
	void OnEventSelectionChanged(TSharedPtr<int32> Selection, ESelectInfo::Type SelectInfo, EUITab Tab, bool bInputList);

	/** 
	 * Generates SEventItem widgets for the events tree
	 *
	 * @param InItem Event to generate SEventItem for
	 * @param OwnerTable Owner Table
	 */
	TSharedRef< ITableRow > OnGenerateWidgetForList(TSharedPtr<int32> InItem, const TSharedRef< STableViewBase >& OwnerTable , EUITab Tab, bool bInputList);

	/** 
	 * Given a profiler event, generates children for it
	 *
	 * @param InItem Event to generate children for
	 * @param OutChildren Generated children
	 */
	void OnGetChildrenForList(TSharedPtr<int32> InItem, TArray<TSharedPtr<int32>>& OutChildren, EUITab Tab, bool bInputList);
};


class SNiagaraSpreadsheetRow : public SMultiColumnTableRow<TSharedPtr<int32> >
{
public:
	typedef TSharedPtr<TArray<FName> > NamesArray;
	typedef TSharedPtr<TMap <FName, SNiagaraSpreadsheetView::FieldInfo> > FieldsMap;

	SLATE_BEGIN_ARGS(SNiagaraSpreadsheetRow)
		: _RowIndex(0), _ColumnsAreAttributes(true), _DataSet(nullptr)
	{}
	SLATE_ARGUMENT(int32, RowIndex)
	SLATE_ARGUMENT(bool, ColumnsAreAttributes)
	SLATE_ARGUMENT(FNiagaraDataSet*, DataSet)
	SLATE_ARGUMENT(NamesArray, SupportedFields)
	SLATE_ARGUMENT(FieldsMap, FieldInfoMap)
	SLATE_ARGUMENT(bool, UseGlobalOffsets)
	SLATE_ARGUMENT(FNiagaraParameterStore*, ParameterStore)
	SLATE_END_ARGS()

		/**
		* Generates a widget for debug attribute list column
		*
		* @param InArgs   A declaration from which to construct the widget
		*/
		virtual TSharedRef< SWidget > GenerateWidgetForColumn(const FName& ColumnName) override;


	/**
	* Construct the widget
	*
	* @param InArgs   A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	int32 RowIndex;
	FNiagaraDataSet* DataSet;
	FNiagaraParameterStore* ParameterStore;
	TSharedPtr<TArray<FName> > SupportedFields;
	TSharedPtr<TMap<FName, SNiagaraSpreadsheetView::FieldInfo> > FieldInfoMap;
	bool ColumnsAreAttributes;
	bool UseGlobalOffsets;
};

