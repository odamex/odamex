// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	PO_MAN.C : Heretic 2 : Raven Software, Corp.
//
//-----------------------------------------------------------------------------

// HEADER FILES ------------------------------------------------------------

#include "odamex.h"

#include "p_local.h"
#include "r_local.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_bbox.h"
#include "tables.h"
#include "s_sndseq.h"

// MACROS ------------------------------------------------------------------

#define PO_MAXPOLYSEGS 64

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

BOOL PO_MovePolyobj(int num, int x, int y);
BOOL PO_RotatePolyobj(int num, angle_t angle);
void PO_Init();

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static polyobj_t *GetPolyobj(int polyNum);
static int GetPolyobjMirror(int poly);
static void UpdateSegBBox(seg_t *seg);
static void RotatePt(int an, fixed_t *x, fixed_t *y, fixed_t startSpotX,
	fixed_t startSpotY);
static void UnLinkPolyobj(polyobj_t *po);
static void LinkPolyobj(polyobj_t *po);
static BOOL CheckMobjBlocking(seg_t *seg, polyobj_t *po);
static void InitBlockMap();
static void IterFindPolySegs(int x, int y, seg_t **segList);
static void SpawnPolyobj(int index, int tag, BOOL crush);
static void TranslateToStartSpot(int tag, int originX, int originY);
static void DoMovePolyobj(polyobj_t *po, int x, int y);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern seg_t *segs;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

polyblock_t **PolyBlockMap;
polyobj_t *polyobjs; // list of all poly-objects on the level
int po_NumPolyobjs;
polyspawns_t *polyspawns; // [RH] Let P_SpawnMapThings() find our thingies for us

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int PolySegCount;
static fixed_t PolyStartX;
static fixed_t PolyStartY;

// CODE --------------------------------------------------------------------

IMPLEMENT_SERIAL (DPolyAction, DThinker)
IMPLEMENT_SERIAL (DRotatePoly, DPolyAction)
IMPLEMENT_SERIAL (DMovePoly, DPolyAction)
IMPLEMENT_SERIAL (DPolyDoor, DMovePoly)

DPolyAction::DPolyAction() { }

void DPolyAction::Serialize(FArchive& arc)
{
	Super::Serialize(arc);
	if (arc.IsStoring())
		arc << m_PolyObj << m_Speed << m_Dist;
	else
		arc >> m_PolyObj >> m_Speed >> m_Dist;
}

DPolyAction::DPolyAction(int polyNum)
{
	m_PolyObj = polyNum;
	m_Speed = 0;
	m_Dist = 0;
}

DRotatePoly::DRotatePoly() { }

void DRotatePoly::Serialize(FArchive& arc)
{
	Super::Serialize(arc);
}

DRotatePoly::DRotatePoly(int polyNum)
	: Super(polyNum)
{
}

DMovePoly::DMovePoly()
{
}

void DMovePoly::Serialize(FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << m_Angle << m_xSpeed << m_ySpeed;
	else
		arc >> m_Angle >> m_xSpeed >> m_ySpeed;
}

DMovePoly::DMovePoly(int polyNum)
	: Super (polyNum)
{
	m_Angle = 0;
	m_xSpeed = 0;
	m_ySpeed = 0;
}

DPolyDoor::DPolyDoor()
{
}

void DPolyDoor::Serialize(FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << m_Direction << m_TotalDist << m_Tics << m_WaitTics << m_Type << m_Close;
	else
		arc >> m_Direction >> m_TotalDist >> m_Tics >> m_WaitTics >> m_Type >> m_Close;
}

DPolyDoor::DPolyDoor(int polyNum, podoortype_t type)
	: Super(polyNum)
{
	m_Type = type;
	m_Direction = 0;
	m_TotalDist = 0;
	m_Tics = 0;
	m_WaitTics = 0;
	m_Close = false;
}

// ===== Polyobj Event Code =====


//
// T_RotatePoly
//
void DRotatePoly::RunThink()
{
	if (PO_RotatePolyobj (m_PolyObj, m_Speed))
	{
		const int absSpeed = abs (m_Speed);

		if (m_Dist == -1)
		{ // perpetual polyobj
			return;
		}
		m_Dist -= absSpeed;
		if (m_Dist <= 0)
		{
			polyobj_t *poly = GetPolyobj (m_PolyObj);
			if (poly->specialdata == this)
			{
				poly->specialdata = NULL;
			}
			SN_StopSequence (poly);
			Destroy ();
		}
		else if (m_Dist < absSpeed)
		{
			m_Speed = m_Dist * (m_Speed < 0 ? -1 : 1);
		}
	}
}

