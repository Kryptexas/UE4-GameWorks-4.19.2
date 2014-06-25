// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InteractiveTutorial.h"

/** Delegate for binding  a tutorials interactivity data */
DECLARE_DELEGATE_TwoParams(FBindInteractivityData, const FString&, const TSharedRef<class FInteractiveTutorial>&);


/**
 * Interactive tutorials allow certain predefined steps to require
 * the user to change the editor's state to continue
 */
class FInteractiveTutorials : public TSharedFromThis<FInteractiveTutorials>
{
public:
	FInteractiveTutorials(FSimpleDelegate SectionCompleted);

	/**
	 * Sets up delegates for use by the editor
	 *
	 * Should be called immediately after construction
	 * Can't be called during construction because it requires the shared pointer of this
	 */
	void SetupEditorHooks();

	/** Bind interactivity data to a particular UDN path */
	void BindInteractivityData(const FString& InUDNPath, FBindInteractivityData InBindInteractivityData);

	/** Sets the currently viewed tutorial. Returns true if we should skip this page. */
	void SetCurrentTutorial(const FString& InUDNPath);

	/** Get the style of the current tutorial */
	ETutorialStyle::Type GetCurrentTutorialStyle();

	/** Sets the currently viewed excerpt  */
	void SetCurrentExcerpt(const FString& NewExcerpt);

	/** Determines if the passed in excerpt is interactive or not */
	bool IsExcerptInteractive(const FString& ExcerptName);

	/** Decides if the "next" button should be active on the given except */
	bool CanManuallyAdvanceExcerpt(const FString& ExcerptName);

	/** Decides if the "back" button should be active on the given except */
	bool CanManuallyReverseExcerpt(const FString& ExcerptName);

	/** Determines if we should skip an excerpt */
	bool ShouldSkipExcerpt(const FString& ExcerptName);

	/** Get the dialogue audio to play for an excerpt */
	UDialogueWave* GetDialogueForExcerpt(const FString& ExcerptName);

	/** Complete a specific excerpt */
	void CompleteExcerpt(const FString& InExcerpt);

	/** Notification that an excerpt has been completed. */
	void OnExcerptCompleted(const FString& InExcerpt);

	/** Check whether a particular widget should be highlighted */
	bool IsWidgetHighlighted(const FName& InWidgetName) const;

private:
	/** Gets the trigger delegate from the signature */
	template<typename DelegateSignature>
	DelegateSignature GetTriggerDelegate(FBaseTriggerDelegate::Type DelegateType);
	
	/** Executes the function for delegate completion */
	void ExecuteCompletion(bool bSuccessfulCompletion);

	/** Templated functions for handling all the different kinds of delegates that are triggerable */
	template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType>
	void OnDelegateTriggered();
	template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type>
	void OnDelegateTriggered(Param1Type Param1);
	template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type, typename Param2Type>
	void OnDelegateTriggered(Param1Type Param1, Param2Type Param2);

	template<typename DelegateType, FBaseTriggerDelegate::Type DelegateEnumType, typename Param1Type, typename Param2Type, typename Param3Type, typename Param4Type>
	void OnDelegateTriggered(Param1Type Param1, Param2Type Param2, Param3Type Param3, Param4Type Param4);


private:
	/** Map of tutorial UDN paths to interactive tutorials */
	TMap<FString, TSharedPtr<FInteractiveTutorial>> Tutorials;

	/** Current tutorial */
	TSharedPtr<FInteractiveTutorial> CurrentTutorial;

	/** Delegate to call when the current excerpt's conditions are completed */
	FSimpleDelegate ExcerptConditionsCompleted;
};
