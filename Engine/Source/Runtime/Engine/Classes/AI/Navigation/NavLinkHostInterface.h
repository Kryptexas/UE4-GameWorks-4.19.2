// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface.h"
#include "NavLinkHostInterface.generated.h"

struct FNavigationLink;
struct FNavigationSegmentLink;

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UNavLinkHostInterface : public UInterface
{
	GENERATED_BODY()
public:
	ENGINE_API UNavLinkHostInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class ENGINE_API INavLinkHostInterface
{
	GENERATED_BODY()
public:
		
	/**
	 *	Retrieves UNavLinkDefinition derived UClasses hosted by this interface implementor
	 */
	virtual bool GetNavigationLinksClasses(TArray<TSubclassOf<class UNavLinkDefinition> >& OutClasses) const PURE_VIRTUAL(INavLinkHostInterface::GetNavigationLinksClasses,return false;);

	/** 
	 *	_Optional_ way of retrieving navigation link data - if INavLinkHostInterface 
	 *	implementer defines custom navigation links then he can just retrieve 
	 *	a list of links
	 */
	virtual bool GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const { return false; }
};
