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
 * name:		ai_dmnet.h
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /source/code/botai/ai_chat.c $
 *
 *****************************************************************************/

#define MAX_NODESWITCHES	50

void AIEnter_Intermission(bot_state_t *bs, char *s);
void AIEnter_Observer(bot_state_t *bs, char *s);
void AIEnter_Respawn(bot_state_t *bs, char *s);
void AIEnter_Stand(bot_state_t *bs, char *s);
void AIEnter_Seek_ActivateEntity(bot_state_t *bs, char *s);
void AIEnter_Seek_NBG(bot_state_t *bs, char *s);
void AIEnter_Seek_LTG(bot_state_t *bs, char *s);
void AIEnter_Seek_Camp(bot_state_t *bs, char *s);
void AIEnter_Battle_Fight(bot_state_t *bs, char *s);
void AIEnter_Battle_Chase(bot_state_t *bs, char *s);
void AIEnter_Battle_Retreat(bot_state_t *bs, char *s);
void AIEnter_Battle_NBG(bot_state_t *bs, char *s);
int AINode_Intermission(bot_state_t *bs);
int AINode_Observer(bot_state_t *bs);
int AINode_Respawn(bot_state_t *bs);
int AINode_Stand(bot_state_t *bs);
int AINode_Seek_ActivateEntity(bot_state_t *bs);
int AINode_Seek_NBG(bot_state_t *bs);
int AINode_Seek_LTG(bot_state_t *bs);
int AINode_Battle_Fight(bot_state_t *bs);
int AINode_Battle_Chase(bot_state_t *bs);
int AINode_Battle_Retreat(bot_state_t *bs);
int AINode_Battle_NBG(bot_state_t *bs);

void BotResetNodeSwitches(void);
void BotDumpNodeSwitches(bot_state_t *bs);


/*
==========================
ADAM NODES
==========================
*/
#if defined(ADAM_ACTIVE) || defined(ADAM_DEBUG)
void AdamEnter_Seek(bot_state_t* bs);
int Adam_Seek(bot_state_t* bs, float* neatData);
void AdamEnter_NearbySeek(bot_state_t* bs);
int Adam_NearbySeek(bot_state_t* bs, float* neatData);
void AdamEnter_Fight(bot_state_t* bs);
int Adam_Fight(bot_state_t* bs, float* neatData);
void AdamEnter_Respawn(bot_state_t* bs);
int Adam_Respawn(bot_state_t*, float* neatData);
#endif
#if defined(ADAM_DEBUG) || defined(ADAM_TRAINING_DEBUG)
void AdamEnter_Debug(bot_state_t* bs);
int Adam_Debug(bot_state_t* bs, float* neatData);
void AdamEnter_ActiveDebug(bot_state_t* bs);
int Adam_ActiveDebug(bot_state_t* bs,float* neatData);
#endif
#if defined(ADAM_TRAINING) || defined(ADAM_TRAINING_DEBUG)
void AdamEnter_Training(bot_state_t* bs);
int Adam_Training(bot_state_t* bs,float* neatData);
void AdamEnter_Wait(bot_state_t* bs);
int Adam_Wait(bot_state_t* bs, float* neatData);
int AdamSetTrainingRadars(bot_state_t* bs);
void AdamTrainingFitness(bot_state_t* bs);
void AdamEnter_Intermedium(bot_state_t* bs);
int Adam_Intermedium(bot_state_t* bs, float* neatData);
void AdamEnter_SetupView(bot_state_t* bs);
int Adam_SetupView(bot_state_t* bs, float* neatData);
void Adam_CalcFitnessForFrame(bot_state_t* bs);
#endif
// ADAM Helper functions and general nodes
#if defined(ADAM_ACTIVE) || defined(ADAM_DEBUG)

int AdamGetLongTermItemGoal(bot_state_t* bs, int travelFlag, bot_goal_t* goal);
#endif
void AdamSetTrainingTime(float in);
// ADAM Variable

#define NN_THRESHOLD 0.5f