//
// EV_RotatePoly
//
BOOL EV_RotatePoly(line_t *line, int polyNum, int speed, int byteAngle,
					int direction, BOOL overRide)
{
	int mirror;
	polyobj_t *poly;

	if ( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata && !overRide)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_RotatePoly: Invalid polyobj num: %d\n", polyNum);
	}
	DRotatePoly* pe = new DRotatePoly(polyNum);
	if (byteAngle)
	{
		if (byteAngle == 255)
		{
			pe->m_Dist = ~0;
		}
		else
		{
			pe->m_Dist = byteAngle*(ANG90/64); // Angle
		}
	}
	else
	{
		pe->m_Dist = ANG360-1;
	}
	pe->m_Speed = (speed*direction*(ANG90/64))>>3;
	poly->specialdata = pe;
	SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
	
	while ( (mirror = GetPolyobjMirror( polyNum)) )
	{
		poly = GetPolyobj(mirror);
		if (poly && poly->specialdata && !overRide)
		{ // mirroring poly is already in motion
			break;
		}
		pe = new DRotatePoly (mirror);
		poly->specialdata = pe;
		if (byteAngle)
		{
			if (byteAngle == 255)
			{
				pe->m_Dist = ~0;
			}
			else
			{
				pe->m_Dist = byteAngle*(ANG90/64); // Angle
			}
		}
		else
		{
			pe->m_Dist = ANG360-1;
		}
		if( (poly = GetPolyobj(polyNum)) )
		{
			poly->specialdata = pe;
		}
		else
		{
			I_Error ("EV_RotatePoly: Invalid polyobj num: %d\n", polyNum);
		}
		direction = -direction;
		pe->m_Speed = (speed*direction*(ANG90/64))>>3;
		polyNum = mirror;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
	}
	return true;
}

//
// T_MovePoly
//
void DMovePoly::RunThink ()
{
	if (PO_MovePolyobj (m_PolyObj, m_xSpeed, m_ySpeed))
	{
		const int absSpeed = abs (m_Speed);
		m_Dist -= absSpeed;
		if (m_Dist <= 0)
		{
			polyobj_t* poly = GetPolyobj(m_PolyObj);
			if (poly->specialdata == this)
			{
				poly->specialdata = NULL;
			}
			SN_StopSequence (poly);
			Destroy ();
		}
		else if (m_Dist < absSpeed)
		{
			m_Speed = m_Dist * (m_Speed < 0 ? -1 : 1);
			m_xSpeed = FixedMul (m_Speed, finecosine[m_Angle]);
			m_ySpeed = FixedMul (m_Speed, finesine[m_Angle]);
		}
	}
}

//
// EV_MovePoly
//
BOOL EV_MovePoly(line_t *line, int polyNum, int speed, angle_t angle,
				  fixed_t dist, BOOL overRide)
{
	int mirror;
	polyobj_t *poly;

	if ( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata && !overRide)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_MovePoly: Invalid polyobj num: %d\n", polyNum);
	}

	DMovePoly* pe = new DMovePoly(polyNum);
	pe->m_Dist = dist; // Distance
	pe->m_Speed = speed;
	poly->specialdata = pe;

	angle_t an = angle;

	pe->m_Angle = an>>ANGLETOFINESHIFT;
	pe->m_xSpeed = FixedMul (pe->m_Speed, finecosine[pe->m_Angle]);
	pe->m_ySpeed = FixedMul (pe->m_Speed, finesine[pe->m_Angle]);
	SN_StartSequence (poly, poly->seqType, SEQ_DOOR);

	while ( (mirror = GetPolyobjMirror(polyNum)) )
	{
		poly = GetPolyobj(mirror);
		if (poly && poly->specialdata && !overRide)
		{ // mirroring poly is already in motion
			break;
		}
		pe = new DMovePoly (mirror);
		poly->specialdata = pe;
		pe->m_Dist = dist; // Distance
		pe->m_Speed = speed;
		an = an+ANG180; // reverse the angle
		pe->m_Angle = an>>ANGLETOFINESHIFT;
		pe->m_xSpeed = FixedMul (pe->m_Speed, finecosine[pe->m_Angle]);
		pe->m_ySpeed = FixedMul (pe->m_Speed, finesine[pe->m_Angle]);
		polyNum = mirror;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
	}
	return true;
}


