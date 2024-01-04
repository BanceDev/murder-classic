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
        m_chPlayerRole = "BYSTANDER";
    } else if (role == 1) {
        m_chPlayerRole = "MURDERER";
    } else if (role == 2) {
        m_chPlayerRole = "DETECTIVE";
    } else if (role == 3) {
        m_chPlayerRole = "MURDERER WINS";
    } else if (role == 4) {
        m_chPlayerRole = "BYSTANDERS WIN";
    }
    m_fFade = 150;
	

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
    sprintf(fragMessage, "%s", m_chPlayerRole);
	y = (ScreenHeight / 2) - gHUD.m_iFontHeight * 5;
	x = ScreenWidth / 2 - (6.5 * strlen(fragMessage))/2;

    gHUD.DrawHudString(x + 2, y-5, x*2, fragMessage, r, g, b);

	return true;
}
