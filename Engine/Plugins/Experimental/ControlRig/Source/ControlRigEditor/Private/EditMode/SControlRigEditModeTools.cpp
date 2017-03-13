// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SControlRigEditModeTools.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "ISequencer.h"
#include "PropertyHandle.h"
#include "ControlRig.h"
#include "ControlRigEditModeSettings.h"
#include "IDetailRootObjectCustomization.h"
#include "ModuleManager.h"

class FControlRigRootCustomization : public IDetailRootObjectCustomization
{
public:
	// IDetailRootObjectCustomization interface
	virtual TSharedPtr<SWidget> CustomizeObjectHeader(const UObject* InRootObject) override
	{
		return SNullWidget::NullWidget;
	}

	virtual bool IsObjectVisible(const UObject* InRootObject) const override
	{
		return true;
	}

	virtual bool ShouldDisplayHeader(const UObject* InRootObject) const override
	{
		return false;
	}
};

void SControlRigEditModeTools::Construct(const FArguments& InArgs)
{
	// initialize settings view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bShowActorLabel = false;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
	}

	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	DetailsView->SetKeyframeHandler(SharedThis(this));
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization));
	DetailsView->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &SControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization));
	DetailsView->SetRootObjectCustomizationInstance(MakeShareable(new FControlRigRootCustomization));

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SControlRigEditModeTools::SetDetailsObjects(const TArray<TWeakObjectPtr<>>& InObjects)
{
	DetailsView->SetObjects(InObjects, true);
}

void SControlRigEditModeTools::SetSequencer(TSharedPtr<ISequencer> InSequencer)
{
	WeakSequencer = InSequencer;
}

bool SControlRigEditModeTools::IsPropertyKeyable(UClass* InObjectClass, const IPropertyHandle& InPropertyHandle) const
{
	FCanKeyPropertyParams CanKeyPropertyParams(InObjectClass, InPropertyHandle);

	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->CanKeyProperty(CanKeyPropertyParams))
	{
		return true;
	}

	return false;
}

bool SControlRigEditModeTools::IsPropertyKeyingEnabled() const
{
	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid() && Sequencer->GetFocusedMovieSceneSequence())
	{
		return true;
	}

	return false;
}

void SControlRigEditModeTools::OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle)
{
	TArray<UObject*> Objects;
	KeyedPropertyHandle.GetOuterObjects(Objects);
	FKeyPropertyParams KeyPropertyParams(Objects, KeyedPropertyHandle, ESequencerKeyMode::ManualKeyForced);

	TSharedPtr<ISequencer> Sequencer = WeakSequencer.Pin();
	if (Sequencer.IsValid())
	{
		Sequencer->KeyProperty(KeyPropertyParams);
	}
}

bool SControlRigEditModeTools::ShouldShowPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	bool bShow = InPropertyAndParent.Property.HasAnyPropertyFlags(CPF_Interp) || InPropertyAndParent.Property.HasMetaData(UControlRig::AnimationInputMetaName) || InPropertyAndParent.Property.HasMetaData(UControlRig::AnimationOutputMetaName);

	// Always show settings properties
	bShow |= Cast<UClass>(InPropertyAndParent.Property.GetOuter()) == UControlRigEditModeSettings::StaticClass();

	return bShow;
}

bool SControlRigEditModeTools::IsReadOnlyPropertyOnDetailCustomization(const FPropertyAndParent& InPropertyAndParent) const
{
	if (Cast<UClass>(InPropertyAndParent.Property.GetOuter()) != UControlRigEditModeSettings::StaticClass())
	{
		bool bReadOnly = !InPropertyAndParent.Property.HasMetaData(UControlRig::AnimationInputMetaName);

		if (InPropertyAndParent.ParentProperty)
		{
			bReadOnly |= !InPropertyAndParent.ParentProperty->HasMetaData(UControlRig::AnimationInputMetaName);
		}

		return bReadOnly;
	}

	return false;
}