//
// T_PolyDoor
//
void DPolyDoor::RunThink ()
{
	int absSpeed;
	polyobj_t *poly;

	if (m_Tics)
	{
		if (!--m_Tics)
		{
			poly = GetPolyobj (m_PolyObj);
			SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
		}
		return;
	}
	switch (m_Type)
	{
	case PODOOR_SLIDE:
		if (m_Dist <= 0 || PO_MovePolyobj (m_PolyObj, m_xSpeed, m_ySpeed))
		{
			absSpeed = abs (m_Speed);
			m_Dist -= absSpeed;
			if (m_Dist <= 0)
			{
				poly = GetPolyobj (m_PolyObj);
				SN_StopSequence (poly);
				if (!m_Close)
				{
					m_Dist = m_TotalDist;
					m_Close = true;
					m_Tics = m_WaitTics;
					m_Direction = (ANG360>>ANGLETOFINESHIFT)-
						m_Direction;
					m_xSpeed = -m_xSpeed;
					m_ySpeed = -m_ySpeed;					
				}
				else
				{
					if (poly->specialdata == this)
					{
						poly->specialdata = NULL;
					}
					Destroy ();
				}
			}
		}
		else
		{
			poly = GetPolyobj (m_PolyObj);
			if (poly->crush || !m_Close)
			{ // continue moving if the poly is a crusher, or is opening
				return;
			}
			else
			{ // open back up
				m_Dist = m_TotalDist - m_Dist;
				m_Direction = (ANG360>>ANGLETOFINESHIFT)-
					m_Direction;
				m_xSpeed = -m_xSpeed;
				m_ySpeed = -m_ySpeed;
				m_Close = false;
				SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
			}
		}
		break;
	case PODOOR_SWING:
		if (PO_RotatePolyobj (m_PolyObj, m_Speed))
		{
			absSpeed = abs (m_Speed);
			if (m_Dist == -1)
			{ // perpetual polyobj
				return;
			}
			m_Dist -= absSpeed;
			if (m_Dist <= 0)
			{
				poly = GetPolyobj (m_PolyObj);
				SN_StopSequence (poly);
				if (!m_Close)
				{
					m_Dist = m_TotalDist;
					m_Close = true;
					m_Tics = m_WaitTics;
					m_Speed = -m_Speed;
				}
				else
				{
					if (poly->specialdata == this)
					{
						poly->specialdata = NULL;
					}
					Destroy ();
				}
			}
		}
		else
		{
			poly = GetPolyobj (m_PolyObj);
			if(poly->crush || !m_Close)
			{ // continue moving if the poly is a crusher, or is opening
				return;
			}
			else
			{ // open back up and rewait
				m_Dist = m_TotalDist - m_Dist;
				m_Speed = -m_Speed;
				m_Close = false;
				SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
			}
		}			
		break;
	default:
		break;
	}
}

//
// EV_OpenPolyDoor
//
BOOL EV_OpenPolyDoor(line_t *line, int polyNum, int speed, angle_t angle,
					  int delay, int distance, podoortype_t type)
{
	int mirror;
	polyobj_t *poly;

	if( (poly = GetPolyobj(polyNum)) )
	{
		if (poly->specialdata)
		{ // poly is already moving
			return false;
		}
	}
	else
	{
		I_Error("EV_OpenPolyDoor: Invalid polyobj num: %d\n", polyNum);
	}
	DPolyDoor* pd = new DPolyDoor(polyNum, type);
	if (type == PODOOR_SLIDE)
	{
		pd->m_WaitTics = delay;
		pd->m_Speed = speed;
		pd->m_Dist = pd->m_TotalDist = distance; // Distance
		pd->m_Direction = angle >> ANGLETOFINESHIFT;
		pd->m_xSpeed = FixedMul (pd->m_Speed, finecosine[pd->m_Direction]);
		pd->m_ySpeed = FixedMul (pd->m_Speed, finesine[pd->m_Direction]);
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
	}
	else if (type == PODOOR_SWING)
	{
		pd->m_WaitTics = delay;
		pd->m_Direction = 1; // ADD:  PODO'OR_SWINGL, PODOOR_SWINGR
		pd->m_Speed = (speed*pd->m_Direction*(ANG90/64))>>3;
		pd->m_Dist = pd->m_TotalDist = angle;
		SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
	}

	poly->specialdata = pd;

	while ( (mirror = GetPolyobjMirror (polyNum)) )
	{
		poly = GetPolyobj (mirror);
		if (poly && poly->specialdata)
		{ // mirroring poly is already in motion
			break;
		}
		pd = new DPolyDoor (mirror, type);
		poly->specialdata = pd;
		if (type == PODOOR_SLIDE)
		{
			pd->m_WaitTics = delay;
			pd->m_Speed = speed;
			pd->m_Dist = pd->m_TotalDist = distance; // Distance
			pd->m_Direction = (angle + ANG180) >> ANGLETOFINESHIFT; // reverse the angle
			pd->m_xSpeed = FixedMul (pd->m_Speed, finecosine[pd->m_Direction]);
			pd->m_ySpeed = FixedMul (pd->m_Speed, finesine[pd->m_Direction]);
			SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
		}
		else if (type == PODOOR_SWING)
		{
			pd->m_WaitTics = delay;
			pd->m_Direction = -1; // ADD:  same as above
			pd->m_Speed = (speed*pd->m_Direction*(ANG90/64))>>3;
			pd->m_Dist = pd->m_TotalDist = angle;
			SN_StartSequence (poly, poly->seqType, SEQ_DOOR);
		}
		polyNum = mirror;
	}
	return true;
}
	
// ===== Higher Level Poly Interface code =====


//
// GetPolyobj
//
static polyobj_t *GetPolyobj (int polyNum)
{
	for (int i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == polyNum)
		{
			return &polyobjs[i];
		}
	}
	return NULL;
}

//
// GetPolyobjMirror
//
static int GetPolyobjMirror(int poly)
{
	for (int i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == poly)
		{
			return (*polyobjs[i].segs)->linedef->args[1];
		}
	}
	return 0;
}

