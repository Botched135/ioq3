/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_main.c $
 *
 *****************************************************************************/


#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"		//bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_vcmd.h"

//
#include "chars.h"
#include "inv.h"
#include "syn.h"

char pipeName[45];

//bot states
bot_state_t	*botstates[MAX_CLIENTS];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;
//
int bot_interbreed;
int bot_interbreedmatchcount;
//
vmCvar_t bot_thinktime;
vmCvar_t bot_memorydump;
vmCvar_t bot_saveroutingcache;
vmCvar_t bot_pause;
vmCvar_t bot_report;
vmCvar_t bot_testsolid;
vmCvar_t bot_testclusters;
vmCvar_t bot_developer;
vmCvar_t bot_interbreedchar;
vmCvar_t bot_interbreedbots;
vmCvar_t bot_interbreedcycle;
vmCvar_t bot_interbreedwrite;

//ADAM Variables
int fitnessSent;

void ExitLevel( void );


/*
==================
BotAI_Print
==================
*/
void QDECL BotAI_Print(int type, char *fmt, ...) {
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			G_Printf("BotPrint: %s", str);
			break;
		}
		case PRT_WARNING: {
			G_Printf( S_COLOR_YELLOW "Warning: %s", str );
			break;
		}
		case PRT_ERROR: {
			G_Printf( S_COLOR_RED "Error: %s", str );
			break;
		}
		case PRT_FATAL: {
			G_Printf( S_COLOR_RED "Fatal: %s", str );
			break;
		}
		case PRT_EXIT: {
			G_Error( S_COLOR_RED "Exit: %s", str );
			break;
		}
		default: {
			G_Printf( "unknown print type\n" );
			break;
		}
	}
}


/*
==================
BotAI_Trace
==================
*/
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) {
	trace_t trace;

	trap_Trace(&trace, start, mins, maxs, end, passent, contentmask);
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = 0;
	bsptrace->surface.flags = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof(playerState_t) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof(entityState_t) );
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy( state, &ent->s, sizeof(entityState_t) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int		entNum;

	entNum = trap_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==================
BotAI_BotInitialChat
==================
*/
void QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... ) {
	int		i, mcontext;
	va_list	ap;
	char	*p;
	char	*vars[MAX_MATCHVARIABLES];

	memset(vars, 0, sizeof(vars));
	va_start(ap, type);
	p = va_arg(ap, char *);
	for (i = 0; i < MAX_MATCHVARIABLES; i++) {
		if( !p ) {
			break;
		}
		vars[i] = p;
		p = va_arg(ap, char *);
	}
	va_end(ap);

	mcontext = BotSynonymContext(bs);

	trap_BotInitialChat( bs->cs, type, mcontext, vars[0], vars[1], vars[2], vars[3], vars[4], vars[5], vars[6], vars[7] );
}


/*
==================
BotTestAAS
==================
*/
void BotTestAAS(vec3_t origin) {
	int areanum;
	aas_areainfo_t info;

	trap_Cvar_Update(&bot_testsolid);
	trap_Cvar_Update(&bot_testclusters);
	if (bot_testsolid.integer) {
		if (!trap_AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (areanum) BotAI_Print(PRT_MESSAGE, "\rempty area");
		else BotAI_Print(PRT_MESSAGE, "\r^1SOLID area");
	}
	else if (bot_testclusters.integer) {
		if (!trap_AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (!areanum)
			BotAI_Print(PRT_MESSAGE, "\r^1Solid!                              ");
		else {
			trap_AAS_AreaInfo(areanum, &info);
			BotAI_Print(PRT_MESSAGE, "\rarea %d, cluster %d       ", areanum, info.cluster);
		}
	}
}

/*
==================
BotReportStatus
==================
*/
void BotReportStatus(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char *leader, flagstatus[32];
	//
	ClientName(bs->client, netname, sizeof(netname));
	if (Q_stricmp(netname, bs->teamleader) == 0) leader = "L";
	else leader = " ";

	strcpy(flagstatus, "  ");
	if (gametype == GT_CTF) {
		if (BotCTFCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}
#ifdef MISSIONPACK
	else if (gametype == GT_1FCTF) {
		if (Bot1FCTFCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}
	else if (gametype == GT_HARVESTER) {
		if (BotHarvesterCarryingCubes(bs)) {
			if (BotTeam(bs) == TEAM_RED) Com_sprintf(flagstatus, sizeof(flagstatus), S_COLOR_RED"%2d", bs->inventory[INVENTORY_REDCUBE]);
			else Com_sprintf(flagstatus, sizeof(flagstatus), S_COLOR_BLUE"%2d", bs->inventory[INVENTORY_BLUECUBE]);
		}
	}
#endif

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: helping %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: accompanying %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: defending %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: getting item %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: killing %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: camping\n", netname, leader, flagstatus);
			break;
		}
		case LTG_PATROL:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: patrolling\n", netname, leader, flagstatus);
			break;
		}
		case LTG_GETFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: capturing flag\n", netname, leader, flagstatus);
			break;
		}
		case LTG_RUSHBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: rushing base\n", netname, leader, flagstatus);
			break;
		}
		case LTG_RETURNFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: returning flag\n", netname, leader, flagstatus);
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: attacking the enemy base\n", netname, leader, flagstatus);
			break;
		}
		case LTG_HARVEST:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: harvesting\n", netname, leader, flagstatus);
			break;
		}
		default:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: roaming\n", netname, leader, flagstatus);
			break;
		}
	}
}

/*
==================
BotTeamplayReport
==================
*/
void BotTeamplayReport(void) {
	int i;
	char buf[MAX_INFO_STRING];

	BotAI_Print(PRT_MESSAGE, S_COLOR_RED"RED\n");
	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_RED) {
			BotReportStatus(botstates[i]);
		}
	}
	BotAI_Print(PRT_MESSAGE, S_COLOR_BLUE"BLUE\n");
	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_BLUE) {
			BotReportStatus(botstates[i]);
		}
	}
}

