// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "PropertyPath.h"

class UObjectDataSource : public IDataSource
{
public:

	UObjectDataSource( const TWeakObjectPtr< UObject >& InObject )
		: Object( InObject )
	{
	}

	virtual TWeakObjectPtr< UObject > AsUObject() const override { return Object; }
	virtual TSharedPtr< FPropertyPath > AsPropertyPath() const override { return NULL; }

	virtual bool IsValid() const override { return Object.IsValid(); }

private:

	TWeakObjectPtr< UObject > Object;
};

class PropertyPathDataSource : public IDataSource
{
public:

	PropertyPathDataSource( const TSharedRef< FPropertyPath >& InPath )
		: Path( InPath )
	{
	}

	virtual TWeakObjectPtr< UObject > AsUObject() const override { return NULL; }
	virtual TSharedPtr< FPropertyPath > AsPropertyPath() const override { return Path; }

	virtual bool IsValid() const override { return true; }

private:

	TSharedRef< FPropertyPath > Path;
};

class NoDataSource : public IDataSource
{
public:

	virtual TWeakObjectPtr< UObject > AsUObject() const override { return NULL; }
	virtual TSharedPtr< FPropertyPath > AsPropertyPath() const override { return NULL; }

	virtual bool IsValid() const override { return false; }
};