//
// ThrustMobj
//
void ThrustMobj (AActor *actor, seg_t *seg, polyobj_t *po)
{
	if (!(actor->flags&MF_SHOOTABLE) && !actor->player)
	{
		return;
	}

	const int thrustAngle = (seg->angle - ANG90) >> ANGLETOFINESHIFT;
	DPolyAction* pe = static_cast<DPolyAction*>(po->specialdata);
	int force;

	if (pe)
	{
		if (pe->IsKindOf (RUNTIME_CLASS (DRotatePoly)))
		{
			force = pe->m_Speed >> 8;
		}
		else
		{
			force = pe->m_Speed >> 3;
		}
		if (force < FRACUNIT)
		{
			force = FRACUNIT;
		}
		else if (force > 4*FRACUNIT)
		{
			force = 4*FRACUNIT;
		}
	}
	else
	{
		force = FRACUNIT;
	}

	const int thrustX = FixedMul(force, finecosine[thrustAngle]);
	const int thrustY = FixedMul(force, finesine[thrustAngle]);
	actor->momx += thrustX;
	actor->momy += thrustY;
	if (po->crush)
	{
		if (!P_CheckPosition (actor, actor->x + thrustX, actor->y + thrustY))
		{
			P_DamageMobj (actor, NULL, NULL, 3, MOD_CRUSH);
		}
	}
}


//
// UpdateSegBBox
//
static void UpdateSegBBox (seg_t *seg)
{
	line_t* line = seg->linedef;

	if (seg->v1->x < seg->v2->x)
	{
		line->bbox[BOXLEFT] = seg->v1->x;
		line->bbox[BOXRIGHT] = seg->v2->x;
	}
	else
	{
		line->bbox[BOXLEFT] = seg->v2->x;
		line->bbox[BOXRIGHT] = seg->v1->x;
	}
	if (seg->v1->y < seg->v2->y)
	{
		line->bbox[BOXBOTTOM] = seg->v1->y;
		line->bbox[BOXTOP] = seg->v2->y;
	}
	else
	{
		line->bbox[BOXBOTTOM] = seg->v2->y;
		line->bbox[BOXTOP] = seg->v1->y;
	}

	// Update the line's slopetype
	line->dx = line->v2->x - line->v1->x;
	line->dy = line->v2->y - line->v1->y;
	if (!line->dx)
	{
		line->slopetype = ST_VERTICAL;
	}
	else if (!line->dy)
	{
		line->slopetype = ST_HORIZONTAL;
	}
	else
	{
		if (FixedDiv(line->dy, line->dx) > 0)
		{
			line->slopetype = ST_POSITIVE;
		}
		else
		{
			line->slopetype = ST_NEGATIVE;
		}
	}
}


//
// PO_MovePolyobj
//
BOOL PO_MovePolyobj (int num, int x, int y)
{
	polyobj_t *po;

	if (!(po = GetPolyobj (num)))
	{
		I_Error("PO_MovePolyobj: Invalid polyobj number: %d\n", num);
	}

	UnLinkPolyobj(po);
	DoMovePolyobj(po, x, y);

	seg_t** segList = po->segs;
	bool blocked = false;
	for (int count = po->numsegs; count; count--, segList++)
	{
		if (CheckMobjBlocking(*segList, po))
		{
			blocked = true;
			break;
		}
	}
	if (blocked)
	{
		DoMovePolyobj (po, -x, -y);
		LinkPolyobj(po);
		return false;
	}
	po->startSpot[0] += x;
	po->startSpot[1] += y;
	LinkPolyobj (po);
	return true;
}

//
// DoMovePolyobj
//
void DoMovePolyobj (polyobj_t *po, int x, int y)
{
	seg_t **veryTempSeg;

	seg_t** segList = po->segs;
	vertex_t* prevPts = po->prevPts;

	validcount++;
	for (int count = po->numsegs; count; count--, segList++, prevPts++)
	{
		line_t *linedef = (*segList)->linedef;
		if (linedef->validcount != validcount)
		{
			linedef->bbox[BOXTOP] += y;
			linedef->bbox[BOXBOTTOM] += y;
			linedef->bbox[BOXLEFT] += x;
			linedef->bbox[BOXRIGHT] += x;
			//if (linedef->sidenum[0] != -1)
			//	ADecal::MoveChain (sides[linedef->sidenum[0]].BoundActors, x, y);
			//if (linedef->sidenum[1] != -1)
			//	ADecal::MoveChain (sides[linedef->sidenum[1]].BoundActors, x, y);
			linedef->validcount = validcount;
		}
		for (veryTempSeg = po->segs; veryTempSeg != segList;
			veryTempSeg++)
		{
			if ((*veryTempSeg)->v1 == (*segList)->v1)
			{
				break;
			}
		}
		if (veryTempSeg == segList)
		{
			(*segList)->v1->x += x;
			(*segList)->v1->y += y;
		}
		(*prevPts).x += x; // previous points are unique for each seg
		(*prevPts).y += y;
	}
}

