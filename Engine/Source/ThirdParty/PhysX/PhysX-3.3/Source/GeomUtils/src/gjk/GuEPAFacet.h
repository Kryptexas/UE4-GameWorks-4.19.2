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

#ifndef	GU_EPA_FACET_H
#define	GU_EPA_FACET_H

#include "CmPhysXCommon.h"
#include "PsVecMath.h"
#include "PsFPU.h"
#include "PsUtilities.h"

#if defined __SPU__ || (defined __GNUC__ && defined _DEBUG)
#define PX_EPA_FORCE_INLINE
#else
#define PX_EPA_FORCE_INLINE PX_FORCE_INLINE
#endif

namespace physx
{

//#define MaxEdges 128
#define MaxEdges 32

namespace Gu
{
	const PxU32 lookUp[3] = {1, 2, 0};
	
	PX_FORCE_INLINE PxU32 incMod3(PxU32 i) { return lookUp[i]; } 
	
	class EdgeBuffer;
	class Edge;
	class EPAFacetManager;

	class Facet
	{
	public:
	
		Facet()
		{
		}

		PX_FORCE_INLINE Facet(const PxU32 _i0, const PxU32 _i1, const PxU32 _i2)
			:  m_UDist(0),  m_lambda1(0.f), m_lambda2(0.f), m_obsolete(false), m_inHeap(false)  //, m_recipDet(0.f)
		{
			m_indices[0]= Ps::toI8(_i0);
			m_indices[1]= Ps::toI8(_i1);
			m_indices[2]= Ps::toI8(_i2);
			 
			m_adjFacets[0] = m_adjFacets[1] = m_adjFacets[2] = NULL;
			m_adjEdges[0] = m_adjEdges[1] = m_adjEdges[2] = -1;
		}


		PX_FORCE_INLINE void invalidate()
		{
			m_adjFacets[0] = m_adjFacets[1] = m_adjFacets[2] = NULL;
			m_adjEdges[0] = m_adjEdges[1] = m_adjEdges[2] = -1;
		}

		PX_FORCE_INLINE bool Valid()
		{
			return (m_adjFacets[0] != NULL) & (m_adjFacets[1] != NULL) & (m_adjFacets[2] != NULL);
		}

		PX_FORCE_INLINE PxU32 operator[](const PxU32 i) const 
		{ 
			return (PxU32)m_indices[i]; 
		} 

		bool link(const PxU32 edge0, Facet* PX_RESTRICT  facet, const PxU32 edge1);
		//void link(const PxU32 edge0, Facet* PX_RESTRICT  facet, const PxU32 edge1);

		PX_FORCE_INLINE bool isObsolete() const { return m_obsolete; }

		PX_FORCE_INLINE Ps::aos::Vec3V getClosest() const 
		{ 
			return Ps::aos::V3LoadA(&m_closest.x); 
		}

		PX_EPA_FORCE_INLINE Ps::aos::BoolV isValid2(const PxU32 i0, const PxU32 i1, const PxU32 i2, const Ps::aos::Vec3V* PX_RESTRICT aBuf, const Ps::aos::Vec3V* PX_RESTRICT bBuf, 
			const Ps::aos::FloatVArg lower, const Ps::aos::FloatVArg upper, PxI32& b1);

		PX_FORCE_INLINE Ps::aos::FloatV getDist() const 
		{ 
			return Ps::aos::FLoad(PX_FR(m_UDist));
		}

		PX_FORCE_INLINE Ps::aos::FloatV getPlaneDist() const 
		{ 
			return Ps::aos::FLoad(m_planeD);
		}

		PX_FORCE_INLINE Ps::aos::Vec3V getPlaneNormal()const
		{
			return Ps::aos::V3LoadA(&m_planeN.x); 
		}

		void getClosestPoint(const Ps::aos::Vec3V* PX_RESTRICT aBuf, const Ps::aos::Vec3V* PX_RESTRICT bBuf, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB) const;
		
		void silhouette(const Ps::aos::Vec3VArg w, EdgeBuffer& edgeBuffer, EPAFacetManager& manager);
		
		bool operator <(const Facet& b) const
		{
			return m_UDist < b.m_UDist;
		}
		bool operator > (const Facet& b) const
		{
			return m_UDist > b.m_UDist;
		}
		bool operator <=(const Facet& b) const
		{
			return m_UDist <= b.m_UDist;
		}
		bool operator >=(const Facet& b) const
		{
			return m_UDist >= b.m_UDist;
		}
	 
		PX_FORCE_INLINE void silhouette(const PxU32 index, const Ps::aos::Vec3VArg w, EdgeBuffer& edgeBuffer, EPAFacetManager& manager);

		
		
		//don't change the variable order, otherwise, m_planeN might not be 16 byte align and the loading will got problem
		PX_ALIGN(16, PxVec3 m_closest);																										//12
		PxU32 m_UDist;																														//16
		PxVec3 m_planeN;																													//28
		PxF32 m_planeD;																														//32
		
