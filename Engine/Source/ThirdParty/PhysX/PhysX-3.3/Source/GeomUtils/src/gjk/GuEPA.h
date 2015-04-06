/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef GU_EPA_H
#define GU_EPA_H

#include "CmPhysXCommon.h"
#include "GuEPAFacet.h"
#include "PsAllocator.h"
#include "GuConvexSupportTable.h"
#include "GuGJKSimplex.h"
#include "GuGJKUtil.h"

namespace physx
{

#define MaxFacets 64
#define MaxSupportPoints 64
  
namespace Gu
{
	PxGJKStatus epaPenetration(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, PxU8* PX_RESTRICT aInd, PxU8* PX_RESTRICT bInd, PxU8 _size, Ps::aos::Vec3V& contactA, Ps::aos::Vec3V& contactB, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penetrationDepth, const bool takeCoreShape = false);

	class ConvexV;

	template<class Element, PxU32 Size>
	class BinaryHeap 
	{
	public:
		BinaryHeap() 
		{
			heapSize = 0;
		}
		
		~BinaryHeap() 
		{
		}
		
		PX_FORCE_INLINE Element* getTop() 
		{
			//return heapTop;//data[0];
			return data[0];
		}
		
		PX_FORCE_INLINE bool isEmpty()
		{
			return (heapSize == 0);
		}
		
		PX_FORCE_INLINE void makeEmpty()
		{
			heapSize = 0;
		}  

		PX_FORCE_INLINE void insert(Element* value)
		{
			PX_ASSERT((PxU32)heapSize < Size);
			PxU32 newIndex;
			PxI32 parentIndex = parent(heapSize);
			for (newIndex = (PxU32)heapSize; newIndex > 0 && (*data[(PxU32)parentIndex]) > (*value); newIndex = (PxU32)parentIndex, parentIndex= parent((PxI32)newIndex)) 
			{
				data[ newIndex ] = data[parentIndex];
			}
			data[newIndex] = value; 
			heapSize++;
			PX_ASSERT(isValid());
		}


		PX_FORCE_INLINE Element* deleteTop()
		{
			PX_ASSERT(heapSize > 0);
			PxI32 i, child;
			//try to avoid LHS
			PxI32 tempHs = heapSize-1;
			heapSize = tempHs;
			Element* PX_RESTRICT min = data[0];
			Element* PX_RESTRICT last = data[tempHs];
			PX_ASSERT(tempHs != -1);
			
			for (i = 0; (child = left(i)) < tempHs; i = child) 
			{
				/* Find smaller child */
				const PxI32 rightChild = child + 1;
				/*if((rightChild < heapSize) && (*data[rightChild]) < (*data[child]))
					child++;*/
				child += ((rightChild < tempHs) & ((*data[rightChild]) < (*data[child]))) ? 1 : 0;

				if((*data[child]) >= (*last))
					break;

				PX_ASSERT(i >= 0 && i < (PxI32)Size);
				data[i] = data[child];
			}
			PX_ASSERT(i >= 0 && i < (PxI32)Size);
			data[ i ] = last;
			/*heapTop = min;*/
			PX_ASSERT(isValid());
			return min;
		} 

		bool isValid()
		{
			Element* min = data[0];
			for(PxI32 i=1; i<heapSize; ++i)
			{
				if((*min) > (*data[i]))
					return false;
			}

			return true;
		}


		PxI32 heapSize;
//	private:
		Element* PX_RESTRICT data[Size];
		
		PX_FORCE_INLINE PxI32 left(PxI32 nodeIndex) 
		{
			return (nodeIndex << 1) + 1;
		}
		
		PX_FORCE_INLINE PxI32 parent(PxI32 nodeIndex) 
		{
			return (nodeIndex - 1) >> 1;
		}
	};

	class EPAFacetManager
	{
	public:
		EPAFacetManager() : currentID(0), freeIDCount(0), deferredFreeIDCount(0)	{}

