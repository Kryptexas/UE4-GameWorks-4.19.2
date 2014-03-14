// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// LANDSCAPE SPLINES HIT PROXY

struct HLandscapeSplineProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	HLandscapeSplineProxy(EHitProxyPriority InPriority = HPP_Wireframe) :
		HHitProxy(InPriority)
	{
	}
	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::Crosshairs;
	}
};

struct HLandscapeSplineProxy_Segment : public HLandscapeSplineProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	class ULandscapeSplineSegment* SplineSegment;

	HLandscapeSplineProxy_Segment(class ULandscapeSplineSegment* InSplineSegment) :
		HLandscapeSplineProxy(),
		SplineSegment(InSplineSegment)
	{
	}
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Collector.AddReferencedObject( SplineSegment );
	}
};

struct HLandscapeSplineProxy_ControlPoint : public HLandscapeSplineProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	class ULandscapeSplineControlPoint* ControlPoint;

	HLandscapeSplineProxy_ControlPoint(class ULandscapeSplineControlPoint* InControlPoint) :
		HLandscapeSplineProxy(HPP_Foreground),
		ControlPoint(InControlPoint)
	{
	}
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE
	{
		Collector.AddReferencedObject( ControlPoint );
	}
};

struct HLandscapeSplineProxy_Tangent : public HLandscapeSplineProxy
{
	DECLARE_HIT_PROXY( ENGINE_API );

	class ULandscapeSplineSegment* SplineSegment;
	uint32 End:1;

	HLandscapeSplineProxy_Tangent(class ULandscapeSplineSegment* InSplineSegment, bool InEnd) :
		HLandscapeSplineProxy(HPP_UI),
		SplineSegment(InSplineSegment),
		End(InEnd)
	{
	}
	virtual void Serialize(FArchive& Ar)
	{
		Ar << SplineSegment;
	}
	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::CardinalCross;
	}
};
