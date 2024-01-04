/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// teamplay_gamerules.cpp
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "skill.h"
#include "game.h"
#include "items.h"
#include "voice_gamemgr.h"
#include "hltv.h"
#include "UserMessages.h"

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

CVoiceGameMgr g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker) override
	{
		if (g_teamplay)
		{
			if (g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE)
			{
				return false;
			}
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	RefreshSkillData();

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if (IS_DEDICATED_SERVER())
	{
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		char* lservercfgfile = (char*)CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && '\0' != lservercfgfile[0])
		{
			char szCommand[256];

			ALERT(at_console, "Executing listen server config file\n");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers()
{
	int num = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pEnt = UTIL_PlayerByIndex(i);

		if (pEnt)
		{
			num = num + 1;
		}
	}

	return num;
}

bool CHalfLifeMultiplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return true;

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData()
{
	// load all default values
	CGameRules::RefreshSkillData();

	// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 999;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 999;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20; // fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME 120
#define MURDERER_WIN 3
#define BYSTANDER_WIN 4

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////

	//	Murder Game Logic
	if (CountPlayers() >= 3 && !m_iInGame) {
		// start a round
		StartRound();
	}



	///// handle win states /////

	// bystander win
	CBasePlayer* pMurderer = (CBasePlayer*)(UTIL_PlayerByIndex(m_iMurderer));
	if (pMurderer && pMurderer->IsPlayer()) {
		if (!pMurderer->IsAlive()) {
			// tell all clients who won
			GoToIntermission(BYSTANDER_WIN);
		}
	}

	// murderer win

	
}


void CHalfLifeMultiplay::StartRound() {
	m_iInGame = false;
	int murderer = g_engfuncs.pfnRandomLong(1, CountPlayers());
	int detective = 2;//g_engfuncs.pfnRandomLong(1, CountPlayers());
	while (detective == murderer) {
		detective = g_engfuncs.pfnRandomLong(1, CountPlayers());
	}
	for (int i = 1; i <= CountPlayers(); i++) {
		CBasePlayer* pPlayer = (CBasePlayer*)(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->IsPlayer()) {
			if (i == murderer) {
				pPlayer->m_iPlayerRole = 1;
				pPlayer->GiveNamedItem("weapon_crowbar");
			} else if (i == detective) {
				pPlayer->m_iPlayerRole = 2;
				pPlayer->GiveNamedItem("weapon_357");
			pPlayer->GiveAmmo(6, "357", _357_MAX_CARRY);
			} else {
				pPlayer->m_iPlayerRole = 0;
			}
			// TODO: Make the player spawn back in somehow??
			MESSAGE_BEGIN(MSG_ONE, gmsgRole, NULL, pPlayer->pev);
			WRITE_BYTE(pPlayer->m_iPlayerRole);
			MESSAGE_END();
		}
	}
	m_iMurderer = murderer;
	m_iInGame = true;
}


//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsMultiplayer()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsDeathmatch()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsCoOp()
{
	return 0 != gpGlobals->coop;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		// that weapon can't deploy anyway.
		return false;
	}

	if (!pPlayer->m_pActiveItem)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveItem->CanHolster())
	{
		// can't put away the active item.
		return false;
	}

	//Never switch
	if (pPlayer->m_iAutoWepSwitch == 0)
	{
		return false;
	}

	//Only switch if not attacking
	if (pPlayer->m_iAutoWepSwitch == 2 && (pPlayer->m_afButtonLast & (IN_ATTACK | IN_ATTACK2)) != 0)
	{
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
	{
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	return true;
}

void CHalfLifeMultiplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(0); // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD(CBasePlayer* pl)
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s has joined the game\n",
											 (!FStringNull(pl->pev->netname) && STRING(pl->pev->netname)[0] != 0) ? STRING(pl->pev->netname) : "unconnected"));



	UTIL_LogPrintf("\"%s<%i><%s><%i>\" entered the game\n",
		STRING(pl->pev->netname),
		GETPLAYERUSERID(pl->edict()),
		GETPLAYERAUTHID(pl->edict()),
		GETPLAYERUSERID(pl->edict()));


	UpdateGameMode(pl);

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
	WRITE_BYTE(ENTINDEX(pl->edict()));
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	MESSAGE_END();

	SendMOTDToClient(pl->edict());

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, NULL, pl->edict());
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected(edict_t* pClient)
{

	if (pClient)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			// team match?
			if (g_teamplay)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"));
			}
			else
			{
				UTIL_LogPrintf("\"%s<%i><%s><%i>\" disconnected\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					GETPLAYERUSERID(pPlayer->edict()));
			}

			pPlayer->RemoveAllItems(true); // destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	return 0;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	if ((pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP)) != 0) {
		if (!m_iInGame) {
			//StartRound();
			pPlayer->Respawn();
		}
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	bool addDefault;
	CBaseEntity* pWeaponEntity = NULL;

	//Ensure the player switches to the Glock on spawn regardless of setting
	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

	pPlayer->SetHasSuit(true);

	addDefault = true;

	while (pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip"))
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = false;
	}

	if (addDefault) // TODO: role based weapons
	{
		if (pPlayer->m_iPlayerRole == 1) {
			pPlayer->GiveNamedItem("weapon_crowbar");
		} else if (pPlayer->m_iPlayerRole == 2) {
			pPlayer->GiveNamedItem("weapon_357");
			pPlayer->GiveAmmo(6, "357", _357_MAX_CARRY);
		}
		
		
	}

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return !m_iInGame;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; //now!
}

