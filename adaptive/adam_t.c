//===========================================================================
//
// Name:			Tigc.c
// Function:		chat lines for Biker
// Programmer:		MrElusive (MrElusive@idsoftware.com)
// Author:			The Seven Swords & R.A. Salvatore
// Editor:			Paul Jaquays
// Last update:		Oct. 10, 1999
// Tab Size:		3 (real tabs)
// Hack and changed for Tigger by Tigger-oN 19 Jan 00
//===========================================================================

//example initial chats
chat "adam"
{
	//the teamplay.h file is included for all kinds of teamplay chats
	#include "teamplay.h"

	//======================================================
	//======================================================

	type "game_enter" //initiated when the bot enters the game
	{
		HELLO5;
		"Adaptive resources initialized.";
		// 0 = bot name
	} //end type

	type "game_exit" //initiated when the bot exits the game
	{
		"Bounce bounce bounce - I'm out of here!.";
		"Screw this, I've got better things to do.";
		GOODBYE1;
		// 0 = bot name
	} //end type

	type "level_start" //initiated when a new level starts
	{
		LEVEL_START1;
		"", 4, " my new playground. Oi! ", fighter, " - off the round-about!.";
		"Cool, ", 4, " was fun once.";
		// 0 = bot name
		// 1 = randomly chosen player
		// 4 = Level's title
		} //end type

	type "level_end" //initiated when a level ends and the bot is not first and not last in the rankings
	{
		"Let me get warmed up, ok?";
		LEVEL_END0;
		2, " cheats and ", 3, " sucks. Now that we know, let's go again.";
		// 0 = bot name
		// 2 = opponent in first place
		// 3 = opponent in last place
	} //end type

	type "level_end_victory" //initiated when a level ends and the bot is first in the rankings
	{
		"I bounced heads!!";
		"Better luck next time ", 3, " - ya loser :]";
		LEVEL_END_VICTORY3;
		// 0 = bot name
		// 3 = opponent in last place
	} //end type

	type "level_end_lose" //initiated when a level ends and the bot is last in the rankings
	{
		"That wasn't really much of a win ", 2, " .... gimme another chance?.";
		"That's it! - anyone seen Winnie?.";

		LEVEL_END_LOSE1;
		// 0 = bot name
	} //end type

	//======================================================
	//======================================================

	type "hit_talking" //bot is hit while chat balloon is visible; lecture attacker on poor sportsmanship
	{
		//"Waaasssssupppp, ", fighter, "?";
		"That must've been hard ", 0, ".";
		"HEY!, ", 0, ". didn't ya see the chat bubble? - lame ass";
		//0 = shooter
	} //end type

	type "hit_nodeath" //bot is hit by an opponent's weapon attack; either praise or insult
	{
		"I'm goona bounce ya head for that one!!";
		"Don't dirty the stripes, ", 0,"!";
		"You honey lover! I bet you eat it out of the jar.";
		"ahahahhahahaha HehehHEhhahhrharhah, you're just soo good - NOT!";
		"Let me bounce on ya face ", 0, ".";
		DEATH_INSULT2;
		//0 = shooter
	} //end type

	type "hit_nokill" //bot hits an opponent but does not kill it
	{
		TAUNT1;
		"Yer as slow as ya are ugly.";
		TAUNT8;
		"You look mighty pretty in blood red.";
		"Lousy, cheap ammo.";
		"WTF? - did I miss?";
		//0 = opponent
	} //end type

	//======================================================
	//======================================================

	type "death_telefrag" //initiated when the bot is killed by a telefrag
	{
		"What happened?.";
		"Um... ok";
		"well, that was hard";
		DEATH_TELEFRAGGED0;
		// 0 = enemy name
	} //end type

	type "death_cratered" //initiated when the bot is killed by falling damage
	{
		"But I can bounce?.";
		"what happened to my tail?";
		"Oh great..";
		// 0 = enemy name
	} //end type

	type "death_lava" //initiated when the bot dies in lava
	{
		"Och!";
		"Oh bummer";
		DEATH_LAVA1;
		// 0 = enemy name
	} //end type

	type "death_slime" //initiated when the bot dies in slime
	{
		"Reminds me of Poohs vomit";
		"Piglet sleeps in the stuff?";
		"Um... ok.. I though it was a pool";
		// 0 = enemy name
	} //end type

	type "death_drown" //initiated when the bot drowns
	{
		"I hate water.";
		"Tops! furs all wet now :[";
		"Stupid place for a swimmmin' pool.";
		// 0 = enemy name
	} //end type

	type "death_suicide" //initiated when bot blows self up with a weapon or craters
	{
		"I didn't see the wall :[, ", 0, ", your turn now.";
		"ok, wheres Crash?";
		DEATH_SUICIDE0;
		DEATH_SUICIDE5;
		// 0 = enemy name
	} //end type

	type "death_gauntlet" //initiated when the bot is killed by a gauntlet attack
	{
		"Get yer slimy paws off o' me, ", 0, "!";
		"HAhhahahHAHh.";
		"You think you sooo hot don't you!";
		DEATH_GAUNTLET2;
		// 0 = enemy name
	} //end type

	type "death_rail" //initiated when the bot is killed by a rail gun shot
	{
		DEATH_RAIL2;
		"NIce and clean ", 0, ".";
		"Ohhh.. that smarts.";
		// 0 = enemy name
	} //end type

	type "death_bfg" //initiated when the bot died by a BFG
	{
		"Umm.. a no skill gun for you hey?";
		DEATH_BFG2;
		"Ha, ya missed, ", 0, "! 'Almost' only counts in hand grenades and ... oh, puke ...";
		"Snall nuke explosions don't count.";
		// 0 = enemy name
	} //end type

	type "death_insult" //insult initiated when the bot died
	{
		"Hey, ", 0, " whatss uupp?.";
		"100 Acre Forest ain't for the likes of you, ", fighter, ".";
		"I can't believe a ", fighter, " like ", 0, " fragged me!";
		DEATH_INSULT0;
		DEATH_INSULT4;
		DEATH_INSULT5;
		// 0 = enemy name
	} //end type

	type "death_praise" //praise initiated when the bot died
	{
		"What, you want a medal now ", 0, "?";
		"", 0, " who invited you?";
		D_PRAISE2;
		0, " your going down!";
		// 0 = enemy name
	} //end type

	//======================================================
	//======================================================

	type "kill_rail" //initiated when the bot kills someone with rail gun
	{
		"Bulls eye! ", fighter, ", ya a great target.";
		KILL_RAIL1;
		KILL_RAIL0;
		// 0 = enemy name
	} //end type

	type "kill_gauntlet" //initiated when the bot kills someone with gauntlet
	{
		"you sure do stink.";
		"Gee, thats a nasty scab.";
		"HAHhahHahhHAhHEHEHHhhahH.";
		"Who wants some?";
		KILL_GAUNTLET0;
		// 0 = enemy name
	} //end type

	type "kill_telefrag" //initiated when the bot telefragged someone
	{
		"You stand still, look what happens";
		"Didn't see ya there :]";
		"Pop!!";
		TELEFRAGGED0;
		// 0 = enemy name
	} //end type

	type "kill_suicide" //initiated when the player kills self
	{
		"Hey moron, AIM, then fire!";
		"Thats the wrong way around :]";
		// 0 = enemy name
	} //end type

	type "kill_insult" //Insult initiated when the bot killed someone
	{
		"You're good to play with ", 0, ".";
		"Ahhhh, Cool, got another?";
		"Come and play some more?";
		KILL_INSULT0;
		KILL_INSULT4;
		KILL_INSULT8;
		// 0 = enemy name
	} //end type

	type "kill_praise" //praise initiated when the bot killed someone
	{
		KILL_INSULT36;
		"No honey for you!";
		"", 0, " - I fart at you.";
	
		// 0 = enemy name
	} //end type

	//======================================================
	//======================================================

	type "random_insult" //insult initiated randomly (just when the bot feels like it)
	{
		TAUNT7;
		MISC6;	
		"You sure are ugly looking";
		"Oi!, ", 0, " you look like something Piglet would eat.";
		"Here Piggy Piggy Pig!";
		"", 0, " is one big fat turd just waiting to drop.";
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type

	type "random_misc" //miscellanous chats initiated randomly
	{
		GUYTALK2;
		"I'm a big bouncer.";
		"Bring it on, I can take it!";
		"Want some apples?";
		"This map kicks but";
		one_liners;
		"Hunter and me goo waaaaayyy back.";
		femalebot, " likes it that way I tell ya"; 
		MISC0;
		MISC8;
		MISC12;
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type
} //end tig/biker chat
