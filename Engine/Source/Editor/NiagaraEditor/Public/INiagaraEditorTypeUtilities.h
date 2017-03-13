// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SharedPointer.h"
#include "Delegate.h"

class FStructOnScope;
class SNiagaraParameterEditor;

class INiagaraEditorTypeUtilities
{
public:
	DECLARE_DELEGATE(FNotifyValueChanged);
public:
	virtual ~INiagaraEditorTypeUtilities() {}

	virtual bool CanProvideDefaultValue() const = 0;

	virtual void UpdateStructWithDefaultValue(TSharedRef<FStructOnScope> Struct) const = 0;

	virtual bool CanCreateParameterEditor() const = 0;

	virtual TSharedPtr<SNiagaraParameterEditor> CreateParameterEditor() const = 0;
};

class FNiagaraEditorTypeUtilities : public INiagaraEditorTypeUtilities
{
public:
	DECLARE_DELEGATE(FNotifyValueChanged);
public:
	//~ INiagaraEditorTypeUtilities
	virtual bool CanProvideDefaultValue() const override { return false; }
	virtual void UpdateStructWithDefaultValue(TSharedRef<FStructOnScope> Struct) const override { }
	virtual bool CanCreateParameterEditor() const override { return false; }
	virtual TSharedPtr<SNiagaraParameterEditor> CreateParameterEditor() const override { return TSharedPtr<SNiagaraParameterEditor>(); }
};