		void	freeID(PxU8 id)
		{
			if(id == (currentID - 1))
				--currentID;
			else
			{
				PX_ASSERT(freeIDCount < MaxFacets);
				freeIDs[freeIDCount++] = id;
			}
		}

		void	deferFreeID(PxU8 id)
		{
			PX_ASSERT(deferredFreeIDCount < MaxFacets);
			deferredFreeIDs[deferredFreeIDCount++] = id;
		}

		void processDeferredIds()
		{
			for(PxU8 a = deferredFreeIDCount; a >0; --a)
			{
				freeID(deferredFreeIDs[a-1]);
			}
			deferredFreeIDCount = 0;
		}

		void	freeAll()
		{
			currentID = 0;
			deferredFreeIDCount = freeIDCount = 0;
		}

		PxU8	getNewID()
		{
			// If recycled IDs are available, use them
			if(freeIDCount)
			{
				//const PxU8 size = --freeIDCount;
				const PxU8 size = PxU8(freeIDCount-1);
				freeIDCount--;
				//const PxU8 id = freeIDs[size]; // Recycle last ID
				return freeIDs[size];// Recycle last ID
			}
			// Else create a new ID
			return currentID++;
		}

		PxU32 getNumFacets()
		{
			return PxU32(currentID - freeIDCount);
		}
	private:
		PxU8				freeIDs[MaxFacets];
		PxU8				deferredFreeIDs[MaxFacets];
		PxU8				currentID;
		PxU8				freeIDCount;
		PxU8				deferredFreeIDCount;
	};

#if defined(PX_VC) 
    #pragma warning(push)
	#pragma warning( disable : 4324 ) // Padding was added at the end of a structure because of a __declspec(align) value.
#endif
	class EPA
	{
	
	public:

		
		PxGJKStatus PenetrationDepth(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, const Ps::aos::Vec3V* PX_RESTRICT Q, const Ps::aos::Vec3V* PX_RESTRICT A, const Ps::aos::Vec3V* PX_RESTRICT B, const PxI32 size, Ps::aos::Vec3V& pa, Ps::aos::Vec3V& pb, Ps::aos::Vec3V& normal, Ps::aos::FloatV& penDepth, const bool takeCoreShape = false);
		bool expandSegment(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, PxI32& numVerts);
		bool expandTriangle(const ConvexV& a, const ConvexV& b, EPASupportMapPair* pair, PxI32& numVerts);
		
		bool addInitialFacet4();
		bool addInitialFacet5();
		bool expand(const Ps::aos::Vec3VArg q0, const Ps::aos::Vec3VArg q1, const Ps::aos::Vec3VArg q2, EPASupportMapPair* pair, PxI32& numVerts);
		Facet* addFacet(const PxU32 i0, const PxU32 i1, const PxU32 i2, const Ps::aos::FloatVArg lower2, const Ps::aos::FloatVArg upper2);
	
		bool originInTetrahedron(const Ps::aos::Vec3VArg p1, const Ps::aos::Vec3VArg p2, const Ps::aos::Vec3VArg p3, const Ps::aos::Vec3VArg p4);

		BinaryHeap<Facet, MaxFacets> heap;
		Ps::aos::Vec3V aBuf[MaxSupportPoints];
		Ps::aos::Vec3V bBuf[MaxSupportPoints];
		Facet facetBuf[MaxFacets];
		EdgeBuffer edgeBuffer;
		EPAFacetManager facetManager;
	};
#if defined(PX_VC) 
     #pragma warning(pop) 
#endif

	PX_FORCE_INLINE bool EPA::originInTetrahedron(const Ps::aos::Vec3VArg p1, const Ps::aos::Vec3VArg p2, const Ps::aos::Vec3VArg p3, const Ps::aos::Vec3VArg p4)
	{
		using namespace Ps::aos;
		const BoolV bFalse = BFFFF();
		return BAllEq(PointOutsideOfPlane4(p1, p2, p3, p4), bFalse) == 1;
	}

}

}

#endif
