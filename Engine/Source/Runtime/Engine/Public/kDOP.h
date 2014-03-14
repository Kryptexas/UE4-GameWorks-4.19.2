// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


// ONLY HERE FOR BACKWARDS COMPAT SERIALIZATION
// Can remove this file once VER_UE4_REMOVE_KDOP_PROPERTIES is no longer supported


#ifndef _KDOP_H
#define _KDOP_H


// Indicates how many "k / 2" there are in the k-DOP. 3 == AABB == 6 DOP
#define NUM_PLANES	3

template<typename BOUND_TYPE>
struct MS_ALIGN(16) FEnsurePadding : public BOUND_TYPE
{
	float Pad[2];
	FEnsurePadding()
	{
	}
	FEnsurePadding(const BOUND_TYPE& InBound)
		: BOUND_TYPE(InBound)
	{
	}
} GCC_ALIGN(16);


template<typename KDOP_IDX_TYPE> struct FkDOPCollisionTriangle
{
	KDOP_IDX_TYPE v1, v2, v3;
	KDOP_IDX_TYPE MaterialIndex;

	friend FArchive& operator<<(FArchive& Ar, FkDOPCollisionTriangle<KDOP_IDX_TYPE>& Tri)
	{
		Ar << Tri.v1 << Tri.v2 << Tri.v3 << Tri.MaterialIndex;
		return Ar;
	}
};


// Forward declarations
template <typename KDOP_IDX_TYPE> struct TkDOP;
template <typename KDOP_IDX_TYPE> struct TkDOPNode;
template <typename KDOP_IDX_TYPE> struct TkDOPTree;
template <typename KDOP_IDX_TYPE> struct TkDOPCompact;
template <typename KDOP_IDX_TYPE> struct TkDOPNodeCompact;
template <typename KDOP_IDX_TYPE> struct TkDOPTreeCompact;


struct FkDOPPlanes
{
	static FVector PlaneNormals[NUM_PLANES];
};


template <typename KDOP_IDX_TYPE> struct TkDOP :
	public FkDOPPlanes
{
	float Min[NUM_PLANES];
	float Max[NUM_PLANES];

	// Serialization
	friend FArchive& operator<<(FArchive& Ar,TkDOP<KDOP_IDX_TYPE>& kDOP)
	{
		for (int32 nIndex = 0; nIndex < NUM_PLANES; nIndex++)
		{
			Ar << kDOP.Min[nIndex];
		}

		for (int32 nIndex = 0; nIndex < NUM_PLANES; nIndex++)
		{
			Ar << kDOP.Max[nIndex];
		}
		return Ar;
	}
};


template <typename KDOP_IDX_TYPE> struct TkDOPNode
{
	typedef TkDOP<KDOP_IDX_TYPE>		kDOPType;
	typedef TkDOPNode<KDOP_IDX_TYPE>	NodeType;

	kDOPType BoundingVolume;
	bool bIsLeaf;

	union
	{
		struct
		{
			KDOP_IDX_TYPE LeftNode;
			KDOP_IDX_TYPE RightNode;
		} n;

		struct
		{
			KDOP_IDX_TYPE NumTriangles;
			KDOP_IDX_TYPE StartIndex;
		} t;
	};

	friend FArchive& operator<<(FArchive& Ar,NodeType& Node)
	{
		Ar << Node.BoundingVolume << Node.bIsLeaf;
		Ar << Node.n.LeftNode << Node.n.RightNode;
		return Ar;
	}
};

template<typename KDOP_IDX_TYPE> struct TkDOPTree
{
	typedef TkDOPNode<KDOP_IDX_TYPE>	NodeType;

	TArray<NodeType> Nodes;
	TArray<FkDOPCollisionTriangle<KDOP_IDX_TYPE> > Triangles;

	friend FArchive& operator<<(FArchive& Ar,TkDOPTree<KDOP_IDX_TYPE>& Tree)
	{
		Tree.Nodes.BulkSerialize( Ar );
		Tree.Triangles.BulkSerialize( Ar );
		return Ar;
	}
};


template <typename KDOP_IDX_TYPE> struct TkDOPCompact :
	public FkDOPPlanes
{
	typedef TkDOP<KDOP_IDX_TYPE> BoundType;

	uint8 Min[NUM_PLANES];
	uint8 Max[NUM_PLANES];

	friend FArchive& operator<<(FArchive& Ar,TkDOPCompact<KDOP_IDX_TYPE>& kDOP)
	{
		for (int32 nIndex = 0; nIndex < NUM_PLANES; nIndex++)
		{
			Ar << kDOP.Min[nIndex];
		}

		for (int32 nIndex = 0; nIndex < NUM_PLANES; nIndex++)
		{
			Ar << kDOP.Max[nIndex];
		}
		return Ar;
	}
};


template <typename KDOP_IDX_TYPE> struct TkDOPNodeCompact
{
	typedef TkDOPNodeCompact<KDOP_IDX_TYPE>	NodeCompactType;
	typedef TkDOPCompact<KDOP_IDX_TYPE>		kDOPCompactType;
	typedef TkDOP<KDOP_IDX_TYPE>				kDOPUncompressedType;

	kDOPCompactType BoundingVolume;

	friend FArchive& operator<<(FArchive& Ar,NodeCompactType& Node)
	{
		Ar << Node.BoundingVolume;
		return Ar;
	}
};


template<typename KDOP_IDX_TYPE> struct TkDOPTreeCompact
{
	typedef TkDOPNodeCompact<KDOP_IDX_TYPE>	NodeType;
	typedef typename NodeType::kDOPUncompressedType				kDOPUncompressedType;

	TArray<NodeType>											Nodes;
	TArray<FkDOPCollisionTriangle<KDOP_IDX_TYPE> >				Triangles;
	FEnsurePadding<kDOPUncompressedType>						RootBound;

	// Serialization
	friend FArchive& operator<<(FArchive& Ar,TkDOPTreeCompact<KDOP_IDX_TYPE>& Tree)
	{
		Ar << Tree.RootBound;
		Tree.Nodes.BulkSerialize( Ar );
		Tree.Triangles.BulkSerialize( Ar );
		check(!Tree.Nodes.Num() || (PTRINT(&Tree.Nodes[0]) & 3) == 0); // SIMD will need this avoid the potential for page faults
		return Ar;
	}

};

#endif