bool CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return (aimcrosshair.value != 0);
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 0;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	/*edict_t* pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(pVictim);
	pVictim->StartObserver(pVictim->pev->origin, VARS(pentSpawnSpot)->angles);

	// notify other clients of player switching to spectator mode
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s switched to spectator mode\n",
												(!FStringNull(pVictim->pev->netname) && STRING(pVictim->pev->netname)[0] != 0) ? STRING(pVictim->pev->netname) : "unconnected"));
*/
}

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity* Killer = CBaseEntity::Instance(pKiller);

	const char* killer_weapon_name = "world"; // by default, the player is killed by the world
	int killer_index = 0;

	// Hack to fix name change
	const char* tau = "tau_cannon";
	const char* gluon = "gluon gun";

	if ((pKiller->flags & FL_CLIENT) != 0)
	{
		killer_index = ENTINDEX(ENT(pKiller));

		if (pevInflictor)
		{
			if (pevInflictor == pKiller)
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pKiller);

				if (pPlayer->m_pActiveItem)
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING(pevInflictor->classname); // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING(pevInflictor->classname);
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		killer_weapon_name += 7;
	else if (strncmp(killer_weapon_name, "monster_", 8) == 0)
		killer_weapon_name += 8;
	else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		killer_weapon_name += 5;

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE(killer_index);				// the killer
	WRITE_BYTE(ENTINDEX(pVictim->edict())); // the victim
	WRITE_STRING(killer_weapon_name);		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if (0 == strcmp(killer_weapon_name, "egon"))
		killer_weapon_name = gluon;
	else if (0 == strcmp(killer_weapon_name, "gauss"))
		killer_weapon_name = tau;

	if (pVictim->pev == pKiller)
	{
		// killed self

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
	else if ((pKiller->flags & FL_CLIENT) != 0)
	{
		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(ENT(pKiller)), "model"),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"\n",
				STRING(pKiller->netname),
				GETPLAYERUSERID(ENT(pKiller)),
				GETPLAYERAUTHID(ENT(pKiller)),
				GETPLAYERUSERID(ENT(pKiller)),
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}
	else
	{
		// killed by the world

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pVictim->edict()), "model"),
				killer_weapon_name);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GETPLAYERUSERID(pVictim->edict()),
				killer_weapon_name);
		}
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							 // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				 // player killed
	WRITE_SHORT(ENTINDEX(pVictim->edict())); // index number of primary entity
	if (pevInflictor)
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	else
		WRITE_SHORT(ENTINDEX(ENT(pKiller))); // index number of secondary entity
	WRITE_LONG(7 | DRC_FLAG_DRAMATIC);		 // eventflags (priority and flags)
	MESSAGE_END();

	//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
			/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime(CBasePlayerItem* pWeapon)
{
	if (weaponstay.value > 0)
	{
		// make sure it's only certain weapons
		if ((pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) == 0)
		{
			return gpGlobals->time + 0; // weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE 100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn(CBasePlayerItem* pWeapon)
{
	if (pWeapon && WEAPON_NONE != pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerItem* pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerItem* pWeapon)
{
	if ((pWeapon->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHalfLifeMultiplay::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pItem)
{
	if (weaponstay.value > 0)
	{
		if ((pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
			return CGameRules::CanHavePlayerItem(pPlayer, pItem);

		// check if the player already has this weapon
		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			CBasePlayerItem* it = pPlayer->m_rgpPlayerItems[i];

			while (it != NULL)
			{
				if (it->m_iId == pItem->m_iId)
				{
					return false;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn(CItem* pItem)
{
	return false;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime(CItem* pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	//	if ( pEntity->pev->flags & FL_MONSTER )
	//		return false;

	return true;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if ((pAmmo->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t* CHalfLifeMultiplay::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	edict_t* pentSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);
	if (IsMultiplayer() && !FStringNull(pentSpawnSpot->v.target))
	{
		FireTargets(STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

bool CHalfLifeMultiplay::PlayFootstepSounds(CBasePlayer* pl, float fvol)
{
	if (g_footsteps && g_footsteps->value == 0)
		return false;

	if (pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220)
		return true; // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

bool CHalfLifeMultiplay::FAllowFlashlight()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FAllowMonsters()
{
	return (allowmonsters.value != 0);
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME 6

void CHalfLifeMultiplay::GoToIntermission(int iWinner)
{
	// handle endgame
	for (int i = 1; i <= CountPlayers(); i++) {
		CBasePlayer* pPlayer = (CBasePlayer*)(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->IsPlayer()) {
			MESSAGE_BEGIN(MSG_ONE, gmsgRole, NULL, pPlayer->pev);
			WRITE_BYTE(iWinner);
			MESSAGE_END();
		}
	}
	StartRound();
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s* next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s* items;
	struct mapcycle_item_s* next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle(mapcycle_t* cycle)
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if (p)
	{
		start = p;
		p = p->next;
		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[1500];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char* COM_Parse(char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL; // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while ('\0' != *data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || '\0' == c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
bool COM_TokenWaiting(char* buffer)
{
	char* p;

	p = buffer;
	while ('\0' != *p && *p != '\n')
	{
		if (0 == isspace(*p) || 0 != isalnum(*p))
			return true;

		p++;
	}

	return false;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
bool ReloadMapCycleFile(char* filename, mapcycle_t* cycle)
{
	char szBuffer[MAX_RULE_BUFFER];
	char szMap[32];
	int length;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME(filename, &length);
	bool hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if (pFileList && 0 != length)
	{
		// the first map name in the file becomes the default
		while (true)
		{
			hasbuffer = false;
			memset(szBuffer, 0, MAX_RULE_BUFFER);

			pFileList = COM_Parse(pFileList);
			if (strlen(com_token) <= 0)
				break;

			strcpy(szMap, com_token);

			// Any more tokens on this line?
			if (COM_TokenWaiting(pFileList))
			{
				pFileList = COM_Parse(pFileList);
				if (strlen(com_token) > 0)
				{
					hasbuffer = true;
					strcpy(szBuffer, com_token);
				}
			}

			// Check map
			if (IS_MAP_VALID(szMap))
			{
				// Create entry
				char* s;

				item = new mapcycle_item_s;

				strcpy(item->mapname, szMap);

				item->minplayers = 0;
				item->maxplayers = 0;

				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);

				if (hasbuffer)
				{
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "minplayers");
					if (s && '\0' != s[0])
					{
						item->minplayers = atoi(s);
						item->minplayers = V_max(item->minplayers, 0);
						item->minplayers = V_min(item->minplayers, gpGlobals->maxClients);
					}
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "maxplayers");
					if (s && '\0' != s[0])
					{
						item->maxplayers = atoi(s);
						item->maxplayers = V_max(item->maxplayers, 0);
						item->maxplayers = V_min(item->maxplayers, gpGlobals->maxClients);
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "minplayers");
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "maxplayers");

					strcpy(item->rulebuffer, szBuffer);
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT(at_console, "Skipping %s from mapcycle, not a valid map\n", szMap);
			}
		}

		FREE_FILE(aFileList);
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if (!item)
	{
		return false;
	}

	while (item->next)
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return true;
}



/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString(char* s, char* szCommand)
{
	// Now make rules happen
	char pkey[512];
	char value[512]; // use two buffers so compares
					 // work without stomping on each other
	char* o;

	if (*s == '\\')
		s++;

	while (true)
	{
		o = pkey;
		while (*s != '\\')
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && '\0' != *s)
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat(szCommand, pkey);
		if (strlen(value) > 0)
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}
		strcat(szCommand, "\n");

		if ('\0' == *s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel()
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy(szFirstMapInList, "hldm1"); // the absolute default level is hldm1

	int curplayers;
	bool do_cycle = true;

	// find the map to change to
	char* mapcfile = (char*)CVAR_GET_STRING("mapcyclefile");
	ASSERT(mapcfile != NULL);

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if (stricmp(mapcfile, szPreviousMapCycleFile))
	{
		strcpy(szPreviousMapCycleFile, mapcfile);

		DestroyMapCycle(&mapcycle);

		if (!ReloadMapCycleFile(mapcfile, &mapcycle) || (!mapcycle.items))
		{
			ALERT(at_console, "Unable to load map cycle file %s\n", mapcfile);
			do_cycle = false;
		}
	}

	if (do_cycle && mapcycle.items)
	{
		bool keeplooking = false;
		bool found = false;
		mapcycle_item_s* item;

		// Assume current map
		strcpy(szNextMap, STRING(gpGlobals->mapname));
		strcpy(szFirstMapInList, STRING(gpGlobals->mapname));

		// Traverse list
		for (item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		{
			keeplooking = false;

			ASSERT(item != NULL);

			if (item->minplayers != 0)
			{
				if (curplayers >= item->minplayers)
				{
					found = true;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (item->maxplayers != 0)
			{
				if (curplayers <= item->maxplayers)
				{
					found = true;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (keeplooking)
				continue;

			found = true;
			break;
		}

		if (!found)
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy(szNextMap, item->mapname);

		ExtractCommandString(item->rulebuffer, szCommands);
		strcpy(szRules, item->rulebuffer);
	}

	if (!IS_MAP_VALID(szNextMap))
	{
		strcpy(szNextMap, szFirstMapInList);
	}

	g_fGameOver = true;

	ALERT(at_console, "CHANGE LEVEL: %s\n", szNextMap);
	if (0 != minplayers || 0 != maxplayers)
	{
		ALERT(at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers);
	}
	if (strlen(szRules) > 0)
	{
		ALERT(at_console, "RULES:  %s\n", szRules);
	}

	CHANGE_LEVEL(szNextMap, NULL);
	if (strlen(szCommands) > 0)
	{
		SERVER_COMMAND(szCommands);
	}
	
}

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient(edict_t* client)
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char*)CVAR_GET_STRING("motdfile"), &length);

	// send the server name
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, client);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0; // strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(static_cast<int>('\0' == *pFileList)); // false means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);
}