		PxF32 m_lambda1;																													//36
		PxF32 m_lambda2;																													//40
		
		Facet* PX_RESTRICT m_adjFacets[3]; //the triangle adjacent to edge i in this triangle												//52
		PxI8 m_adjEdges[3]; //the edge connected with the corresponding triangle															//55
		PxI8 m_indices[3]; //the index of vertices of the triangle																			//58
		bool m_obsolete; //a flag to denote whether the triangle is visible from the new support point										//59
		bool m_inHeap;																														//60
		PxU8 m_FacetId;																														//61																									//73

	};

	class Edge 
	{
	public:
		PX_FORCE_INLINE Edge() {}
		PX_FORCE_INLINE Edge(Facet * PX_RESTRICT facet, const PxU32 index) : m_facet(facet), m_index(index) {}
		PX_FORCE_INLINE Edge(const Edge& other) : m_facet(other.m_facet), m_index(other.m_index){}

		PX_FORCE_INLINE Edge& operator = (const Edge& other)
		{
			m_facet = other.m_facet;
			m_index = other.m_index;
			return *this;
		}

		PX_FORCE_INLINE Facet *getFacet() const { return m_facet; }
		PX_FORCE_INLINE PxU32 getIndex() const { return m_index; }

		PX_FORCE_INLINE PxU32 getSource() const
		{
			PX_ASSERT(m_index < 3);
			return (*m_facet)[m_index];
		}

		PX_FORCE_INLINE PxU32 getTarget() const
		{
			PX_ASSERT(m_index < 3);
			return (*m_facet)[incMod3(m_index)];
		}

		Facet* PX_RESTRICT m_facet;
		PxU32 m_index;
	};


	class EdgeBuffer
	{
	public:
		EdgeBuffer() : m_Size(0)
		{
		}

		Edge* Insert(const Edge& edge)
		{
			PX_ASSERT(m_Size < MaxEdges);
			Edge* PX_RESTRICT pEdge = &m_pEdges[m_Size++];
			*pEdge = edge;
			return pEdge;
		}

		Edge* Insert(Facet* PX_RESTRICT  facet, const PxU32 index)
		{
			PX_ASSERT(m_Size < MaxEdges);
			Edge* pEdge = &m_pEdges[m_Size++];
			pEdge->m_facet=facet;
			pEdge->m_index=index;
			return pEdge;
		}

		Edge* Get(const PxU32 index)
		{
			PX_ASSERT(index < m_Size);
			return &m_pEdges[index];
		}

		PxU32 Size()
		{
			return m_Size;
		}

		bool IsEmpty()
		{
			return m_Size == 0;
		}

		void MakeEmpty()
		{
			m_Size = 0;
		}

		Edge m_pEdges[MaxEdges];
		PxU32 m_Size;
	};


	PX_FORCE_INLINE void Facet::getClosestPoint(const Ps::aos::Vec3V* PX_RESTRICT aBuf, const Ps::aos::Vec3V* PX_RESTRICT bBuf, Ps::aos::Vec3V& closestA, Ps::aos::Vec3V& closestB) const 
	{
		using namespace Ps::aos;

		const FloatV lambda1 = FLoad(m_lambda1);
		const FloatV lambda2 = FLoad(m_lambda2);
		
		const Vec3V pa0(aBuf[m_indices[0]]);
		const Vec3V pa1(aBuf[m_indices[1]]);
		const Vec3V pa2(aBuf[m_indices[2]]);

		const Vec3V pb0(bBuf[m_indices[0]]);
		const Vec3V pb1(bBuf[m_indices[1]]);
		const Vec3V pb2(bBuf[m_indices[2]]);


		const Vec3V a0 = V3Scale(V3Sub(pa1, pa0), lambda1);
		const Vec3V a1 = V3Scale(V3Sub(pa2, pa0), lambda2);
		const Vec3V b0 = V3Scale(V3Sub(pb1, pb0), lambda1);
		const Vec3V b1 = V3Scale(V3Sub(pb2, pb0), lambda2);
		closestA = V3Add(V3Add(a0, a1), pa0);
		closestB = V3Add(V3Add(b0, b1), pb0);
	}

	PX_FORCE_INLINE bool Facet::link(const PxU32 edge0, Facet * PX_RESTRICT facet, const PxU32 edge1) 
	{
		m_adjFacets[edge0] = facet;
		m_adjEdges[edge0] = Ps::toI8(edge1);
		facet->m_adjFacets[edge1] = this;
		facet->m_adjEdges[edge1] = Ps::toI8(edge0);

		return (m_indices[edge0] == facet->m_indices[incMod3(edge1)]) && (m_indices[incMod3(edge0)] == facet->m_indices[edge1]);
	} 

}

}

#endif