//
// RotatePt
//
static void RotatePt(int an, fixed_t *x, fixed_t *y, fixed_t startSpotX, fixed_t startSpotY)
{
	const fixed_t tr_x = *x;
	const fixed_t tr_y = *y;

	fixed_t gxt = FixedMul(tr_x, finecosine[an]);
	fixed_t gyt = FixedMul(tr_y, finesine[an]);
	*x = (gxt-gyt)+startSpotX;

	gxt = FixedMul(tr_x, finesine[an]);
	gyt = FixedMul(tr_y, finecosine[an]);
	*y = (gyt+gxt)+startSpotY;
}

//
// PO_RotatePolyobj
//
BOOL PO_RotatePolyobj(int num, angle_t angle)
{
	int count;
	polyobj_t *po;

	if(!(po = GetPolyobj(num)))
	{
		I_Error("PO_RotatePolyobj: Invalid polyobj number: %d\n", num);
	}
	const int an = (po->angle + angle) >> ANGLETOFINESHIFT;

	UnLinkPolyobj(po);

	seg_t** segList = po->segs;
	vertex_t* originalPts = po->originalPts;
	vertex_t* prevPts = po->prevPts;

	for(count = po->numsegs; count; count--, segList++, originalPts++,
		prevPts++)
	{
		prevPts->x = (*segList)->v1->x;
		prevPts->y = (*segList)->v1->y;
		(*segList)->v1->x = originalPts->x;
		(*segList)->v1->y = originalPts->y;
		RotatePt (an, &(*segList)->v1->x, &(*segList)->v1->y, po->startSpot[0],
			po->startSpot[1]);
	}
	segList = po->segs;
	bool blocked = false;
	validcount++;
	for (count = po->numsegs; count; count--, segList++)
	{
		if (CheckMobjBlocking(*segList, po))
		{
			blocked = true;
		}
		if ((*segList)->linedef->validcount != validcount)
		{
			UpdateSegBBox(*segList);
			line_t *line = (*segList)->linedef;
			//if (line->sidenum[0] != -1)
			//	ADecal::FixForSide (&sides[line->sidenum[0]]);
			//if (line->sidenum[1] != -1)
			//	ADecal::FixForSide (&sides[line->sidenum[1]]);
			line->validcount = validcount;
		}
		(*segList)->angle += angle;
	}
	if (blocked)
	{
		segList = po->segs;
		prevPts = po->prevPts;
		for (count = po->numsegs; count; count--, segList++, prevPts++)
		{
			(*segList)->v1->x = prevPts->x;
			(*segList)->v1->y = prevPts->y;
		}
		segList = po->segs;
		validcount++;
		for (count = po->numsegs; count; count--, segList++, prevPts++)
		{
			if ((*segList)->linedef->validcount != validcount)
			{
				UpdateSegBBox(*segList);
				line_t *line = (*segList)->linedef;
				//if (line->sidenum[0] != -1)
				//	ADecal::FixForSide (&sides[line->sidenum[0]]);
				//if (line->sidenum[1] != -1)
				//	ADecal::FixForSide (&sides[line->sidenum[1]]);
				line->validcount = validcount;
			}
			(*segList)->angle -= angle;
		}
		LinkPolyobj(po);
		return false;
	}
	po->angle += angle;
	LinkPolyobj(po);
	return true;
}

