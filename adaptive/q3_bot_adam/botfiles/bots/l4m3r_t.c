//===========================================================================
//	L4m3r	version 0.0
//	By: Apathy[AGQx], apathy@agqx.org, http://www.agqx.org
//
//	Made with doktor's BotStudio v. 0.97
//	http://www.planetquake.com/botstudio  e-mail: doktor@cutey.com
//===========================================================================


chat "l4m3r"
{
	#include "teamplay.h"

	type "game_enter"
	{
		HELLO1;
		HELLO6;
		"Fag0t!', ", 1, ", I R QuakG0d! U R Homo!";
		"U H0m0! ", 4, " I R00L this l3v3l!";
	}

	type "game_exit"
	{
		"Fukin cheeter!";
		"E4t M3 U Fag0t!";
		"Cheeting Biotch!";
		"U Suck!";
		GOODBYE;
		GOODBYE4;
	}

	type "level_start"
	{
		"I gonna kik yur ass";
		"I am l33t U will die";
		"I ro0lz  U suk hom0;";
	}

	type "level_end"
	{
		"Cheeter bich!.";
		"I ro0lz!!!";
		"I win!!!";
		"HAHAHAHA, ", 3, "sux!";
	}

	type "level_end_victory"
	{
		"I am QuakG0D!!!";
		"Loosers!";
		LEVEL_END_VICTORY2;
		LEVEL_END_VICTORY3;
		"34T M3 Loosers!!!";
		 3, ", U sux. HAHAHAHAHAHAHAHAHAHA!!!";
	}

	type "level_end_lose"
	{
		fighter, "UR a fukin cheeter";
		"LAGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG";
		LEVEL_END_LOSE2;
		LEVEL_END_LOSE1;
	}

	type "hit_talking"
	{
		"Fukin Cheeter Hom0 Bich!";
		 0, ", U Sux!";
		"Thats the only way you can kill me HAHAHAHAHA";
	}

	type "hit_nodeath"
	{
		"U missed", 0, " U Fukin Cheeter!";
		"HAHAHAHAHAHAHA  Luz3r";
		"I am the Best U fukin H0m0!!!.";
		 0, " is a ", 1, " Ho!!!";
	}

	type "hit_nokill"
	{
		"Die Cheeter!!!";
		"Run away, ", 0, " U hom0!!!";
		"I shot U like a million times Cheeter!!! ";
		TAUNT;
		PRAISE2;
	}

	type "death_telefrag"
	{
		"Fukin Telefrag Cheeter!";
		DEATH_TELEFRAGGED2;
	}

	type "death_cratered"
	{
		"I meant to do that";
		"Fukin Cheeters";
		"LAGGGGGGGGGGGGGGG";
	}

	type "death_lava"
	{
		"Lava Cheeter!!!";
		"Cheeting Lava Ho!!!";
		DEATH_LAVA0;
	}

	type "death_slime"
	{
		"I wanted to do that.";
		"Hey, ", 0, ", did U see me act like you?";
	}

	type "death_drown"
	{
		"LAGGGGGGGGGGGGGGGGGG";
		"Water Cheeter Ho!!!";
		DEATH_DROWN1;
	}

	type "death_suicide"
	{
		"LaGGGGGGGGGGGGGGG";
		"I wanted to do that.";
		DEATH_SUICIDE3;
		DEATH_SUICIDE4;
	}

	type "death_gauntlet"
	{
		"Gauntlet Cheeter Fag0t!";
		DEATH_GAUNTLET0;
	}

	type "death_rail"
	{
		"Fukin Aimbot";
		DEATH_RAIL1;
		"U hav aimbot, ", 0, "!";
	}

	type "death_bfg"
	{
		"BFG Ho!!!";
		DEATH_BFG1;
	}

	type "death_insult"
	{
		"Fag0t!";
		"Cheeter!";
		 0, "U Sux Fag0t!!!  I am QuakG0D!!!";
		DEATH_INSULT2;
	}

	type "death_praise"
	{
		"";
	}

	type "kill_rail"
	{
		"HAHAHAHAHAHAHAHAHAHAHAHAHAHA, ", 0, " Looser!";
		"I am QuakG0D!";
		KILL_RAIL0;
	}

	type "kill_gauntlet"
	{
		KILL_GAUNTLET1;
	}

	type "kill_telefrag"
	{
		"HAHAHAHAHAHAHAHAHAHAHAHAHAHA, ", fighter, ".";
		TELEFRAGGED2;
	}

	type "kill_suicide"
	{
		"U sux";
		"Hom0 Fagot Looser";
	}

	type "kill_insult"
	{
		"HAHAHAHAHA, ", 0, " sux!";
		"Stoopid Looser!";
		KILL_INSULT3;
		KILL_INSULT6;
	}

	type "kill_praise"
	{
		"";
	}

	type "random_insult"
	{
		 0, "is a Fag0t!";
		"Cheeter!";
		"LAGGGGGGGGGGGGGGGGGGGGGG!!!";
		"U sux Hom0!";
		" Pheer my ", 5;
		MISC10;
		MISC12;
	}

	type "random_misc"
	{
		"Ur a cheeter and a fag0t, ", 0, "!";
		"Hey ", 1, ", u sux!   HAHAHAHAHAHAHAHAHAHA";
		"Hey ", 0, ", 34T M3!  HAHAHAHAHAHAHAHAHAHA";
		 1, ", I will have to show you how to use that ", 5, " again!";
		MISC2;
		one_liners;
	}
}
