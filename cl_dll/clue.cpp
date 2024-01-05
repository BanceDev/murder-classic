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
// clue.cpp
//
// implementation of CHudClue class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"


DECLARE_MESSAGE(m_Clue, Clue);

bool CHudClue::Init()
{
	m_iFlags = 0;

	HOOK_MESSAGE(Clue);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudClue::VidInit()
{
	return true;
}

bool CHudClue::MsgFunc_Clue(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int clue = READ_BYTE();
    
    m_fFade = 150;
    m_iClues = clue;
	

	return true;
}


bool CHudClue::Draw(float flTime)
{
	int r, g, b, a, x, y;

	r = 255;
    g = 255;
    b = 255;

    if (0 != m_fFade)
    {
        if (m_fFade > 150)
            m_fFade = 150;

        m_fFade -= (gHUD.m_flTimeDelta * 20);
        if (m_fFade <= 0)
        {
            a = 255;
            m_fFade = 0;
        }
        a = (m_fFade / 150) * 255;
    }
    else
        a = 0;

    ScaleColors(r, g, b, a);

    char fragMessage[256];
    sprintf(fragMessage, "%d/5 clues", m_iClues);
	y = (ScreenHeight / 2) - gHUD.m_iFontHeight * 5;
	x = ScreenWidth / 2 - (8 * strlen(fragMessage))/2;

    gHUD.DrawHudString(x + 2, y-5, x*2, fragMessage, r, g, b);

	return true;
}