/*
==================
BotSetInfoConfigString
==================
*/
void BotSetInfoConfigString(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char action[MAX_MESSAGE_SIZE];
	char *leader, carrying[32], *cs;
	bot_goal_t goal;
	//
	ClientName(bs->client, netname, sizeof(netname));
	if (Q_stricmp(netname, bs->teamleader) == 0) leader = "L";
	else leader = " ";

	strcpy(carrying, "  ");
	if (gametype == GT_CTF) {
		if (BotCTFCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}
#ifdef MISSIONPACK
	else if (gametype == GT_1FCTF) {
		if (Bot1FCTFCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}
	else if (gametype == GT_HARVESTER) {
		if (BotHarvesterCarryingCubes(bs)) {
			if (BotTeam(bs) == TEAM_RED) Com_sprintf(carrying, sizeof(carrying), "%2d", bs->inventory[INVENTORY_REDCUBE]);
			else Com_sprintf(carrying, sizeof(carrying), "%2d", bs->inventory[INVENTORY_BLUECUBE]);
		}
	}
#endif

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "helping %s", goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "accompanying %s", goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "defending %s", goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "getting item %s", goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "killing %s", goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			Com_sprintf(action, sizeof(action), "camping");
			break;
		}
		case LTG_PATROL:
		{
			Com_sprintf(action, sizeof(action), "patrolling");
			break;
		}
		case LTG_GETFLAG:
		{
			Com_sprintf(action, sizeof(action), "capturing flag");
			break;
		}
		case LTG_RUSHBASE:
		{
			Com_sprintf(action, sizeof(action), "rushing base");
			break;
		}
		case LTG_RETURNFLAG:
		{
			Com_sprintf(action, sizeof(action), "returning flag");
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			Com_sprintf(action, sizeof(action), "attacking the enemy base");
			break;
		}
		case LTG_HARVEST:
		{
			Com_sprintf(action, sizeof(action), "harvesting");
			break;
		}
		default:
		{
			trap_BotGetTopGoal(bs->gs, &goal);
			trap_BotGoalName(goal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "roaming %s", goalname);
			break;
		}
	}
  	cs = va("l\\%s\\c\\%s\\a\\%s",
				leader,
				carrying,
				action);
  	trap_SetConfigstring (CS_BOTINFO + bs->client, cs);
}

/*
==============
BotUpdateInfoConfigStrings
==============
*/
void BotUpdateInfoConfigStrings(void) {
	int i;
	char buf[MAX_INFO_STRING];

	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse )
			continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
			continue;
		BotSetInfoConfigString(botstates[i]);
	}
}

/*
==============
BotInterbreedBots
==============
*/
void BotInterbreedBots(void) {
	float ranks[MAX_CLIENTS];
	int parent1, parent2, child;
	int i;

	// get rankings for all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			ranks[i] = botstates[i]->num_kills * 2 - botstates[i]->num_deaths;
		}
		else {
			ranks[i] = -1;
		}
	}

	if (trap_GeneticParentsAndChildSelection(MAX_CLIENTS, ranks, &parent1, &parent2, &child)) {
		trap_BotInterbreedGoalFuzzyLogic(botstates[parent1]->gs, botstates[parent2]->gs, botstates[child]->gs);
		trap_BotMutateGoalFuzzyLogic(botstates[child]->gs, 1);
	}
	// reset the kills and deaths
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			botstates[i]->num_kills = 0;
			botstates[i]->num_deaths = 0;
		}
	}
}

/*
==============
BotWriteInterbreeded
==============
*/
void BotWriteInterbreeded(char *filename) {
	float rank, bestrank;
	int i, bestbot;

	bestrank = 0;
	bestbot = -1;
	// get the best bot
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			rank = botstates[i]->num_kills * 2 - botstates[i]->num_deaths;
		}
		else {
			rank = -1;
		}
		if (rank > bestrank) {
			bestrank = rank;
			bestbot = i;
		}
	}
	if (bestbot >= 0) {
		//write out the new goal fuzzy logic
		trap_BotSaveGoalFuzzyLogic(botstates[bestbot]->gs, filename);
	}
}

/*
==============
BotInterbreedEndMatch

add link back into ExitLevel?
==============
*/
void BotInterbreedEndMatch(void) {

	if (!bot_interbreed) return;
	bot_interbreedmatchcount++;
	if (bot_interbreedmatchcount >= bot_interbreedcycle.integer) {
		bot_interbreedmatchcount = 0;
		//
		trap_Cvar_Update(&bot_interbreedwrite);
		if (strlen(bot_interbreedwrite.string)) {
			BotWriteInterbreeded(bot_interbreedwrite.string);
			trap_Cvar_Set("bot_interbreedwrite", "");
		}
		BotInterbreedBots();
	}
}

/*
==============
BotInterbreeding
==============
*/
void BotInterbreeding(void) {
	int i;

	trap_Cvar_Update(&bot_interbreedchar);
	if (!strlen(bot_interbreedchar.string)) return;
	//make sure we are in tournament mode
	if (gametype != GT_TOURNAMENT) {
		trap_Cvar_Set("g_gametype", va("%d", GT_TOURNAMENT));
		ExitLevel();
		return;
	}
	//shutdown all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAIShutdownClient(botstates[i]->client, qfalse);
		}
	}
	//make sure all item weight configs are reloaded and Not shared
	trap_BotLibVarSet("bot_reloadcharacters", "1");
	//add a number of bots using the desired bot character
	for (i = 0; i < bot_interbreedbots.integer; i++) {
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s 4 free %i %s%d\n",
						bot_interbreedchar.string, i * 50, bot_interbreedchar.string, i) );
	}
	//
	trap_Cvar_Set("bot_interbreedchar", "");
	bot_interbreed = qtrue;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(int entnum, aas_entityinfo_t *info) {
	trap_AAS_EntityInfo(entnum, info);
}

/*
==============
NumBots
==============
*/
int NumBots(void) {
	return numbots;
}

