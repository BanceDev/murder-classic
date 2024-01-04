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
// frag.cpp
//
// implementation of CHudFrag class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"


DECLARE_MESSAGE(m_Role, Role);

bool CHudRole::Init()
{
	m_iFlags = 0;

	HOOK_MESSAGE(Role);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudRole::VidInit()
{
	return true;
}

bool CHudRole::MsgFunc_Role(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int role = READ_BYTE();

    if (role == 0) {
        m_chPlayerRole = "bystander.";
    } else if (role == 1) {
        m_chPlayerRole = "murderer.";
    } else {
        m_chPlayerRole = "detective.";
    }
	

	return true;
}


bool CHudRole::Draw(float flTime)
{
	int r, g, b, a, x, y;

	r = 255;
    g = 255;
    b = 255;

    if (0 != m_fFade)
    {
        if (m_fFade > 40)
            m_fFade = 40;

        m_fFade -= (gHUD.m_flTimeDelta * 20);
        if (m_fFade <= 0)
        {
            a = 255;
            m_fFade = 0;
        }
        if (m_fFade <= 10)
            a = (m_fFade / 10) * 255;
        else
            a = 255;
    }
    else
        a = 0;

    ScaleColors(r, g, b, a);

    char fragMessage[256];
    sprintf(fragMessage, "You are a %s", m_chPlayerRole);
	y = (ScreenHeight / 2) - gHUD.m_iFontHeight * 5;
	x = ScreenWidth / 2 - (6.5 * strlen(fragMessage))/2;

    gHUD.DrawHudString(x + 2, y-5, x*2, fragMessage, r, g, b);

	return true;
}
