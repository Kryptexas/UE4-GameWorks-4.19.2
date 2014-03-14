// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#pragma once
#include "Slate.h"

class ENGINE_API SGameWidget : public SCompoundWidget
{
public:
 	void Construct( const FGameWorldContext& InGameWidgetContext );

	/* Return the world context. */
	UWorld* GetWorld() const;

 	/* Returns the local player. */
 	class ULocalPlayer* GetLocalPlayer() const;

	/* Returns a templated cast of the local player. */
	template< class T >
	T* GetLocalPlayer() const
	{
		return Cast<T>(GetLocalPlayer());
	}
	 
 	/* Returns the player controller. */
 	class APlayerController* GetPlayerController() const;

	/* Returns a templated cast of the player controller. */
	template< class T >
	T* GetPlayerController() const
	{
		return Cast<T>(GetPlayerController());
	}	

protected:

	/** Disallow public construction */
	FORCENOINLINE SGameWidget()		
	{
	}
private:
	FGameWorldContext			WorldContext;
};
