// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteEditorOnlyTypes.h"

#if WITH_EDITOR

// PaperGeomTools is editor only, used only by PaperSprite.cpp
class PaperGeomTools
{
public:
	// Returns a list of polygons with the points in the corresponding winding to the winding flag on FSpritePolygon
	// Ie. If the vertex order doesn't match the winding, it is reversed
	// Entries in the array that aren't polygons (< 3 points) are removed
	static TArray<FSpritePolygon> CorrectPolygonWinding(const TArray<FSpritePolygon>& Polygons);

	// Returns true if the points forming a polygon have CCW winding
	// Returns true if the polygon isn't valid
	static bool IsPolygonWindingCCW(const TArray<FVector2D>& Points);

	// Checks that these polygons can be successfully triangulated	
	static bool ArePolygonsValid(const TArray<FSpritePolygon>& Polygons);

	// Merge additive and subtractive polygons, split them up into additive polygons
	// Assumes all polygons and overlapping polygons are valid, and the windings match the setting on the polygon
	static TArray<FSpritePolygon> ReducePolygons(const TArray<FSpritePolygon>& Polygons);

	// Triangulate a polygon. Check notes in implementation
	static bool TriangulatePoly(TArray<FVector2D>& OutTris, const TArray<FVector2D>& PolygonVertices, bool bKeepColinearVertices);

	// Wrapper around GeomTools.cpp, RemoveRedundantTriangles
	static void RemoveRedundantTriangles(TArray<FVector2D>& OutTriangles, const TArray<FVector2D>& InTriangles);
};

#endif
