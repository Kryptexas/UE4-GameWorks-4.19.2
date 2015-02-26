// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FSelectedItem

class FSelectedItem
{
protected:
	FSelectedItem(FName InTypeName)
		: TypeName(InTypeName)
	{
	}

protected:
	FName TypeName;
public:
	virtual bool IsA(FName TestType) const
	{
		return TestType == TypeName;
	}

	virtual uint32 GetTypeHash() const
	{
		return 0;
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const
	{
		return false;
	}

	virtual void ApplyDelta(const FVector2D& Delta, const FRotator& Rotation, const FVector& Scale3D, FWidget::EWidgetMode MoveMode)
	{
	}

	//@TODO: Doesn't belong here in base!
	virtual void SplitEdge()
	{
	}

	virtual FVector GetWorldPos() const
	{
		return FVector::ZeroVector;
	}

	virtual const class FSpriteSelectedVertex* CastSelectedVertex() const
	{
		return nullptr;
	}

	virtual ~FSelectedItem() {}
};

inline uint32 GetTypeHash(const FSelectedItem& Item)
{
	return Item.GetTypeHash();
}

inline bool operator==(const FSelectedItem& V1, const FSelectedItem& V2)
{
	return V1.Equals(V2);
}

