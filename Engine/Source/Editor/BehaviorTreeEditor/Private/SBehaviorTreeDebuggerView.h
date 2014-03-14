// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"

namespace EBTDebuggerBlackboard
{
	enum Type
	{
		Saved,
		Current,
	};
};

class SBehaviorTreeDebuggerView : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SBehaviorTreeDebuggerView )
	{ }
	SLATE_END_ARGS()

	SBehaviorTreeDebuggerView();
	void Construct(const FArguments& InArgs);
	void UpdateBlackboard(class UBlackboardData* BlackboardAsset);

	EVisibility GetStateSwitchVisibility() const;
	ESlateCheckBoxState::Type IsRadioChecked(EBTDebuggerBlackboard::Type ButtonId) const;
	void OnRadioChanged(ESlateCheckBoxState::Type NewRadioState, EBTDebuggerBlackboard::Type ButtonId);

	FString GetValueDesc(int32 ValueIdx) const;
	FString GetTimestampDesc() const;
	
	FORCEINLINE bool IsShowingCurrentState() const { return bCanShowCurrentState && bShowCurrentState; }

	TArray<FString> CurrentValues;
	TArray<FString> SavedValues;
	float SavedTimestamp;
	float CurrentTimestamp;
	bool bCanShowCurrentState;

protected:

	TSharedPtr<SVerticalBox> MyContent;
	bool bShowCurrentState;
};
