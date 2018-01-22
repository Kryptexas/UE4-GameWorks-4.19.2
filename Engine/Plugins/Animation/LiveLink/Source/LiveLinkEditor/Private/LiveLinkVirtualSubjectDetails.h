// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/**
* Abstract base class of animation composite base
* This contains Composite Section data and some necessary interface to make this work
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "IDetailCustomization.h"
#include "Types/SlateEnums.h"
#include "SListView.h"

#include "LiveLinkVirtualSubject.h"
#include "LiveLinkClient.h"

#include "LiveLinkVirtualSubjectDetails.generated.h"

class IPropertyHandle;
class ITableRow;
class STableViewBase;
class FLiveLinkClient;

// Detail panel helper object for virtual subjects
UCLASS()
class ULiveLinkVirtualSubjectDetails : public UObject
{
	GENERATED_BODY()
public:
	ULiveLinkVirtualSubjectDetails(const FObjectInitializer& ObjectInitializer);

	//Proxy of the virtual subject for the details panel to interact with
	UPROPERTY(EditAnywhere, Category = Settings)
	FLiveLinkVirtualSubject VirtualSubjectProxy;

	FLiveLinkClient* Client;

	// Id of the virtual subject we are editing within the live link client
	FLiveLinkSubjectKey SubjectKey;

	// Begin UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
	// End UObject interface
};

/**
* Customizes a ULiveLinkVirtualSubjectDetails
*/
class FLiveLinkVirtualSubjectDetailCustomization : public IDetailCustomization
{
public:
	// Data type of out subject tree UI
	typedef TSharedPtr<FName> FSubjectEntryPtr;


	FLiveLinkVirtualSubjectDetailCustomization(FLiveLinkClient* InClient);

	static TSharedRef<IDetailCustomization> MakeInstance(FLiveLinkClient* InClient)
	{
		return MakeShared<FLiveLinkVirtualSubjectDetailCustomization>(InClient);
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization interface

private:
	
	// Handler function for source combo box changing
	void OnSourceChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	// Creates subject tree entry widget
	TSharedRef<ITableRow> OnGenerateWidgetForSubjectItem(FSubjectEntryPtr InItem, const TSharedRef<STableViewBase>& OwnerTable);

	FLiveLinkClient* Client;

	// Cached reference to our details builder so we can force refresh
	IDetailLayoutBuilder* MyDetailsBuilder;

	// Cached property pointers
	TSharedPtr<IPropertyHandle> SourceGuidPropertyHandle;
	TSharedPtr<IPropertyHandle> SubjectsPropertyHandle;

	// Cached data for the source combo box UI
	TArray<FGuid> SourceGuids;
	TArray<TSharedPtr<FString>> SourceListItems;

	// Cached data for the subject tree UI
	TArray<FSubjectEntryPtr> SubjectsListItems;
	TSharedPtr< SListView< FSubjectEntryPtr > > SubjectsListView;
};