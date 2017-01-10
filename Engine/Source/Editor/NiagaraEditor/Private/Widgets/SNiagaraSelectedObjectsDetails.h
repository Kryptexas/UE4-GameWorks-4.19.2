// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "Delegate.h"
#include "DeclarativeSyntaxSupport.h"

class FNiagaraObjectSelection;
class IDetailsView;

/** A widget for viewing and editing a set of selected objects with a details panel. */
class SNiagaraSelectedObjectsDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNiagaraSelectedObjectsDetails)
	{}

	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedRef<FNiagaraObjectSelection> InSelectedObjects);

private:
	/** Called whenever the object selection changes. */
	void SelectedObjectsChanged();

private:
	/** The selected objects being viewed and edited by this widget. */
	TSharedPtr<FNiagaraObjectSelection> SelectedObjects;

	/** The details view for the selected object. */
	TSharedPtr<IDetailsView> DetailsView;
};