/*
==============
BotTeamLeader
==============
*/
int BotTeamLeader(bot_state_t *bs) {
	int leader;

	leader = ClientFromName(bs->teamleader);
	if (leader < 0) return qfalse;
	if (!botstates[leader] || !botstates[leader]->inuse) return qfalse;
	return qtrue;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0) diff -= 360.0;
	}
	else {
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if (ideal_angle > angle) {
		if (move > 180.0) move -= 360.0;
	}
	else {
		if (move < -180.0) move += 360.0;
	}
	if (move > 0) {
		if (move > speed) move = speed;
	}
	else {
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
	//
	if (bs->enemy >= 0) {
		factor = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1);
		maxchange = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1, 1800);
	}
	else {
		factor = 0.05f;
		maxchange = 360;
	}
	if (maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for (i = 0; i < 2; i++) {
		//
		if (bot_challenge.integer) {
			//smooth slowdown view model
			diff = fabs(AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]));
			anglespeed = diff * factor;
			if (anglespeed > maxchange) anglespeed = maxchange;
			bs->viewangles[i] = BotChangeViewAngle(bs->viewangles[i],
											bs->ideal_viewangles[i], anglespeed);
		}
		else {
			//over reaction view model
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
			diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
			disired_speed = diff * factor;
			bs->viewanglespeed[i] += (bs->viewanglespeed[i] - disired_speed);
			if (bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
			if (bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
			anglespeed = bs->viewanglespeed[i];
			if (anglespeed > maxchange) anglespeed = maxchange;
			if (anglespeed < -maxchange) anglespeed = -maxchange;
			bs->viewangles[i] += anglespeed;
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			//demping
			bs->viewanglespeed[i] *= 0.45 * (1 - factor);
		}
		//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", bs->ideal_viewangles[0], bs->ideal_viewangles[1], bs->ideal_viewangles[2]);`
		//bs->viewangles[i] = bs->ideal_viewangles[i];
	}
	//bs->viewangles[PITCH] = 0;
	if (bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	//elementary action: view
	trap_EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_TALK) ucmd->buttons |= BUTTON_TALK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;
	if (bi->actionflags & ACTION_AFFIRMATIVE) ucmd->buttons |= BUTTON_AFFIRMATIVE;
	if (bi->actionflags & ACTION_NEGATIVE) ucmd->buttons |= BUTTON_NEGATIVE;
	if (bi->actionflags & ACTION_GETFLAG) ucmd->buttons |= BUTTON_GETFLAG;
	if (bi->actionflags & ACTION_GUARDBASE) ucmd->buttons |= BUTTON_GUARDBASE;
	if (bi->actionflags & ACTION_PATROL) ucmd->buttons |= BUTTON_PATROL;
	if (bi->actionflags & ACTION_FOLLOWME) ucmd->buttons |= BUTTON_FOLLOWME;
	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		/*NOTE: disabled because temp should be mod first
		if ( j == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) temp = 16000;
			else if ( temp < -16000 ) temp = -16000;
		}
		*/
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	f = DotProduct(forward, bi->dir);
	r = DotProduct(right, bi->dir);
	u = fabs(forward[2]) * bi->dir[2];
	m = fabs(f);

	if (fabs(r) > m) {
		m = fabs(r);
	}

	if (fabs(u) > m) {
		m = fabs(u);
	}

	if (m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;

	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//change the bot view angles
	BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	//retrieve the bot input
	trap_EA_GetInput(bs->client, (float) time / 1000, &bi);
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN|ACTION_ATTACK);
	}
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate(void) {
	if (regularupdate_time < FloatTime()) {
		trap_BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (Q_IsColorString(&text[i])) {
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
==============
BotAI
==============
*/
int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j, skillAdaptation, currentSkill;

	trap_EA_ResetInput(client);
	//
	bs = botstates[client];
	if (!bs || !bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	//retrieve the current client state
	if (!BotAI_GetClientState(client, &bs->cur_ps)) {
		BotAI_Print(PRT_FATAL, "BotAI: failed to get player state for player %d\n", client);
		return qfalse;
	}
	//retrieve any waiting server commands
	while( trap_BotGetServerCommand(client, buf, sizeof(buf)) ) {
		//have buf point to the command and args to the command arguments
		args = strchr( buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences( args );

		if (!Q_stricmp(buf, "cp "))
			{ /*CenterPrintf*/ }
		else if (!Q_stricmp(buf, "cs"))
			{ /*ConfigStringModified*/ }
		else if (!Q_stricmp(buf, "print")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_NORMAL, args);
		}
		else if (!Q_stricmp(buf, "chat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
		else if (!Q_stricmp(buf, "tchat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
#ifdef MISSIONPACK
		else if (!Q_stricmp(buf, "vchat")) {
			BotVoiceChatCommand(bs, SAY_ALL, args);
		}
		else if (!Q_stricmp(buf, "vtchat")) {
			BotVoiceChatCommand(bs, SAY_TEAM, args);
		}
		else if (!Q_stricmp(buf, "vtell")) {
			BotVoiceChatCommand(bs, SAY_TELL, args);
		}
#endif
		else if (!Q_stricmp(buf, "scores"))
			{ /*FIXME: parse scores?*/ }
		else if (!Q_stricmp(buf, "clientLevelShot"))
			{ /*ignore*/ }
	}
	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);
	//eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
	//get the area the bot is in
	bs->areanum = BotPointAreaNum(bs->origin);
	//the real AI
	BotDeathmatchAI(bs, thinktime);
	
	#ifdef ADAPTATION
	if(bs->debugTime < FloatTime() && bs->setupcount <= 0)
	{
		currentSkill = bs->settings.skill;
		skillAdaptation = currentSkill+bs->skillAdaptation;
		if(skillAdaptation > 5 || skillAdaptation < 1)
			skillAdaptation = currentSkill;
		
		if(skillAdaptation != currentSkill)
		{
			G_Printf("Debug time: %f\n",bs->debugTime);
			AdaptiveUpdate(bs,skillAdaptation);// whatever skill needed
		}
		bs->debugTime = FloatTime()+ADAPT_INTERVAL;
		G_Printf("Debug time: %f\n",bs->debugTime);
		
	}
	
	#endif
	//set the weapon selection every AI frame
	//trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	trap_EA_SelectWeapon(bs->client, WP_MACHINEGUN);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	Add_Ammo(&g_entities[bs->entitynum],WEAPONINDEX_MACHINEGUN,100);
	//everything was ok
	return qtrue;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = bot_thinktime.integer * botnum / numbots;
		botnum++;
	}
}

/*
==============
BotWriteSessionData
==============
*/
void BotWriteSessionData(bot_state_t *bs) {
	const char	*s;
	const char	*var;

	s = va(
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f"
			" %f",
		bs->lastgoal_decisionmaker,
		bs->lastgoal_ltgtype,
		bs->lastgoal_teammate,
		bs->lastgoal_teamgoal.areanum,
		bs->lastgoal_teamgoal.entitynum,
		bs->lastgoal_teamgoal.flags,
		bs->lastgoal_teamgoal.iteminfo,
		bs->lastgoal_teamgoal.number,
		bs->lastgoal_teamgoal.origin[0],
		bs->lastgoal_teamgoal.origin[1],
		bs->lastgoal_teamgoal.origin[2],
		bs->lastgoal_teamgoal.mins[0],
		bs->lastgoal_teamgoal.mins[1],
		bs->lastgoal_teamgoal.mins[2],
		bs->lastgoal_teamgoal.maxs[0],
		bs->lastgoal_teamgoal.maxs[1],
		bs->lastgoal_teamgoal.maxs[2],
		bs->formation_dist
		);

	var = va( "botsession%i", bs->client );

	trap_Cvar_Set( var, s );
}

/*
==============
BotReadSessionData
==============
*/
void BotReadSessionData(bot_state_t *bs) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	var = va( "botsession%i", bs->client );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf(s,
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f"
			" %f",
		&bs->lastgoal_decisionmaker,
		&bs->lastgoal_ltgtype,
		&bs->lastgoal_teammate,
		&bs->lastgoal_teamgoal.areanum,
		&bs->lastgoal_teamgoal.entitynum,
		&bs->lastgoal_teamgoal.flags,
		&bs->lastgoal_teamgoal.iteminfo,
		&bs->lastgoal_teamgoal.number,
		&bs->lastgoal_teamgoal.origin[0],
		&bs->lastgoal_teamgoal.origin[1],
		&bs->lastgoal_teamgoal.origin[2],
		&bs->lastgoal_teamgoal.mins[0],
		&bs->lastgoal_teamgoal.mins[1],
		&bs->lastgoal_teamgoal.mins[2],
		&bs->lastgoal_teamgoal.maxs[0],
		&bs->lastgoal_teamgoal.maxs[1],
		&bs->lastgoal_teamgoal.maxs[2],
		&bs->formation_dist
		);
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart) {
	char filename[144], name[144], gender[144];
	bot_state_t *bs;
	int errnum;

	if (!botstates[client]) botstates[client] = G_Alloc(sizeof(bot_state_t));
	bs = botstates[client];

	if (!bs) {
		return qfalse;
	}

	if (bs && bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	if (!trap_AAS_Initialized()) {
		BotAI_Print(PRT_FATAL, "AAS not initialized\n");
		return qfalse;
	}

	//load the bot character
	bs->character = trap_BotLoadCharacter(settings->characterfile, settings->skill);
	if (!bs->character) {
		BotAI_Print(PRT_FATAL, "couldn't load skill %f from %s\n", settings->skill, settings->characterfile);
		return qfalse;
	}
	//copy the settings
	memcpy(&bs->settings, settings, sizeof(bot_settings_t));
	//allocate a goal state
	bs->gs = trap_BotAllocGoalState(client);
	//load the item weights
	trap_Characteristic_String(bs->character, CHARACTERISTIC_ITEMWEIGHTS, filename, sizeof(filename));
	errnum = trap_BotLoadItemWeights(bs->gs, filename);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeGoalState(bs->gs);
		return qfalse;
	}
	//allocate a weapon state
	bs->ws = trap_BotAllocWeaponState();
	//load the weapon weights
	trap_Characteristic_String(bs->character, CHARACTERISTIC_WEAPONWEIGHTS, filename, sizeof(filename));
	errnum = trap_BotLoadWeaponWeights(bs->ws, filename);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeGoalState(bs->gs);
		trap_BotFreeWeaponState(bs->ws);
		return qfalse;
	}
	//allocate a chat state
	bs->cs = trap_BotAllocChatState();
	//load the chat file
	trap_Characteristic_String(bs->character, CHARACTERISTIC_CHAT_FILE, filename, sizeof(filename));
	trap_Characteristic_String(bs->character, CHARACTERISTIC_CHAT_NAME, name, sizeof(name));
	errnum = trap_BotLoadChatFile(bs->cs, filename, name);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeChatState(bs->cs);
		trap_BotFreeGoalState(bs->gs);
		trap_BotFreeWeaponState(bs->ws);
		return qfalse;
	}
	//get the gender characteristic
	trap_Characteristic_String(bs->character, CHARACTERISTIC_GENDER, gender, sizeof(gender));
	//set the chat gender
	if (*gender == 'f' || *gender == 'F') trap_BotSetChatGender(bs->cs, CHAT_GENDERFEMALE);
	else if (*gender == 'm' || *gender == 'M') trap_BotSetChatGender(bs->cs, CHAT_GENDERMALE);
	else trap_BotSetChatGender(bs->cs, CHAT_GENDERLESS);

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap_BotAllocMoveState();
	bs->walker = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_WALKER, 0, 1);

	numbots++;

	if (trap_Cvar_VariableIntegerValue("bot_testichat")) {
		trap_BotLibVarSet("bot_testichat", "1");
		BotChatTest(bs);
	}
	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	//if interbreeding start with a mutation
	if (bot_interbreed) {
		trap_BotMutateGoalFuzzyLogic(bs->gs, 1);
	}
	// if we kept the bot client
	if (restart) {
		BotReadSessionData(bs);
	}
	//bot has been setup successfully
	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(int client, qboolean restart) {
	bot_state_t *bs;

	bs = botstates[client];
	if (!bs || !bs->inuse) {
		//BotAI_Print(PRT_ERROR, "BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	if (restart) {
		BotWriteSessionData(bs);
	}

	if (BotChat_ExitGame(bs)) {
		trap_BotEnterChat(bs->cs, bs->client, CHAT_ALL);
	}

	trap_BotFreeMoveState(bs->ms);
	//free the goal state
	trap_BotFreeGoalState(bs->gs);
	//free the chat file
	trap_BotFreeChatState(bs->cs);
	//free the weapon weights
	trap_BotFreeWeaponState(bs->ws);
	//free the bot character
	trap_BotFreeCharacter(bs->character);
	//
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//clear activate goal stack
	BotClearActivateGoalStack(bs);
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState(bot_state_t *bs) {
	int client, entitynum, inuse, adamFlag;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;					//current player state
	float entergame_time;

	
	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));

	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = bs->character;
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	adamFlag = bs->adamFlag;	
	
	//free checkpoints and patrol points
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->character = character;
	bs->entergame_time = entergame_time;
	bs->adamFlag = adamFlag;
	//reset several states
	if (bs->ms) trap_BotResetMoveState(bs->ms);
	if (bs->gs) trap_BotResetGoalState(bs->gs);
	if (bs->ws) trap_BotResetWeaponState(bs->ws);
	if (bs->gs) trap_BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap_BotResetAvoidReach(bs->ms);
}

void AdaptiveUpdate(bot_state_t* bs, float skill)
{
	int client, entitynum, inuse, adamFlag, enemy;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;					//current player state
	float entergame_time;

	
	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	
	G_Printf("Bot previous skill: %f\n",settings.skill);
	settings.skill = skill;
	enemy = bs->enemy;
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = trap_BotLoadCharacter(settings.characterfile, settings.skill);
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	adamFlag = bs->adamFlag;	
	
	//free checkpoints and patrol points
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->enemy = enemy;
	bs->character = character;
	bs->entergame_time = entergame_time;
	bs->adamFlag = adamFlag;
	G_Printf("Bot current skill: %f\n",settings.skill);
	//reset several states
	if (bs->ms) trap_BotResetMoveState(bs->ms);
	if (bs->gs) trap_BotResetGoalState(bs->gs);
	if (bs->ws) trap_BotResetWeaponState(bs->ws);
	if (bs->gs) trap_BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap_BotResetAvoidReach(bs->ms);
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int			i;
	vmCvar_t	mapname;

	if (!restart) {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap_BotLibLoadMap( mapname.string );
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	BotSetupDeathmatchAI();

	return qtrue;
}

#ifdef MISSIONPACK
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace );
#endif

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame(int time) {
	gentity_t	*ent;
	bot_entitystate_t state;
	int elapsed_time, thinktime,i;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;
	#ifdef ADAM_ACTIVE 
	// FOR ADAM
	int adaptiveAgents,pipeIn,pipeOut,shotsHits;
	char  neatOutput[385],pausing[2], finish[2];
	int AdamAgentIndices[ADAM_AGENTS];
	float neatInput[MAX_CLIENTS][ADAM_NN_INPUT];
	// FINAL NUMBER IS DEFINED BY HOW MANY ACTIONS IT CAN TAKE
	float fitnessOutput[MAX_CLIENTS][ADAM_NN_FITNESS];
	#elif defined(ADAPTATION)
	int pipeIn,pipeOut;
	int adaptiveUpdate;
	#endif
	float neatActions[MAX_CLIENTS][ADAM_NN_OUTPUT];
	float tempBattleFrames;
	
	
	G_CheckBotSpawn();
	
	#ifdef ADAM_ACTIVE
		adaptiveAgents = GetAdaptiveAgents(botstates,AdamAgentIndices);
		if(adaptiveAgents)
		{
	#endif
	#if defined(ADAM_ACTIVE) || defined(ADAPTATION)
			if(strlen(pipeName) == 0)
			{
				trap_Adam_Com_Get_PipeName(pipeName);
				G_Printf("Pipename in AI_MAIN: %s\n",pipeName);
			}
			
			// Informing trainer that this server is ready
			pipeOut = trap_Adam_Com_Open_Pipe(pipeName,0);
			trap_Adam_Com_Write_Ready(pipeOut);
			trap_Adam_Com_Close_Pipe(pipeOut);
			adaptiveUpdate = 0;
	#endif
	#ifdef ADAM_ACTIVE
				// Check if it needs to pause for a new generation
			pipeIn = trap_Adam_Com_Open_Pipe(pipeName,1);
			trap_Adam_Com_Read_Pause(pipeIn,pausing);
			trap_Adam_Com_Close_Pipe(pipeIn);
	
		}
		if(pausing[0] == 'p')
			trap_Cvar_Set("bot_pause","1");
		else
			trap_Cvar_Set("bot_pause","0");
	#endif

	trap_Cvar_Update(&bot_rocketjump);
	trap_Cvar_Update(&bot_grapple);
	trap_Cvar_Update(&bot_fastchat);
	trap_Cvar_Update(&bot_nochat);
	trap_Cvar_Update(&bot_testrchat);
	trap_Cvar_Update(&bot_thinktime);
	trap_Cvar_Update(&bot_memorydump);
	trap_Cvar_Update(&bot_saveroutingcache);
	trap_Cvar_Update(&bot_pause);
	trap_Cvar_Update(&bot_report);

	//G_Printf("Botpause is: %i \n",bot_pause.integer);
	if (bot_report.integer) {
//		BotTeamplayReport();
//		trap_Cvar_Set("bot_report", "0");
		BotUpdateInfoConfigStrings();
	}

	if (bot_pause.integer) {
		G_Printf("Pausing \n");
		// execute bot user commands every frame
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if( !botstates[i] || !botstates[i]->inuse ) {
				continue;
			}
			if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			

			botstates[i]->lastucmd.forwardmove = 0;
			botstates[i]->lastucmd.rightmove = 0;
			botstates[i]->lastucmd.upmove = 0;
			botstates[i]->lastucmd.buttons = 0;
			botstates[i]->lastucmd.serverTime = time;
			#if defined(ADAM_TRAINING) && defined(ADAM_ACTIVE)
				fitnessOutput[i][0] = 0;
				//RESET THE BOT
				if(botstates[i]->adamFlag & (ADAM_ADAPTIVE | ADAM_RESET))
				{
					// So basically, what is important?
					// Accuracy --> should not reward for JUST accuracy but also promote shooting
					// --> but lets try it first anyway
					// Amount of kills
					// Avoid dying 
					// Gather fitness values
					Adam_CalcFitnessForFrame(botstates[i]);
					tempBattleFrames = botstates[i]->frameInBattle > 0 ? botstates[i]->frameInBattle/300.0f : 1.0f;
					shotsHits = botstates[i]->lasthitcount-botstates[i]->lastGenerationShotHit;
					fitnessOutput[i][0] = 2;
					fitnessOutput[i][1] = botstates[i]->fitnessScore;
					// Accuracy
					/*
					fitnessOutput[i][1] = botstates[i]->shotsTaken > 0 ? 
										(1.0f*shotsHits)
										/(botstates[i]->shotsTaken*0.75f) : 0;
					
					fitnessOutput[i][1]*= (shotsHits*0.4f);
					*/

					// Movement
					//G_Printf("frames on Target: %d\n",botstates[i]->framesOnTarget);
					fitnessOutput[i][2] =0.0f;//botstates[i]->framesOnTarget*0.05f;// 0.0f;//;//botstates[i]->num_kills;
					// Deaths
					fitnessOutput[i][3] = 0.0f;//botstates[i]->moveFaliures*0.005f;
					botstates[i]->num_deaths = 0;
					botstates[i]->num_kills = 0;
					botstates[i]->timesHit = 0;
					botstates[i]->framesOnTarget =0;
					botstates[i]->lastGenerationShotHit = botstates[i]->cur_ps.persistant[PERS_HITS];
					//trap_EA_Respawn(i);
					botstates[i]->adamFlag = ADAM_ADAPTIVE;
					botstates[i]->shotsTaken = 0;
					botstates[i]->moveFaliures =0;
					botstates[i]->frameInBattle = 0;
					botstates[i]->fitnessScore = 0;
					g_entities[botstates[i]->entitynum].health = 0;
					VectorClear(botstates[i]->lastMove);
					AdamEnter_Respawn(botstates[i]);
					
				}
				
			#endif
			trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
		}
		#if defined(ADAM_TRAINING) && defined(ADAM_ACTIVE)
			if(adaptiveAgents)
			{
				//Send Fitness data
				if(fitnessSent == 2)
				{
					pipeOut = trap_Adam_Com_Open_Pipe(pipeName,0);
					trap_Adam_Com_Write_Fitness(pipeOut,fitnessOutput,adaptiveAgents);
					trap_Adam_Com_Close_Pipe(pipeOut);

					pipeIn = trap_Adam_Com_Open_Pipe(pipeName,1);
					trap_Adam_Com_Read_Finish(pipeIn,finish);
					trap_Adam_Com_Close_Pipe(pipeIn);
					fitnessSent = 0;
				}
			}
		#endif
		return qtrue;
	}

	for( i = 0; i < MAX_CLIENTS; i++ ) 
	{
			if( !botstates[i] || !botstates[i]->inuse )
				continue;
			if( g_entities[i].client->pers.connected != CON_CONNECTED )
				continue;
		if(botstates[i]->adamFlag & ADAM_ADAPTIVE)
			botstates[i]->adamFlag |= ADAM_RESET;
	}
	

	if (bot_memorydump.integer) {
		trap_BotLibVarSet("memorydump", "1");
		trap_Cvar_Set("bot_memorydump", "0");
	}
	if (bot_saveroutingcache.integer) {
		trap_BotLibVarSet("saveroutingcache", "1");
		trap_Cvar_Set("bot_saveroutingcache", "0");
	}
	//check if bot interbreeding is activated
	BotInterbreeding();
	//cap the bot think time
	if (bot_thinktime.integer > 200) {
		trap_Cvar_Set("bot_thinktime", "200");
	}
	//if the bot think time changed we should reschedule the bots
	if (bot_thinktime.integer != lastbotthink_time) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if (elapsed_time > bot_thinktime.integer) thinktime = elapsed_time;
	else thinktime = bot_thinktime.integer;

	// update the bot library
	if ( botlib_residual >= thinktime ) {
		botlib_residual -= thinktime;

		trap_BotLibStartFrame((float) time / 1000);

		if (!trap_AAS_Initialized()) return qfalse;

		//update entities in the botlib
		for (i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if (!ent->inuse) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (!ent->r.linked) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (ent->r.svFlags & SVF_NOCLIENT) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update missiles
			if (ent->s.eType == ET_MISSILE && ent->s.weapon != WP_GRAPPLING_HOOK) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update event only entities
			if (ent->s.eType > ET_EVENTS) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
#ifdef MISSIONPACK
			// never link prox mine triggers
			if (ent->r.contents == CONTENTS_TRIGGER) {
				if (ent->touch == ProximityMine_Trigger) {
					trap_BotLibUpdateEntity(i, NULL);
					continue;
				}
			}
#endif
			//
			memset(&state, 0, sizeof(bot_entitystate_t));
			//
			VectorCopy(ent->r.currentOrigin, state.origin);
			if (i < MAX_CLIENTS) {
				VectorCopy(ent->s.apos.trBase, state.angles);
			} else {
				VectorCopy(ent->r.currentAngles, state.angles);
			}
			VectorCopy(ent->s.origin2, state.old_origin);
			VectorCopy(ent->r.mins, state.mins);
			VectorCopy(ent->r.maxs, state.maxs);
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if (ent->r.bmodel) state.solid = SOLID_BSP;
			else state.solid = SOLID_BBOX;
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.event = ent->s.event;
			state.eventParm = ent->s.eventParm;
			state.powerups = ent->s.powerups;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
			state.weapon = ent->s.weapon;
			//
			trap_BotLibUpdateEntity(i, &state);
		}

		BotAIRegularUpdate();
	}

	floattime = trap_AAS_Time();
	//Collect data here.
	#ifdef ADAM_ACTIVE
		if(adaptiveAgents)
		{
			// WRITE DATA
			BotStateToNEAT(neatInput,botstates);

			pipeOut = trap_Adam_Com_Open_Pipe(pipeName,0);
			trap_Adam_Com_Write_Neat(pipeOut,neatInput,adaptiveAgents);
			trap_Adam_Com_Close_Pipe(pipeOut);

			// READ DATA
			pipeIn = trap_Adam_Com_Open_Pipe(pipeName,1);
			trap_Adam_Com_Read_Neat(pipeIn,neatOutput,adaptiveAgents);
			trap_Adam_Com_Close_Pipe(pipeIn);
			// TRANSLATE STRING DATA TO FLOAT ARRAY
			if(strlen(neatOutput) >0)
			{
				//G_Printf("Before conversion to actions\n");
				trap_Adam_Com_Array_To_Action(neatActions,AdamAgentIndices,neatOutput);
				//G_Printf("After conversion to actions\n");
				//G_Printf("First:%.2f, Second: %.2f, Third: %.2f\n",neatActions[0][0],neatActions[1][0],neatActions[2][0]);
			}
				
		}
	#elif defined(ADAPTATION)
	// HERE WE SEND DATA FROM THE PYTHON CODE TO ADAPT THE BOTS
	pipeIn = trap_Adam_Com_Open_Pipe(pipeName,1);
	trap_Adam_Com_Read_Adaptation(pipeIn,&adaptiveUpdate);
	trap_Adam_Com_Close_Pipe(pipeIn);
	#endif
	// execute scheduled bot AI
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//
		#ifdef ADAPTATION
		botstates[i]->skillAdaptation = adaptiveUpdate;
		#endif
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			if (!trap_AAS_Initialized()) return qfalse;

			if (g_entities[i].client->pers.connected == CON_CONNECTED) 
			{
				if(botstates[i]->adamFlag & ADAM_ADAPTIVE)
					BotAdamAgent(i,(float)thinktime/1000,neatActions[i]);
				else
					BotAI(i, (float) thinktime / 1000);
			}
		}
	}
	fitnessSent = 2;
	// execute bot user commands every frame
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}
	return qtrue;
}

