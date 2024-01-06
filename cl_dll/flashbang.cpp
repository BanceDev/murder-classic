/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// flashbang.cpp
//
// implementation of CHudFlashbang class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"


DECLARE_MESSAGE(m_Flashbang, Flashbang);

bool CHudFlashbang::Init()
{
	m_iFlags = 0;

	HOOK_MESSAGE(Flashbang);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudFlashbang::VidInit()
{
	return true;
}

bool CHudFlashbang::MsgFunc_Flashbang(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int flash = READ_BYTE();

    //make sure message is legit
    if (flash == 1) {
        m_iIsFlashed = true;
    }

    m_fFade = 100;
	

	return true;
}


bool CHudFlashbang::Draw(float flTime)
{
	int r, g, b, a, x, y;

	r = 255;
    g = 255;
    b = 255;

    if (0 != m_fFade)
    {
        if (m_fFade > 100)
            m_fFade = 100;

        m_fFade -= (gHUD.m_flTimeDelta * 20);
        if (m_fFade <= 0)
        {
            a = 0;
            m_fFade = 0;
        }
        if (m_fFade > 30) {
            a = (m_fFade / 30) * 255;
        } else {
            a = 255;
        }
        
    }
    else {
        a = 0;
        m_iIsFlashed = false;
    }

    if (m_iIsFlashed) {
        FillRGBA(0, 0, ScreenWidth, ScreenHeight, r, g, b, a);
    }
    

	return true;
}
