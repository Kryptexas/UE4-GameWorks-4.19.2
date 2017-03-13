// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraParameterEditMode.h"
#include "DelegateCombinations.h"
#include "Delegate.h"
#include "SlateTypes.h"
#include "StructOnScope.h"
#include "Text.h"

struct FNiagaraTypeDefinition;
struct FNiagaraVariable;

/** Defines the view model for a parameter in the parameter collection editor. */
class INiagaraParameterViewModel
{
public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDefaultValueChanged, const FNiagaraVariable*);
	DECLARE_MULTICAST_DELEGATE(FOnTypeChanged);

public:
	// Defines the type of default value this parameter provides.
	enum class EDefaultValueType
	{
		None,
		Struct,
		Object
	};

public:
	virtual ~INiagaraParameterViewModel() {}

	/** Gets a unique id for this parameter.  If this parameter has no unique id, the guid will not be valid. */
	virtual FGuid GetId() const = 0;

	/** Gets the name of the parameter. */
	virtual FName GetName() const = 0;

	/** Gets whether or not this parameter can be renamed. */
	virtual bool CanRenameParameter() const = 0;

	/** Gets the text representation of the name of the paramter. */
	virtual FText GetNameText() const = 0;

	/** Handles a hame text change being comitter from the UI. */
	virtual void NameTextComitted(const FText& Name, ETextCommit::Type CommitInfo) = 0;

	/** Gets the display name for the parameter's type. */
	virtual FText GetTypeDisplayName() const = 0;

	/** Gets whether or not the type of this parameter can be changed. */
	virtual bool CanChangeParameterType() const = 0;

	/** Gets the type of the paramter. */
	virtual TSharedPtr<FNiagaraTypeDefinition> GetType() const = 0;

	/** Handles the paramter type being changed from the UI. */
	virtual void SelectedTypeChanged(TSharedPtr<FNiagaraTypeDefinition> Item, ESelectInfo::Type SelectionType) = 0;

	/** Gets the type of default value this view model provides. */
	virtual EDefaultValueType GetDefaultValueType() = 0;

	/** Gets the struct representing the default value for the parameter. */
	virtual TSharedRef<FStructOnScope> GetDefaultValueStruct() = 0;

	/** Gets the object representing the default value for the parameter. */
	virtual UObject* GetDefaultValueObject() = 0;

	/** Called to notify the parameter view model that a property on the default value has been changed by the UI. */
	virtual void NotifyDefaultValuePropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent) = 0;

	/** Called to notify the parameter view model that a change to the default value has begun. */
	virtual void NotifyBeginDefaultValueChange() = 0;

	/** Called to notify the parameter view model that a change to the default value has ended. */
	virtual void NotifyEndDefaultValueChange() = 0;

	/** Called to notify the parameter view model that the default value has been changed by the UI. */
	virtual void NotifyDefaultValueChanged() = 0;

	/** Get a multicast delegate which is called whenever the default value of the parameter changes. */
	virtual FOnDefaultValueChanged& OnDefaultValueChanged() = 0;

	/** Gets a multicast delegate which is called whenever the type of this parameter changes. */
	virtual FOnTypeChanged& OnTypeChanged() = 0;
};

/** Base class for parameter view models.  Partially implements the parameter interface with
behavior common to all view models. */
class FNiagaraParameterViewModel : public INiagaraParameterViewModel
{
public:
	FNiagaraParameterViewModel(ENiagaraParameterEditMode ParameterEditMode);

	//~ INiagaraParameterViewModel interface
	virtual bool CanRenameParameter() const override;
	virtual FText GetNameText() const override;
	virtual bool CanChangeParameterType() const override;
	virtual FOnDefaultValueChanged& OnDefaultValueChanged() override;
	virtual FOnTypeChanged& OnTypeChanged() override { return OnTypeChangedDelegate; }

protected:
	/** Defines the edit mode for this parameter. */
	ENiagaraParameterEditMode ParameterEditMode;

	/** A multicast delegate which is called whenever the default value changes. */
	FOnDefaultValueChanged OnDefaultValueChangedDelegate;

	/** A multicast delegate which is called whenever the type of the parameter changes. */
	FOnTypeChanged OnTypeChangedDelegate;
};