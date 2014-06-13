// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Customizes a DataTable asset to use a dropdown
 */
class FAnimGraphNodeSlotDetails : public IDetailCustomization
{
public:
	FAnimGraphNodeSlotDetails(TWeakPtr<class FPersona> PersonalEditor);

	static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<class FPersona> PersonalEditor) 
	{
		return MakeShareable( new FAnimGraphNodeSlotDetails(PersonalEditor) );
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	
private:
	// persona editor 
	TWeakPtr<class FPersona> PersonaEditor;

	// property of the two 
	TSharedPtr<IPropertyHandle> SlotNodeNamePropertyHandle;
	TSharedPtr<IPropertyHandle> GroupNamePropertyHandle;

	// slot node names
	TSharedPtr< class STextComboBox>	SlotNameComboBox;
	TArray< TSharedPtr< FString > >		SlotNameComboList;

	void OnSlotNameChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	// slot node names buttons
	FReply OnAddSlotName(TSharedRef<SWidget> Widget);
	FReply OnManageSlotName();

	void AddSlotName(const FText & SlotNameToAdd, ETextCommit::Type CommitInfo);
	int32 RefreshSlotNames();

	// group names
	TSharedPtr< class STextComboBox>	GroupNameComboBox;
	TArray< TSharedPtr< FString > >		GroupNameComboList;

	void OnGroupNameChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	// slot node names buttons
	FReply OnAddGroupName(TSharedRef<SWidget> Widget);
	FReply OnManageGroupName();

	void AddGroupName(const FText & GroupNameToAdd, ETextCommit::Type CommitInfo);
	int32 RefreshGroupNames();

	USkeleton * Skeleton;
};