//
// UnLinkPolyobj
//
static void UnLinkPolyobj(polyobj_t *po)
{
	// remove the polyobj from each blockmap section
	for (int j = po->bbox[BOXBOTTOM]; j <= po->bbox[BOXTOP]; j++)
	{
		const int index = j * bmapwidth;
		for(int i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
		{
			if(i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight)
			{
				polyblock_t* link = PolyBlockMap[index + i];
				while(link != NULL && link->polyobj != po)
				{
					link = link->next;
				}
				if(link == NULL)
				{ // polyobj not located in the link cell
					continue;
				}
				link->polyobj = NULL;
			}
		}
	}
}

//
// LinkPolyobj
//
static void LinkPolyobj(polyobj_t *po)
{
	int leftX;
	int bottomY;
	polyblock_t *tempLink;
	int i;

	// calculate the polyobj bbox
	seg_t** tempSeg = po->segs;
	int rightX = leftX = (*tempSeg)->v1->x;
	int topY = bottomY = (*tempSeg)->v1->y;

	for(i = 0; i < po->numsegs; i++, tempSeg++)
	{
		if((*tempSeg)->v1->x > rightX)
		{
			rightX = (*tempSeg)->v1->x;
		}
		if((*tempSeg)->v1->x < leftX)
		{
			leftX = (*tempSeg)->v1->x;
		}
		if((*tempSeg)->v1->y > topY)
		{
			topY = (*tempSeg)->v1->y;
		}
		if((*tempSeg)->v1->y < bottomY)
		{
			bottomY = (*tempSeg)->v1->y;
		}
	}
	po->bbox[BOXRIGHT] = (rightX-bmaporgx)>>MAPBLOCKSHIFT;
	po->bbox[BOXLEFT] = (leftX-bmaporgx)>>MAPBLOCKSHIFT;
	po->bbox[BOXTOP] = (topY-bmaporgy)>>MAPBLOCKSHIFT;
	po->bbox[BOXBOTTOM] = (bottomY-bmaporgy)>>MAPBLOCKSHIFT;
	// add the polyobj to each blockmap section
	for(int j = po->bbox[BOXBOTTOM] * bmapwidth; j <= po->bbox[BOXTOP]*bmapwidth;
		j += bmapwidth)
	{
		for(i = po->bbox[BOXLEFT]; i <= po->bbox[BOXRIGHT]; i++)
		{
			if(i >= 0 && i < bmapwidth && j >= 0 && j < bmapheight*bmapwidth)
			{
				polyblock_t** link = &PolyBlockMap[j + i];
				if(!(*link))
				{ // Create a new link at the current block cell
					*link = (polyblock_t *)Z_Malloc(sizeof(polyblock_t), PU_LEVEL, 0);
					(*link)->next = NULL;
					(*link)->prev = NULL;
					(*link)->polyobj = po;
					continue;
				}
				else
				{
					tempLink = *link;
					while(tempLink->next != NULL && tempLink->polyobj != NULL)
					{
						tempLink = tempLink->next;
					}
				}
				if(tempLink->polyobj == NULL)
				{
					tempLink->polyobj = po;
					continue;
				}
				else
				{
					tempLink->next = (polyblock_t *)Z_Malloc (sizeof(polyblock_t), 
						PU_LEVEL, 0);
					tempLink->next->next = NULL;
					tempLink->next->prev = tempLink;
					tempLink->next->polyobj = po;
				}
			}
			// else, don't link the polyobj, since it's off the map
		}
	}
}

//
// CheckMobjBlocking
//
static BOOL CheckMobjBlocking (seg_t *seg, polyobj_t *po)
{
	int i, j;
	fixed_t tmbbox[4];

	line_t* ld = seg->linedef;

	int top = (ld->bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
	int bottom = (ld->bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
	int left = (ld->bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
	int right = (ld->bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;

	bool blocked = false;

	bottom = bottom < 0 ? 0 : bottom;
	bottom = bottom >= bmapheight ? bmapheight-1 : bottom;
	top = top < 0 ? 0 : top;
	top = top >= bmapheight  ? bmapheight-1 : top;
	left = left < 0 ? 0 : left;
	left = left >= bmapwidth ? bmapwidth-1 : left;
	right = right < 0 ? 0 : right;
	right = right >= bmapwidth ?  bmapwidth-1 : right;

	for (j = bottom*bmapwidth; j <= top*bmapwidth; j += bmapwidth)
	{
		for (i = left; i <= right; i++)
		{
			for (AActor* mobj = blocklinks[j + i]; mobj; mobj = mobj->bmapnode.Next(i, j/bmapwidth))
			{
				if ((mobj->flags&MF_SOLID) && !(mobj->flags&MF_NOCLIP))
				{
					tmbbox[BOXTOP] = mobj->y+mobj->radius;
					tmbbox[BOXBOTTOM] = mobj->y-mobj->radius;
					tmbbox[BOXLEFT] = mobj->x-mobj->radius;
					tmbbox[BOXRIGHT] = mobj->x+mobj->radius;

					if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
						||      tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
						||      tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
						||      tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
					{
						continue;
					}
					if (P_BoxOnLineSide(tmbbox, ld) != -1)
					{
						continue;
					}
					ThrustMobj (mobj, seg, po);
					blocked = true;
				}
			}
		}
	}
	return blocked;
}

//
// InitBlockMap
//
static void InitBlockMap()
{
	PolyBlockMap = (polyblock_t **)Z_Malloc (bmapwidth*bmapheight*sizeof(polyblock_t *),
		PU_LEVEL, 0);
	memset (PolyBlockMap, 0, bmapwidth*bmapheight*sizeof(polyblock_t *));

	for (int i = 0; i < po_NumPolyobjs; i++)
	{
		LinkPolyobj(&polyobjs[i]);
	}
}

//
// IterFindPolySegs
//
//              Passing NULL for segList will cause IterFindPolySegs to
//      count the number of segs in the polyobj
static void IterFindPolySegs(int x, int y, seg_t **segList)
{
	if (x == PolyStartX && y == PolyStartY)
	{
		return;
	}
	for (int i = 0; i < numsegs; i++)
	{
		if (segs[i].v1->x == x && segs[i].v1->y == y)
		{
			if(!segList)
			{
				PolySegCount++;
			}
			else
			{
				*segList++ = &segs[i];
			}
			IterFindPolySegs(segs[i].v2->x, segs[i].v2->y, segList);
			return;
		}
	}
	I_Error("IterFindPolySegs: Non-closed Polyobj located.\n");
}

//
// SpawnPolyobj
//
static void SpawnPolyobj (int index, int tag, BOOL crush)
{
	int i;

	for (i = 0; i < numsegs; i++)
	{
		if (segs[i].linedef->special == PO_LINE_START &&
			segs[i].linedef->args[0] == tag)
		{
			if (polyobjs[index].segs)
			{
				I_Error ("SpawnPolyobj: Polyobj %d already spawned.\n", tag);
			}
			segs[i].linedef->special = 0;
			segs[i].linedef->args[0] = 0;
			PolySegCount = 1;
			PolyStartX = segs[i].v1->x;
			PolyStartY = segs[i].v1->y;
			IterFindPolySegs(segs[i].v2->x, segs[i].v2->y, NULL);

			polyobjs[index].numsegs = PolySegCount;
			polyobjs[index].segs = (seg_t **)Z_Malloc (PolySegCount*sizeof(seg_t *),
				PU_LEVEL, 0);
			polyobjs[index].segs[0] = &segs[i]; // insert the first seg
			IterFindPolySegs (segs[i].v2->x, segs[i].v2->y,
				polyobjs[index].segs+1);
			polyobjs[index].crush = crush;
			polyobjs[index].tag = tag;
			polyobjs[index].seqType = segs[i].linedef->args[2];
			if (polyobjs[index].seqType < 0 || polyobjs[index].seqType > 63)
			{
				polyobjs[index].seqType = 0;
			}
			break;
		}
	}
	if (!polyobjs[index].segs)
	{
		seg_t *polySegList[PO_MAXPOLYSEGS];

		// didn't find a polyobj through PO_LINE_START
		int psIndex = 0;
		polyobjs[index].numsegs = 0;
		for (int j = 1; j < PO_MAXPOLYSEGS; j++)
		{
			const int psIndexOld = psIndex;
			for (i = 0; i < numsegs; i++)
			{
				if (segs[i].linedef->special == PO_LINE_EXPLICIT &&
					segs[i].linedef->args[0] == tag)
				{
					if (!segs[i].linedef->args[1])
					{
						I_Error ("SpawnPolyobj: Explicit line missing order number (probably %d) in poly %d.\n",
							j+1, tag);
					}
					if (segs[i].linedef->args[1] == j)
					{
						polySegList[psIndex] = &segs[i];
						polyobjs[index].numsegs++;
						psIndex++;
						if (psIndex > PO_MAXPOLYSEGS)
						{
							I_Error ("SpawnPolyobj: psIndex > PO_MAXPOLYSEGS\n");
						}
					}
				}
			}
			// Clear out any specials for these segs...we cannot clear them out
			// 	in the above loop, since we aren't guaranteed one seg per
			//		linedef.
			for (i = 0; i < numsegs; i++)
			{
				if (segs[i].linedef->special == PO_LINE_EXPLICIT &&
					segs[i].linedef->args[0] == tag && segs[i].linedef->args[1] == j)
				{
					segs[i].linedef->special = 0;
					segs[i].linedef->args[0] = 0;
				}
			}
			if (psIndex == psIndexOld)
			{ // Check if an explicit line order has been skipped
				// A line has been skipped if there are any more explicit
				// lines with the current tag value
				for (i = 0; i < numsegs; i++)
				{
					if(segs[i].linedef->special == PO_LINE_EXPLICIT &&
						segs[i].linedef->args[0] == tag)
					{
						I_Error ("SpawnPolyobj: Missing explicit line %d for poly %d\n",
							j, tag);
					}
				}
			}
		}
		if (polyobjs[index].numsegs)
		{
			PolySegCount = polyobjs[index].numsegs; // PolySegCount used globally
			polyobjs[index].crush = crush;
			polyobjs[index].tag = tag;
			polyobjs[index].segs = (seg_t **)Z_Malloc (polyobjs[index].numsegs
				*sizeof(seg_t *), PU_LEVEL, 0);
			for (i = 0; i < polyobjs[index].numsegs; i++)
			{
				polyobjs[index].segs[i] = polySegList[i];
			}
			polyobjs[index].seqType = (*polyobjs[index].segs)->linedef->args[3];
			// Next, change the polyobj's first line to point to a mirror
			//		if it exists
			(*polyobjs[index].segs)->linedef->args[1] =
				(*polyobjs[index].segs)->linedef->args[2];
		}
		else
			I_Error ("SpawnPolyobj: Poly %d does not exist\n", tag);
	}
}

//
// TranslateToStartSpot
//
static void TranslateToStartSpot (int tag, int originX, int originY)
{
	seg_t **veryTempSeg;
	vertex_t avg; // used to find a polyobj's center, and hence subsector
	int i;

	polyobj_t* po = NULL;
	for (i = 0; i < po_NumPolyobjs; i++)
	{
		if (polyobjs[i].tag == tag)
		{
			po = &polyobjs[i];
			break;
		}
	}
	if (po == NULL)
	{ // didn't match the tag with a polyobj tag
		I_Error("TranslateToStartSpot: Unable to match polyobj tag: %d\n",
			tag);
	}
	if (po->segs == NULL)
	{
		I_Error ("TranslateToStartSpot: Anchor point located without a StartSpot point: %d\n", tag);
	}
	po->originalPts = (vertex_t *)Z_Malloc(po->numsegs*sizeof(vertex_t), PU_LEVEL, 0);
	po->prevPts = (vertex_t *)Z_Malloc(po->numsegs*sizeof(vertex_t), PU_LEVEL, 0);
	const int deltaX = originX - po->startSpot[0];
	const int deltaY = originY - po->startSpot[1];

	seg_t** tempSeg = po->segs;
	vertex_t* tempPt = po->originalPts;
	avg.x = 0;
	avg.y = 0;

	validcount++;
	for (i = 0; i < po->numsegs; i++, tempSeg++, tempPt++)
	{
		if ((*tempSeg)->linedef->validcount != validcount)
		{
			(*tempSeg)->linedef->bbox[BOXTOP] -= deltaY;
			(*tempSeg)->linedef->bbox[BOXBOTTOM] -= deltaY;
			(*tempSeg)->linedef->bbox[BOXLEFT] -= deltaX;
			(*tempSeg)->linedef->bbox[BOXRIGHT] -= deltaX;
			(*tempSeg)->linedef->validcount = validcount;
		}
		for (veryTempSeg = po->segs; veryTempSeg != tempSeg; veryTempSeg++)
		{
			if((*veryTempSeg)->v1 == (*tempSeg)->v1)
			{
				break;
			}
		}
		if (veryTempSeg == tempSeg)
		{ // the point hasn't been translated, yet
			(*tempSeg)->v1->x -= deltaX;
			(*tempSeg)->v1->y -= deltaY;
		}
		avg.x += (*tempSeg)->v1->x>>FRACBITS;
		avg.y += (*tempSeg)->v1->y>>FRACBITS;
		// the original Pts are based off the startSpot Pt, and are
		// unique to each seg, not each linedef
		tempPt->x = (*tempSeg)->v1->x-po->startSpot[0];
		tempPt->y = (*tempSeg)->v1->y-po->startSpot[1];
	}
	avg.x /= po->numsegs;
	avg.y /= po->numsegs;
	subsector_t* sub = P_PointInSubsector(avg.x << FRACBITS, avg.y << FRACBITS);
	if (sub->poly != NULL)
	{
		I_Error ("PO_TranslateToStartSpot: Multiple polyobjs in a single subsector.\n");
	}
	sub->poly = po;
}

//
// PO_Init
//
void PO_Init()
{
	// [RH] Hexen found the polyobject-related things by reloading the map's
	//		THINGS lump here and scanning through it. I have P_SpawnMapThing()
	//		record those things instead, so that in here, we simply need to
	//		look at the polyspawns list.
	polyspawns_t *polyspawn, **prev;

	polyobjs = (polyobj_t *)Z_Malloc (po_NumPolyobjs*sizeof(polyobj_t), PU_LEVEL, 0);
	memset (polyobjs, 0, po_NumPolyobjs*sizeof(polyobj_t));

	int polyIndex = 0; // index polyobj number
	// Find the startSpot points, and spawn each polyobj
	for (polyspawn = polyspawns, prev = &polyspawns; polyspawn;)
	{
		// 9301 (3001) = no crush, 9302 (3002) = crushing
		if (polyspawn->type == PO_SPAWN_TYPE || polyspawn->type == PO_SPAWNCRUSH_TYPE)
		{ // Polyobj StartSpot Pt.
			polyobjs[polyIndex].startSpot[0] = polyspawn->x;
			polyobjs[polyIndex].startSpot[1] = polyspawn->y;
			SpawnPolyobj(polyIndex, polyspawn->angle, (polyspawn->type == PO_SPAWNCRUSH_TYPE));
			polyIndex++;
			*prev = polyspawn->next;
			delete polyspawn;
			polyspawn = *prev;
		} else {
			prev = &polyspawn->next;
			polyspawn = polyspawn->next;
		}
	}
	for (polyspawn = polyspawns; polyspawn;)
	{
		polyspawns_t *next = polyspawn->next;
		if (polyspawn->type == PO_ANCHOR_TYPE)
		{ // Polyobj Anchor Pt.
			TranslateToStartSpot (polyspawn->angle, polyspawn->x, polyspawn->y);
		}
		delete polyspawn;
		polyspawn = next;
	}
	polyspawns = NULL;

	// check for a startspot without an anchor point
	for (polyIndex = 0; polyIndex < po_NumPolyobjs; polyIndex++)
	{
		if (!polyobjs[polyIndex].originalPts)
		{
			I_Error ("PO_Init: StartSpot located without an Anchor point: %d\n",
				polyobjs[polyIndex].tag);
		}
	}
	InitBlockMap();
}

//
// PO_Busy
//
BOOL PO_Busy (int polyobj)
{
	polyobj_t *poly;

	poly = GetPolyobj (polyobj);
	if (!poly->specialdata)
	{
		return false;
	}
	else
	{
		return true;
	}
}

VERSION_CONTROL (po_man_cpp, "$Id$")
