//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include "track.h"
#include "ccinfo.h"
#include "midictrl.h"

CCInfo::CCInfo(Track* t, int port, int chan, int control, int cc, int rec, int toggle)
{
	m_track = t;
	m_port = port;
	m_channel = chan;
	m_control = control;
	m_ccnum = cc;
	m_reconly = rec;
	m_faketoggle = toggle;
	m_nrpn = false;
	m_msb = 0;
	m_lsb = 0;
}

CCInfo::CCInfo()
{
	m_track = 0;
	m_port = 0;
	m_channel = 0;
	m_control = 0;
	m_ccnum = 0;
	m_reconly = 0;
	m_faketoggle = 0;
	m_nrpn = false;
	m_msb = 0;
	m_lsb = 0;
}

CCInfo::CCInfo(const CCInfo& i)
 : QObject(0)
,m_track(i.m_track)
,m_port(i.m_port)
,m_channel(i.m_channel)
,m_control(i.m_control)
,m_ccnum(i.m_ccnum)
,m_reconly(i.m_reconly)
,m_faketoggle(i.m_faketoggle)
,m_nrpn(i.m_nrpn)
,m_msb(i.m_msb)
,m_lsb(i.m_lsb)
{
}

CCInfo::~CCInfo()
{
}

int CCInfo::assignedControl()
{ 
	if(nrpn()) 
	{
		if(m_msb >= 0 && m_lsb >= 0)
		{
			int val = CTRL_NRPN_OFFSET | (m_msb << 8) | m_lsb;
			return val;
		}
		else
			return -1;
	}
	else
		return m_ccnum;
}