/*
==============
BotInitLibrary
==============
*/
int BotInitLibrary(void) {
	char buf[144];

	//set the maxclients and maxentities library variables before calling BotSetupLibrary
	Com_sprintf(buf, sizeof(buf), "%d", level.maxclients);
	trap_BotLibVarSet("maxclients", buf);
	Com_sprintf(buf, sizeof(buf), "%d", MAX_GENTITIES);
	trap_BotLibVarSet("maxentities", buf);
	//bsp checksum
	trap_Cvar_VariableStringBuffer("sv_mapChecksum", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("sv_mapChecksum", buf);
	//maximum number of aas links
	trap_Cvar_VariableStringBuffer("max_aaslinks", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("max_aaslinks", buf);
	//maximum number of items in a level
	trap_Cvar_VariableStringBuffer("max_levelitems", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("max_levelitems", buf);
	//game type
	trap_Cvar_VariableStringBuffer("g_gametype", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap_BotLibVarSet("g_gametype", buf);
	//bot developer mode and log file
	trap_BotLibVarSet("bot_developer", bot_developer.string);
	trap_Cvar_VariableStringBuffer("logfile", buf, sizeof(buf));
	trap_BotLibVarSet("log", buf);
	//no chatting
	trap_Cvar_VariableStringBuffer("bot_nochat", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("nochat", buf);
	//visualize jump pads
	trap_Cvar_VariableStringBuffer("bot_visualizejumppads", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("bot_visualizejumppads", buf);
	//forced clustering calculations
	trap_Cvar_VariableStringBuffer("bot_forceclustering", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forceclustering", buf);
	//forced reachability calculations
	trap_Cvar_VariableStringBuffer("bot_forcereachability", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forcereachability", buf);
	//force writing of AAS to file
	trap_Cvar_VariableStringBuffer("bot_forcewrite", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forcewrite", buf);
	//no AAS optimization
	trap_Cvar_VariableStringBuffer("bot_aasoptimize", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("aasoptimize", buf);
	//
	trap_Cvar_VariableStringBuffer("bot_saveroutingcache", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("saveroutingcache", buf);
	//reload instead of cache bot character files
	trap_Cvar_VariableStringBuffer("bot_reloadcharacters", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap_BotLibVarSet("bot_reloadcharacters", buf);
	//base directory
	trap_Cvar_VariableStringBuffer("fs_basepath", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("basedir", buf);
	//game directory
	trap_Cvar_VariableStringBuffer("fs_game", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("gamedir", buf);
	//home directory
	trap_Cvar_VariableStringBuffer("fs_homepath", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("homedir", buf);
	//
#ifdef MISSIONPACK
	trap_BotLibDefine("MISSIONPACK");
#endif
	//setup the bot library
	return trap_BotLibSetup();
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	int			errnum;

	trap_Cvar_Register(&bot_thinktime, "bot_thinktime", "100", CVAR_CHEAT);
	trap_Cvar_Register(&bot_memorydump, "bot_memorydump", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_saveroutingcache, "bot_saveroutingcache", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_pause, "bot_pause", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_report, "bot_report", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testsolid, "bot_testsolid", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testclusters, "bot_testclusters", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_developer, "bot_developer", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_interbreedchar, "bot_interbreedchar", "", 0);
	trap_Cvar_Register(&bot_interbreedbots, "bot_interbreedbots", "10", 0);
	trap_Cvar_Register(&bot_interbreedcycle, "bot_interbreedcycle", "20", 0);
	trap_Cvar_Register(&bot_interbreedwrite, "bot_interbreedwrite", "", 0);

	//if the game is restarted for a tournament
	if (restart) {
		return qtrue;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof(botstates) );

	errnum = BotInitLibrary();
	if (errnum != BLERR_NOERROR) return qfalse;
	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->client, restart);
			}
		}
		//don't shutdown the bot library
	}
	else {
		trap_BotLibShutdown();
	}
	return qtrue;
}

/*
==============
ADAM Functions
==============
*/
int BotAdamAgent(int clientNum,float thinktime, float *neatInput)
{
	bot_state_t *bs;
	int i;

	trap_EA_ResetInput(clientNum);

	bs = botstates[clientNum];
	// From BotAI
	if(!bs || !bs->inuse)
	{
		BotAI_Print(PRT_FATAL, "BotAI: No setup");
		return qfalse;
	}

	if(!BotAI_GetClientState(clientNum, &bs->cur_ps))
	{
		BotAI_Print(PRT_FATAL,"Failed to get players state from %d\n",clientNum);
		return qfalse;
	}
	AdamBotChatSetup(clientNum,bs);
	for(i = 0; i<3;i++)
	{
		bs->viewangles[i] = AngleMod(bs->viewangles[i]+SHORT2ANGLE(bs->cur_ps.delta_angles[i]));
	}

	bs->ltime += thinktime;
	bs->thinktime = thinktime;

	// Vector copies
	VectorCopy(bs->cur_ps.origin,bs->origin);
	VectorCopy(bs->cur_ps.origin,bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;

	bs->areanum = BotPointAreaNum(bs->origin);


	/*
	=======================
	START OF NEURAL NETWORK
	=======================	
	*/
	/*
	MOVE_WALK						1
    MOVE_CROUCH						2
    MOVE_JUMP						4
    MOVE_GRAPPLE					8
	MOVE_ROCKETJUMP					16
    MOVE_BFGJUMP					32
	*/

	/*
	FUNCTIONS:
	BotChat_HitNoDeath(bs)
	*/
	bs->flags &= ~BFL_IDEALVIEWSET; 
	AdamBotIntermission(bs);

	if (!bs->adamNode) AdamEnter_Seek(bs);
	BotResetNodeSwitches();
	if(bs->lasthealth > bs->inventory[INVENTORY_HEALTH]+1)
		bs->timesHit++;
	
	for (i = 0; i < MAX_NODESWITCHES; i++) 
	{
		if (bs->adamNode(bs,neatInput)) break;
	}
	
	if(!bs->inuse)
		return qfalse;

	bs->lastframe_health = bs->inventory[INVENTORY_HEALTH];
	bs->lasthitcount = bs->cur_ps.persistant[PERS_HITS];



	//subtract the delta angles
	for(i = 0;i<3;i++)
	{
		bs->viewangles[i] = AngleMod(bs->viewangles[i]-SHORT2ANGLE(bs->cur_ps.delta_angles[i]));
	}

	return qtrue;

}
// Converts states from Quake III into floating number from 0-1
// The first 3 is not used by the NEAT, but used in order to create
// reference to the bots and their neural networks
void BotStateToNEAT(float neatArray[MAX_CLIENTS][ADAM_NN_INPUT], bot_state_t **bs)
{
	int i,j;

	for(i = 0; i < MAX_CLIENTS;i++)
	{
		if(bs[i]->adamFlag & ADAM_ADAPTIVE)
		{
			
			//Initialization and NN connection
			neatArray[i][0] = 2;

			// WALL RAYCASTS
			for(j=0;j<8;j++)
				neatArray[i][j+1] = bs[i]->wallRaycast[j];
			
			// ENEMY RADAR
			for(j=0;j<ADAM_RADAR_AMOUNT;j++)
				neatArray[i][j+9] = bs[i]->enemyRadars[j][2];
			
			// IS ON TARGET
			neatArray[i][9+ADAM_RADAR_AMOUNT] = bs[i]->isOnTarget;

			neatArray[i][10+ADAM_RADAR_AMOUNT] = bs[i]->combatStatus;
		/*	health = bs[i]->lastframe_health;
			// Self HP
			if(health < 0)
				neatArray[i][22] = 0;
			else
				neatArray[i][22] = health/200;

			// TAKING DMG
			neatArray[i][23] = bs[i]->isHit;*/
			// Self Weapon
		/*	neatArray[i][3] = bs[i]->weaponnum/9;

			// Self Ammo
			neatArray[i][4] = GetAmmoWeapon(bs[i]->weaponnum,bs[i])/200;
		
			// Self HP
			if(bs[i]->lastframe_health < 0)
				neatArray[i][5] = 0;
			else
				neatArray[i][5] = bs[i]->lastframe_health/200;

			// Self Armor
			neatArray[i][6] = bs[i]->inventory[INVENTORY_ARMOR]/200;

			// Self Crounch
			neatArray[i][7] = (bs[i]->cur_ps.pm_flags & PMF_DUCKED)== PMF_DUCKED;

			// Self in-air
			neatArray[i][8] = bs[i]->cur_ps.groundEntityNum == ENTITYNUM_NONE;

			// Self Velocity
			neatArray[i][9]  = (bs[i]->velocity[0]+320)/320;
			neatArray[i][10] = (bs[i]->velocity[1]+320)/320;
			neatArray[i][11] = (bs[i]->velocity[2]+270)/270;

			// Self View angle
			neatArray[i][12] = bs[i]->viewangles[0]/360;
			neatArray[i][13] = bs[i]->viewangles[1]/360;
			neatArray[i][14] = bs[i]->viewangles[2]/360;

			//
			========================================================
			ENEMY 
			========================================================
			//

			// Enemy Radar

			for(j = 0;j<4;j++) neatArray[i][j+15] = bs[i]->enemyRadars[j];
			
			// G_Printf("Last call for enemyRadar:[%f,%f,%f,%f] \n",neatArray[i][15],neatArray[i][16],neatArray[i][17],neatArray[i][18]);
			
			
			// Enemy Crouching
			neatArray[i][15] = (bs[i]->adamFlag & ADAM_ENEMYCROUCH) == ADAM_ENEMYCROUCH;
			
			// Enemy in Air
			neatArray[i][16] = (bs[i]->adamFlag & ADAM_ENEMYAIR) == ADAM_ENEMYAIR;

			// Enemy Shooting
			neatArray[i][17] = (bs[i]->adamFlag & ADAM_ENEMYFIRE) == ADAM_ENEMYFIRE;
			// Enemy Weapon
			neatArray[i][18] = bs[i]->enemyWeapon/9;
			
			tempSquareDist = bs[i]->squaredEnemyDis;
			if(tempSquareDist > ADAM_MAX_DISTANCE)
				tempSquareDist = 1;
			else if(tempSquareDist < ADAM_MIN_DISTANCE)
				tempSquareDist = 0;
			else
				tempSquareDist = (tempSquareDist-ADAM_MIN_DISTANCE)/(ADAM_MAX_DISTANCE-ADAM_MIN_DISTANCE);

			// Enemy Distance
			neatArray[i][19] = tempSquareDist;
			// Enemy Dir (already normalized)
			neatArray[i][20] = (bs[i]->enemyDir[0]+1)/2;
			neatArray[i][21] = (bs[i]->enemyDir[1]+1)/2;
			neatArray[i][22] = (bs[i]->enemyDir[2]+1)/2;
			// Enemy Velocity (consider to remove initially, since we are using hitscan)
			neatArray[i][23] = (bs[i]->enemyvelocity[0]+320)/320;
			neatArray[i][24] = (bs[i]->enemyvelocity[1]+320)/320;
			neatArray[i][25] = (bs[i]->enemyvelocity[2]+270)/270;*/
		}

	}
	
}

int GetAmmoWeapon(int weaponNumber, bot_state_t* bs)
{
	int ammo;
	switch(weaponNumber)
	{
		case 1:
			ammo = 1;
			break;
		case 2:
			ammo = bs->inventory[INVENTORY_BULLETS];
			break;
		case 3:
			ammo = bs->inventory[INVENTORY_SHELLS];
			break;
		case 4:
			ammo = bs->inventory[INVENTORY_GRENADES];
			break;
		case 5:
			ammo = bs->inventory[INVENTORY_CELLS];
			break;
		case 6:
			ammo = bs->inventory[INVENTORY_LIGHTNINGAMMO];
			break;
		case 7:
			ammo = bs->inventory[INVENTORY_ROCKETS];
			break;
		case 8:
			ammo = bs->inventory[INVENTORY_SLUGS];
			break;
		case 9:
			ammo = bs->inventory[INVENTORY_BFGAMMO];
			break;
		default:
			ammo = 0;
			break;
	}
	return ammo;
}
int AdamAttack(bot_state_t* bs)
{
	weaponinfo_t weaponInfo;
	trap_BotGetWeaponInfo(bs->ws,bs->weaponnum,&weaponInfo);

	if(weaponInfo.flags & WFL_FIRERELEASED)
	{
		if(bs->flags & BFL_ATTACKED)
		{
			trap_EA_Attack(bs->client);
		}
	}
	else
	{
		trap_EA_Attack(bs->client);
	}

	bs->flags ^= BFL_ATTACKED;

	return qtrue;
	//CONSIDER ADDING AMMO HERE 
}
int AdamSelectWeapon(bot_state_t* bs, float weaponIndex)
{
	int ammoval;
	int convertedWeapon;
	convertedWeapon = (weaponIndex*9)+1;
	if(convertedWeapon < 1)
	{
		G_Printf("Client %i: WEAPON SELECTION FAULTY \n",bs->client);
		return qfalse;
	}
	else if(convertedWeapon > 9)
		convertedWeapon = 9;

	switch(convertedWeapon)
	{
		case 1:
			if(bs->inventory[INVENTORY_GAUNTLET])
				bs->weaponnum = WP_GAUNTLET;
			else
				ammoval = -1;
			break;
		case 2:
			if(bs->inventory[INVENTORY_MACHINEGUN])
				bs->weaponnum=WP_MACHINEGUN;
			else
				ammoval = -1;
			break;
		case 3:
			if(bs->inventory[INVENTORY_SHOTGUN])
				bs->weaponnum=WP_SHOTGUN;
			else
				ammoval = -1;
			break;
		case 4:
			if(bs->inventory[INVENTORY_GRENADELAUNCHER])
				bs->weaponnum=WP_GRENADE_LAUNCHER;
			else
				ammoval = -1;
			break;
		case 5:
			if(bs->inventory[INVENTORY_ROCKETLAUNCHER])
				bs->weaponnum=WP_ROCKET_LAUNCHER;
			else
				ammoval = -1;
			break;
		case 6:
			if(bs->inventory[INVENTORY_LIGHTNING])
				bs->weaponnum=WP_LIGHTNING;
			else
				ammoval = -1;
			break;
		case 7:
			if(bs->inventory[INVENTORY_RAILGUN])
				bs->weaponnum=WP_RAILGUN;
			else
				ammoval = -1;
			break;
		case 8:
			if(bs->inventory[INVENTORY_PLASMAGUN])
				bs->weaponnum=WP_PLASMAGUN;
			else
				ammoval = -1;
			break;
		case 9:
			if(bs->inventory[INVENTORY_BFG10K])
				bs->weaponnum=WP_BFG;
			else
				ammoval = -1;
			break;
		default:
			ammoval = -1;
	}
	if(ammoval == -1)
	{
		G_Printf("Not owned weapon\n");
		
	}
	else
	{
		switch(bs->weaponnum)
		{
			case WP_MACHINEGUN:
				ammoval = INVENTORY_BULLETS;
				break;
			case WP_SHOTGUN:
				ammoval = INVENTORY_SHELLS;
				break;
			case WP_GRENADE_LAUNCHER:
				ammoval = INVENTORY_GRENADES;
				break;
			case WP_ROCKET_LAUNCHER:
				ammoval = INVENTORY_ROCKETS;
				break;
			case WP_LIGHTNING:
				ammoval = INVENTORY_LIGHTNINGAMMO;
				break;
			case WP_RAILGUN:
				ammoval = INVENTORY_SLUGS;
				break;
			case WP_PLASMAGUN:
				ammoval = INVENTORY_CELLS;
				break;
			case WP_BFG:
				ammoval = INVENTORY_BFGAMMO;
				break;
			default:
				ammoval = 0;
		}
		trap_EA_SelectWeapon(bs->client,bs->weaponnum);
	}
	return qtrue;
}
/*
	airstate:
	0 = jump not begun
	1 = jump begun
	2 = jump finished

*/
int AdamJump(bot_state_t* bs, int airState)
{
	return qtrue;
}

//Used to keep the bot active
void AdamBotChatSetup(int client, bot_state_t *bs)
{	
	char buf[1024], *args;
	while( trap_BotGetServerCommand(client, buf, sizeof(buf)) ) {
	//have buf point to the command and args to the command arguments
		args = strchr( buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences( args );

		if (!Q_stricmp(buf, "cp "))
			{ /*CenterPrintf*/ }
		else if (!Q_stricmp(buf, "cs"))
			{ /*ConfigStringModified*/ }
		else if (!Q_stricmp(buf, "print")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_NORMAL, args);
		}
		else if (!Q_stricmp(buf, "chat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
		else if (!Q_stricmp(buf, "tchat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
#ifdef MISSIONPACK
		else if (!Q_stricmp(buf, "vchat")) {
			BotVoiceChatCommand(bs, SAY_ALL, args);
		}
		else if (!Q_stricmp(buf, "vtchat")) {
			BotVoiceChatCommand(bs, SAY_TEAM, args);
		}
		else if (!Q_stricmp(buf, "vtell")) {
			BotVoiceChatCommand(bs, SAY_TELL, args);
		}
#endif
		else if (!Q_stricmp(buf, "scores"))
			{ /*FIXME: parse scores?*/ }
		else if (!Q_stricmp(buf, "clientLevelShot"))
			{ /*ignore*/ }
	}
}
int GetAdaptiveAgents(bot_state_t** bs, int* AdamIndices)
{
	int amount,i;
	amount = 0;
	for(i = 0;i<8;i++)
	{
		AdamIndices[i] =-1;
	}
	for(i = 0; i < MAX_CLIENTS;i++)
	{
		if(bs[i]->adamFlag & ADAM_ADAPTIVE)
		{ 
			AdamIndices[amount] = i;
			amount++;
		}		
	}
	return amount;
}
