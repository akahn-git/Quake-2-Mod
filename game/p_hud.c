/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"



/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if (deathmatch->value || coop->value)
		ent->client->showscores = true;
	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value || coop->value)
	{
		DeathmatchScoreboardMessage (ent, NULL);
		gi.unicast (ent, true);
	}

}

void BeginIntermission (edict_t *targ)
{
	int		i, n;
	edict_t	*ent, *client;

	if (level.intermissiontime)
		return;		// already activated

	game.autosaved = false;

	// respawn any dead clients
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0)
			respawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	if (strstr(level.changemap, "*"))
	{
		if (coop->value)
		{
			for (i=0 ; i<maxclients->value ; i++)
			{
				client = g_edicts + 1 + i;
				if (!client->inuse)
					continue;
				// strip players of all keys between units
				for (n = 0; n < MAX_ITEMS; n++)
				{
					if (itemlist[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else
	{
		if (!deathmatch->value)
		{
			level.exitintermission = 1;		// go immediately to the next level
			return;
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);

	// move all clients to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission (client);
	}
}


/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total;
	int		picnum;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char	*tag;

	// sort the clients by score
	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;
		score = game.clients[i].resp.score;
		for (j=0 ; j<total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k=total ; k>j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// print level name and exit rules
	string[0] = 0;

	stringlength = strlen(string);

	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i=0 ; i<total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		picnum = gi.imageindex ("i_fixme");
		x = (i>=6) ? 160 : 0;
		y = 32 + 32 * (i%6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = NULL;
		if (tag)
		{
			Com_sprintf (entry, sizeof(entry),
				"xv %i yv %i picn %s ",x+32, y, tag);
			j = strlen(entry);
			if (stringlength + j > 1024)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		// send the layout
		Com_sprintf (entry, sizeof(entry),
			"client %i %i %i %i %i %i",
			x, y, sorted[i], cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600);
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}


/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
	DeathmatchScoreboardMessage (ent, ent->enemy);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;

	/*if (!deathmatch->value && !coop->value)
		return;*/

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard (ent);
}

//I straight up hijacked HelpComputer
/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer(edict_t* ent, char inMode)
{
	char	string[1024];
	char	mode;

	//set mode
	mode = inMode;

	// send the layout

	//buy menu
	if (mode == 'b')
	{
		char* wth;
		char* ple;
		char* hk;
		char* rl;
		char* wht;
		char* bt;
		char* tkle;

		char* plecost;
		char* hkcost;
		char* rlcost;
		char* whtcost;
		char* btcost;
		char* tklecost;

		char* plerange;
		char* hkthresh;
		char* rlspeed;
		char* whtsize;
		char* bttype;
		char* tklespawn;

		//weather
		if (weather->value == 0)
			wth = "Sunny";
		else if (weather->value == 1)
			wth = "Cloudy";
		else if (weather->value == 2)
			wth = "Raining";
		else
			wth = "Fire Storm";

		//pole
		if (pole->value == 0)
		{
			ple = "Base Pole";
			plerange = "Pole Range: 350m";
		}
		else if (pole->value == 1)
		{
			ple = "Metal Pole";
			plerange = "Pole Range: 550m";
		}
		else if (pole->value == 2)
		{
			ple = "Master Pole";
			plerange = "Pole Range: 750m";
		}
		else
		{
			ple = "Staff of Zeus";
			plerange = "Pole Range: Unlimited";
		}

		//hook
		if (hook->value == 0)
		{
			hk = "Base Hook";
			hkthresh = "Yoinks Required: Three";
		}
		else if (hook->value == 1)
		{
			hk = "Sharp Hook";
			hkthresh = "Yoinks Required: Two";
		}
		else if (hook->value == 2)
		{
			hk = "Master Hook";
			hkthresh = "Yoinks Required: One";
		}
		else
		{
			hk = "Armour Piercing";
			hkthresh = "Yoinks Required: None";
		}

		//reel
		if (reel->value == 0)
		{
			rl = "Base Reel";
			rlspeed = "Reel Time: Regular";
		}
		else if (reel->value == 1)
		{
			rl = "Quick Reel";
			rlspeed = "Reel Time: Extended";
		}
		else if (reel->value == 2)
		{
			rl = "Master Reel";
			rlspeed = "Reel Time: Long";
		}
		else
		{
			rl = "Owls Reel";
			rlspeed = "Reel Time: Infinity";
		}

		//weight
		if (weight->value == 0)
		{
			wht = "Base Weight";
			whtsize = "Fish Size: Small";
		}
		else if (weight->value == 1)
		{
			wht = "Heavy Weight";
			whtsize = "Fish Size: Medium";
		}
		else if (weight->value == 2)
		{
			wht = "Master Weight";
			whtsize = "Fish Size: Large";
		}
		else
		{
			wht = "The Anchor";
			whtsize = "Fish Size: Colosal";
		}

		//bait
		if (bait->value == 0)
		{
			bt = "Base Bait";
			bttype = "Fish Type: Common";
		}
		else if (bait->value == 1)
		{
			bt = "Tasty Bait";
			bttype = "Fish Type: Uncommon";
		}
		else if (bait->value == 2)
		{
			bt = "Master Bait";
			bttype = "Fish Type: Rare";
		}
		else
		{
			bt = "Irresistable Bait";
			bttype = "Fish Type: Mythical";
		}

		//tackle
		if (tackle->value == 0)
		{
			tkle = "Base Tackle";
			tklespawn = "Fish Price: Regular";
		}
		else if (tackle->value == 1)
		{
			tkle = "Flashy Tackle";
			tklespawn = "Fish Price: Bonus";
		}
		else if (tackle->value == 2)
		{
			tkle = "Master Tackle";
			tklespawn = "Fish Price: High";
		}
		else
		{
			tkle = "Diamond Tackle";
			tklespawn = "Fish Price: Jackpot";
		}

		plecost = "";
		rlcost = "";
		hkcost = "";
		whtcost = "";
		btcost = "";
		tklecost = "";

		if (pole->value == 1)
		{
			plecost = "[$100]";
		}

		if (reel->value == 1)
		{
			rlcost = "[$100]";
		}

		if (hook->value == 1)
		{
			hkcost = "[$100]";
		}

		if (weight->value == 1)
		{
			whtcost = "[$100]";
		}

		if (bait->value == 1)
		{
			btcost = "[$100]";
		}

		if (tackle->value == 1)
		{
			tklecost = "[$100]";
		}

		if (pole->value == 0)
		{
			plecost = "[$50]";
		}

		if (reel->value == 0)
		{
			rlcost = "[$50]";
		}

		if (hook->value == 0)
		{
			hkcost = "[$50]";
		}

		if (weight->value == 0)
		{
			whtcost = "[$50]";
		}

		if (bait->value == 0)
		{
			btcost = "[$50]";
		}

		if (tackle->value == 0)
		{
			tklecost = "[$50]";
		}

		if (pole->value == 2 || maxpole->value == 1)
		{
			plecost = "[OWNED]";
		}

		if (reel->value == 2 || maxreel->value == 1)
		{
			rlcost = "[OWNED]";
		}

		if (hook->value == 2 || maxhook->value == 1)
		{
			hkcost = "[OWNED]";
		}

		if (weight->value == 2 || maxweight->value == 1)
		{
			whtcost = "[OWNED]";
		}

		if (bait->value == 2 || maxbait->value == 1)
		{
			btcost = "[OWNED]";
		}

		if (tackle->value == 2 || maxtackle->value == 1)
		{
			tklecost = "[OWNED]";
		}

		Com_sprintf(string, sizeof(string),
			"xv 0 yv 8 picn net "						//background
			"xv 0 yv 10 string2 \"Weather: %s\" "
			"xv 0 yv 10 string2 \"_______________\" "
			"xv 65 yv 30 string2 \"Pole [P] %s\" "		//pole header
			"xv 65 yv 40 string2 \"%s) %s\" "			//pole equipt
			"xv 65 yv 60 string2 \"Reel [R] %s\" "		//reel header
			"xv 65 yv 70 string2 \"%s) %s\" "			//reel equipt
			"xv 65 yv 90 string2 \"Bait [B] %s\" "		//bait header
			"xv 65 yv 100 string2 \"%s) %s\" "			//bait equipt
			"xv 65 yv 120 string2 \"Hook [H] %s\" "		//hook header
			"xv 65 yv 130 string2 \"%s) %s\" "			//hook equipt
			"xv 65 yv 150 string2 \"Weight [W] %s\" "	//weight header
			"xv 65 yv 160 string2 \"%s) %s\" "			//weight equipt
			"xv 65 yv 180 string2 \"Tackle [T] %s\" "	//tackle header
			"xv 65 yv 190 string2 \"%s) %s\" ",			//tackle equipt

			wth,
			plecost,
			ple,
			plerange,
			rlcost,
			rl,
			rlspeed,
			btcost,
			bt,
			bttype,
			hkcost,
			hk,
			hkthresh,
			whtcost,
			wht,
			whtsize,
			tklecost,
			tkle,
			tklespawn);

		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);
	}

	//mini game
	if (mode == 'm')
	{
		char* bar;
		char* outoftime;
		char* pressequal;

		float startTime;
		float curTime;
		float remainingTime;
		float reelTime;

		int fish;
		int psuccess;
		int required;
		int loc;

		//determines the successful pulls
		psuccess = pulls->value;
		//gets the required pulls
		required = 3 - hook->value;
		//sets the start time
		startTime = minitime->value;
		//sets the current time
		curTime = level.time;
		//gets the fish we are going to catch
		fish = minifish->value;
		//gets the reel time
		reelTime = minireel->value;

		
		//determines the remaining time
		remainingTime = reelTime - (curTime - startTime);

		outoftime = " ";
		pressequal = " ";

		//check if you have the required pulls for a win
		if (required-psuccess<=0)
		{
			ent->client->wonmini = true;
			remainingTime = 0;
			pressequal = "Press [L] see your catch!";
		}

		//if you are out of time
		if (remainingTime <= 0)
		{
			
			ent->client->miniover = true;
			remainingTime = 0;
		
			if (psuccess < required)
			{
				pressequal = "It got away, press [L] to fish again.";
				outoftime = "Out of Time!";
			}
			else
			{
				outoftime = "";
			}
			
			
		}

		//determine the bar value
		loc = olocation->value;

		//check for fish pull
		int r = (fish*5)+(rand() % 100);
		int shouldPull = r/100;

		//it should be pulled and isnt in middle
		if (shouldPull == 1 && loc!=16)
		{
			//determine direction
			if (rand()%2==1)
			{
				gi.bprintf(PRINT_HIGH, "Fish is pulling left\n");
				fishpulls->value = fishpulls->value + 1;
				loc = 1;
			}
			else
			{
				gi.bprintf(PRINT_HIGH, "Fish is pulling right\n");
				fishpulls->value = fishpulls->value + 1;
				loc = 31;
			}

			//set the olocation
			olocation->value = loc;
		}

		//locks it to center if game over
		if (psuccess >= required)
		{
			loc = 16;
		}

		switch (loc)
		{
		case (1):
			bar = "[o--------------x---------------]";
			break;

		case (2):
			bar = "[-o-------------x---------------]";
			break;

		case (3):
			bar = "[--o------------x---------------]";
			break;

		case (4):
			bar = "[---o-----------x---------------]";
			break;

		case (5):
			bar = "[----o----------x---------------]";
			break;

		case (6):
			bar = "[-----o---------x---------------]";
			break;

		case (7):
			bar = "[------o--------x---------------]";
			break;

		case (8):
			bar = "[-------o-------x---------------]";
			break;

		case (9):
			bar = "[--------o------x---------------]";
			break;

		case (10):
			bar = "[---------o-----x---------------]";
			break;

		case (11):
			bar = "[----------o----x---------------]";
			break;

		case (12):
			bar = "[-----------o---x---------------]";
			break;

		case (13):
			bar = "[------------o--x---------------]";
			break;

		case (14):
			bar = "[-------------o-x---------------]";
			break;

		case (15):
			bar = "[--------------ox---------------]";
			break;

			//case (16): this is middle so default

		case (17):
			bar = "[---------------xo--------------]";
			break;

		case (18):
			bar = "[---------------x-o-------------]";
			break;

		case (19):
			bar = "[---------------x--o------------]";
			break;

		case (20):
			bar = "[---------------x---o-----------]";
			break;

		case (21):
			bar = "[---------------x----o----------]";
			break;

		case (22):
			bar = "[---------------x-----o---------]";
			break;

		case (23):
			bar = "[---------------x------o--------]";
			break;

		case (24):
			bar = "[---------------x-------o-------]";
			break;

		case (25):
			bar = "[---------------x--------o------]";
			break;

		case (26):
			bar = "[---------------x---------o-----]";
			break;

		case (27):
			bar = "[---------------x----------o----]";
			break;

		case (28):
			bar = "[---------------x-----------o---]";
			break;

		case (29):
			bar = "[---------------x------------o--]";
			break;

		case (30):
			bar = "[---------------x-------------o-]";
			break;

		case (31):
			bar = "[---------------x--------------o]";
			break;

		default:
			bar = "[---------------O---------------]";
			break;
		}

		int fp = fishpulls->value;

		if ((loc < 16) && (remainingTime>0) )
		{
			Com_sprintf(string, sizeof(string),
				"xv 0 yv 8 picn field_3 "							//background
				"xv 100 yv 8 picn tech2 "							//mini picture
				"xv 70 yv 160 string2 \"%s\" "						//bar
				"xv 120 yv 180 string2 \"Time Remaining: %.2f\" "	//time remaining
				"xv 140 yv 210 string2 \"Yoinks Required: %i\" "	//successful pulls remaining
				"xv 150 yv 230 string2 \"Fish Pulls: %i\" ",		//fish pulls

				bar,
				remainingTime,
				required - psuccess,
				fp);
		}
		else if ((loc > 16) && (remainingTime > 0))
		{
			Com_sprintf(string, sizeof(string),
				"xv 0 yv 8 picn field_3 "							//background
				"xv 100 yv 8 picn k_datacd "						//mini picture
				"xv 70 yv 160 string2 \"%s\" "						//bar
				"xv 120 yv 180 string2 \"Time Remaining: %.2f\" "	//time remaining
				"xv 140 yv 210 string2 \"Yoinks Required: %i\" "	//successful pulls remaining
				"xv 150 yv 230 string2 \"Fish Pulls: %i\" ",		//fish pulls

				bar,
				remainingTime,
				required - psuccess,
				fp);
		}
		else if ((loc == 16) && (remainingTime>0) )
		{
			Com_sprintf(string, sizeof(string),
				"xv 0 yv 8 picn field_3 "							//background
				"xv 100 yv 8 picn tag1 "						//mini picture
				"xv 70 yv 160 string2 \"%s\" "						//bar
				"xv 120 yv 180 string2 \"Time Remaining: %.2f\" "	//time remaining
				"xv 115 yv 190 string2 \"Press [SPACE] to Yoink\" "
				"xv 140 yv 210 string2 \"Yoinks Required: %i\" "	//successful pulls remaining
				"xv 150 yv 230 string2 \"Fish Pulls: %i\" ",		//fish pulls

				bar,
				remainingTime,
				required - psuccess,
				fp);
		}
		else if (remainingTime<=0)
		{
			if (psuccess>=required)
			{
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn field_3 "							//background
					"xv 100 yv 8 picn tag1 "						//mini picture
					"xv 70 yv 160 string2 \"%s\" "						//bar
					"xv 120 yv 180 string2 \"You Yoinked the Fish\" "	//time remaining
					"xv 70 yv 210 string2 \"Press [ENTER] to See Your Catch\" "	//successful pulls remaining
					"xv 150 yv 230 string2 \"Fish Pulls: %i\" ",		//fish pulls

					bar,
					fp);
			}
			else
			{
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn field_3 "							//background
					"xv 100 yv 8 picn tag2 "						//mini picture
					"xv 70 yv 160 string2 \"%s\" "						//bar
					"xv 90 yv 180 string2 \"Out of Time the Fish Got Away\" "	//time remaining
					"xv 100 yv 210 string2 \"PRESS [ENTER] to Continue\" "	//successful pulls remaining
					"xv 150 yv 230 string2 \"Fish Pulls: %i\" ",		//fish pulls

					bar,
					fp);
			}
		}
		else
		{
			Com_sprintf(string, sizeof(string),
				"xv 0 yv 8 picn field_3 "							//background
				"xv 70 yv 160 string2 \"ERROR ERROR ERROR\" "						
				);
		}


		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);

	}

	//photo
	if (mode == 'j')
	{
		Com_sprintf(string, sizeof(string),
			"xv 32 yv 8 picn victory "						//background
		);

		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);
	}

	//information
	if (mode == 'i')
	{
		int curPage = page->value;
		char* line1;
		char* line2;
		char* line3;
		char* line4;
		char* line5;
		char* line6;
		char* line7;
		char* line8;
		char* line9;
		char* line10;
		char* line11;
		char* line12;
		char* line13;
		char* line14;
		char* line15;
		char* line16;
		char* line17;
		char* line18;
		char* line19;
		char* line20;
		char* line21;
		char* line22;
		char* line23;

		switch (curPage)
		{
			case (1):
				line1 = "(Table of Contents)";
				line2 = "";
				line3 = "1. Table of Contents";
				line4 = "";
				line5 = "2. Buttons";
				line6 = "";
				line7 = "3. General Information";
				line8 = "";
				line9 = "4. Buy Menu Information";
				line10 = "";
				line11 = "5. Equipment Details";
				line12 = "";
				line13 = "6. Mini Game Details";
				line14 = "";
				line15 = "7. Math";
				line16 = "";
				line17 = "8. Math II";
				line18 = "";
				line19 = "";
				line20 = "";
				line21 = "Press[M] to go to the next page of this menu";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = " _";
				break;

			case (2):
				line1 = "(Buttons)";
				line2 = "[F1] to open and close this menu";
				line3 = "[~] to open and close the command prompt";
				line4 = "";
				line5 = "[TAB] to open and close the buy menu";
				line6 = "[P] cycle pole";
				line7 = "[R] reel";
				line8 = "[B] bait";
				line9 = "[H] hook";
				line10 = "[W] weight";
				line11 = "[T] tackle";
				line12 = "";
				line13 = "[C] will cast a line";
				line14 = "[=] will bring up the mini game if paused";
				line15 = "[[] will pull your line to the left";
				line16 = "[]] will pull your line to the right";
				line17 = "[SPACE] will yoink the fish if centered";
				line18 = "[ENTER] will continue";
				line19 = "";
				line20 = "Press[N] to go to the previous page of this menu.";
				line21 = "Press[M] to go to the next page of this menu.";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "   _";
				break;

			case (3):
				line1 = "(General Information)";
				line2 = "";
				line3 = "Type 'fish' into the command prompt to activate fishing";
				line4 = "";
				line5 = "You automatically you have your fishing pole";
				line6 = "and get set up";
				line7 = "";
				line8 = "If your cast lands in water";
				line9 = "and within the range of your pole";
				line10 = "you will activate a minigame to catch a fish";
				line11 = "";
				line12 = "";
				line13 = "";
				line14 = "If your cast is within the range of your pole";
				line15 = "you will activate a minigame to catch a fish";
				line16 = "";
				line17 = "";
				line18 = "";
				line19 = "";
				line20 = "Press[N] to go to the previous page of this menu.";
				line21 = "Press[M] to go to the next page of this menu.";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "     _";
				break;
			
			case (4):
				line1 = "(Buy Menu)";
				line2 = "";
				line3 = "In the buy menu you will see 6 items";
				line4 = "and a price to upgrade them.";
				line5 = "";
				line6 = "";
				line7 = "";
				line8 = "There are 3 levels for each item";
				line9 = "";
				line10 = "If you have the required cash";
				line11 = "for the upgrade pressing the";
				line12 = "key associated with the item ";
				line13 = "will purchase the upgrade.";
				line14 = "";
				line15 = "";
				line16 = "";
				line17 = "";
				line18 = "";
				line19 = "";
				line20 = "Press[N] to go to the previous page of this menu.";
				line21 = "Press[M] to go to the next page of this menu.";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "       _";
				break;
				
				break;

			case (5):
				line1 = "(Equipment Details)";
				line2 = "";
				line3 = "The Pole affects how far you";
				line4 = "are able to cast your line";
				line5 = "";
				line6 = "The Reel affects how long";
				line7 = "you have to catch a fish";
				line8 = "";
				line9 = "The Bait affects the rarity";
				line10 = "of the fish you can catch";
				line11 = "";
				line12 = "The Hook affects how many succesful";
				line13 = "pulls are needed to catch a fish";
				line14 = "";
				line15 = "The Weight affects the size of";
				line16 = "the fish you are able to catch";
				line17 = "";
				line18 = "The Tackle affects how much";
				line19 = "your fish are worth";
				line20 = "Press[N] to go to the previous page of this menu";
				line21 = "Press[M] to go to the next page of this menu";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "         _";
				break;

			case (6):
				line1 = "(MiniGame Details)";
				line2 = "";
				line3 = "Your objective is to pull your line";
				line4 = "towards the center and when it is";
				line5 = "centered Yoink the fish";
				line6 = "";
				line7 = "The fish will fight back ";
				line8 = "and drag your line all over";
				line9 = "";
				line10 = "You will be able to tell which way to pull";
				line11 = "based on the image in the center of the mini game";
				line12 = "";
				line13 = "";
				line14 = "";
				line15 = "";
				line16 = "";
				line17 = "";
				line18 = "";
				line19 = "";
				line20 = "Press[N] to go to the previous page of this menu.";
				line21 = "Press[M] to go to the next page of this menu.";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "           _";
				break;

			case (7):
				line1 = "(Math)";
				line2 = "";
				line3 = "There are 5 types of fish";
				line4 = "they each have their own spawn";
				line5 = "conditions and value";
				line6 = "[Distance,Weight,Bait]";
				line7 = "";
				line8 = "Mackerel[0,0,0] $20";
				line9 = "";
				line10 = "Cod[0,0,0] $40";
				line11 = "";
				line12 = "Sword Fish[1,0,1 $60";
				line13 = "";
				line14 = "Rainbow Fish[2,1,2] $80";
				line15 = "";
				line16 = "Blue Fin Tuna [2,2,2] $100";
				line17 = "";
				line18 = "";
				line19 = "";
				line20 = "Press[N] to go to the previous page of this menu";
				line21 = "Press[M] to go to the next page of this menu.";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "             _";
				break;

			case (8):
				line1 = "(Math II)";
				line2 = "";
				line3 = "The fish have a (1+5*fishindex)% ";
				line4 = "chance to pull either direction ";
				line5 = "while reeling";
				line6 = "";
				line7 = "Maximum cash is 999";
				line8 = "";
				line9 = "The tackle gives a mulitplier bonus";
				line10 = "to the base price of a fish";
				line11 = "";
				line12 = "";
				line13 = "";
				line14 = "";
				line15 = "";
				line16 = "You can spawn fish by typing 'spawn___'";
				line17 = "in the command prompt filling in the blank";
				line18 = "";
				line19 = "";
				line20 = "";
				line21 = "Press[N] to go to the previous page of this menu";
				line22 = "(1 2 3 4 5 6 7 8)";
				line23 = "               _";
				break;

			default:
				line1 = "FAILSAFE";
				line2 = "FAILSAFE";
				line3 = "FAILSAFE";
				line4 = "FAILSAFE";
				line5 = "FAILSAFE";
				line6 = "FAILSAFE";
				line7 = "FAILSAFE";
				line8 = "FAILSAFE";
				line9 = "FAILSAFE";
				line10 = "FAILSAFE";
				line11 = "FAILSAFE";
				line12 = "FAILSAFE";
				line13 = "FAILSAFE";
				line14 = "FAILSAFE";
				line15 = "FAILSAFE";
				line16 = "FAILSAFE";
				line17 = "FAILSAFE";
				line18 = "FAILSAFE";
				line19 = "FAILSAFE";
				line20 = "FAILSAFE";
				line21 = "FAILSAFE";
				line22 = "FAILSAFE";
				line23 = "FAILSAFE";
				break;
		}

		Com_sprintf(string, sizeof(string),
			"xv 0 yv 8 picn help "			//background
			"xv 0 yv 10 string2 \"%s\" "	//(header)
			"xv 0 yv 20 string2 \"%s\" "	//line 2
			"xv 0 yv 30 string2 \"%s\" "	//line 3
			"xv 0 yv 40 string2 \"%s\" "	//line 4
			"xv 0 yv 50 string2 \"%s\" "	//line 5
			"xv 0 yv 60 string2 \"%s\" "	//line 6
			"xv 0 yv 70 string2 \"%s\" "	//line 7
			"xv 0 yv 80 string2 \"%s\" "	//line 8
			"xv 0 yv 90 string2 \"%s\" "	//line 9
			"xv 0 yv 100 string2 \"%s\" "	//line 10
			"xv 0 yv 110 string2 \"%s\" "	//line 11
			"xv 0 yv 120 string2 \"%s\" "	//line 12
			"xv 0 yv 130 string2 \"%s\" "	//line 13
			"xv 0 yv 140 string2 \"%s\" "	//line 14
			"xv 0 yv 150 string2 \"%s\" "	//line 15
			"xv 0 yv 160 string2 \"%s\" "	//line 16
			"xv 0 yv 170 string2 \"%s\" "	//line 17
			"xv 0 yv 180 string2 \"%s\" "	//line 18
			"xv 0 yv 190 string2 \"%s\" "	//line 19
			"xv 0 yv 270 string2 \"%s\" "	//prev page
			"xv 0 yv 280 string2 \"%s\" "	//next page
			"xv 0 yv 300 string2 \"%s\" "	//page nums
			"xv 0 yv 302 string2 \"%s\" ",	//underscore location 

			line1,
			line2,
			line3,
			line4,
			line5,
			line6,
			line7,
			line8,
			line9,
			line10,
			line11,
			line12,
			line13,
			line14,
			line15,
			line16,
			line17,
			line18,
			line19,
			line20,
			line21,
			line22,
			line23);

		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);
		

		//succesful catch
		if (mode == 's')
		{
			Com_sprintf(string, sizeof(string),
				"xv 32 yv 8 picn help "						//background
				"xv 32 yv 20 cstring2 \"Congratulations you caught the fish!\""

			);

			gi.WriteByte(svc_layout);
			gi.WriteString(string);
			gi.unicast(ent, true);
		}

	}
	
	//endscreen
	if (mode == 'e')
	{
		int fish = minifish->value;
		int tkle = tackle->value;
		int fp = fishpulls->value;

		switch (fish)
		{
			case(0):
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn tech3 "						//background
					"xv 20 yv 20 string2 \"You caught a Mackerel\" "
					"xv 20 yv 30 string2 \"You have been rewarded $%i for your catch\" "
					"xv 20 yv 40 string2 \"Press [ENTER] to continue\" "
					"xv 100 yv 50 picn tech1 ",


					(fish+1)*20 * (tkle + 1),
					fishpulls->value);
				break;

			case(1):
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn tech3 "						//background
					"xv 20 yv 20 string2 \"You caught a Cod\" "
					"xv 20 yv 30 string2 \"You have been rewarded $%i for your catch\" "
					"xv 20 yv 40 string2 \"Press [ENTER] to continue\" "
					"xv 100 yv 50 picn ctfsb1 ",
					

					(fish + 1) * 20 * (tkle + 1),
					fp);
				break;

			case(2):
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn tech3 "						//background
					"xv 20 yv 20 string2 \"You caught a Swordfish\" "
					"xv 20 yv 30 string2 \"You have been rewarded $%i for your catch\" "
					"xv 20 yv 40 string2 \"Press [ENTER] to continue\" "
					"xv 100 yv 50 picn ctfsb2 ",
					

					(fish + 1) * 20 * (tkle + 1),
					fp);
				break;

			case(3):
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn tech3 "						//background
					"xv 20 yv 20 string2 \"You caught a Rainbow Fish\" "
					"xv 20 yv 30 string2 \"You have been rewarded $%i for your catch\" "
					"xv 20 yv 40 string2 \"Press [ENTER] to continue\" "
					"xv 100 yv 50 picn sbfctf1 ",
					

					(fish + 1) * 20 * (tkle + 1),
					fp);
				break;

			case(4):
				Com_sprintf(string, sizeof(string),
					"xv 0 yv 8 picn tech3 "						//background
					"xv 20 yv 20 string2 \"You caught a Blue Fin Tuna\" "
					"xv 10 yv 30 string2 \"You have been rewarded $%i for your catch\" "
					"xv 20 yv 40 string2 \"Press [ENTER] to continue\" "
					"xv 100 yv 50 picn sbfctf2 ",
					

					(fish + 1) * 20 * (tkle + 1),
					fp);
				break;
		}
		

		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);
	}

	//startscreen
	if (mode == 's')
	{

		Com_sprintf(string, sizeof(string),
			"xv 0 yv 8 picn tech4 "						//background
			"xv 0 yv 10 cstring2 \"Fish On!\" "
			"xv 0 yv 12 cstring2 \"________\" "
			"xv 192 yv 270 string2 \"Press [ENTER] to continue\" "

		
		);

		gi.WriteByte(svc_layout);
		gi.WriteString(string);
		gi.unicast(ent, true);
	}
}


//call the info menu
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability not necessary for mod
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	//must be set for each Cmd_Help_f it makes sure the inventory and score overlay are closed
	ent->client->showinventory = false;
	ent->client->showscores = false;

	//this is different for each Cmd and should turn all the menus not open to the closed flag
	ent->client->showmini = false;
	ent->client->showbuy = false;
	ent->client->showend = false;
	ent->client->showstart = false;

	//this is for when the button is pressed again this means we should close the menu and adjust the flags
	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		//command to close overlay
		ent->client->showhelp = false;
		//command to set this flag to false
		ent->client->showinfo = false;
		return;
	}

	//set to true to show the overlay
	ent->client->showhelp = true;
	//set to true to set this flag to true
	ent->client->showinfo = true;
	//extra i dont want to touch
	ent->client->pers.helpchanged = 0;

	//call HelpComputer with the right mode
	HelpComputer (ent,'i');
}

//call the buy menu
void Cmd_HelpBuy_f(edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;
	
	ent->client->showmini = false;
	ent->client->showinfo = false;
	ent->client->showend = false;
	ent->client->showstart = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		ent->client->showbuy = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showbuy = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer(ent, 'b');
}

//call the mini game
void Cmd_HelpMini_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;

	ent->client->showbuy = false;
	ent->client->showinfo = false;
	ent->client->showend = false;
	ent->client->showstart = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		ent->client->showmini = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showmini = true;
	ent->client->pers.helpchanged = 0;

	HelpComputer(ent, 'm');
}

//call my picture menu
void Cmd_HelpJoe_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;

	ent->client->showbuy = false;
	ent->client->showinfo = false;
	ent->client->showend = false;
	ent->client->showmini = false;
	ent->client->showstart = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer(ent, 'j');
}

//call start menu
void Cmd_HelpStart_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;

	ent->client->showbuy = false;
	ent->client->showinfo = false;
	ent->client->showmini = false;
	ent->client->showend = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		ent->client->showstart = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showstart = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer(ent, 's');
}

//call end menu
void Cmd_HelpEnd_f(edict_t* ent)
{
	ent->client->showinventory = false;
	ent->client->showscores = false;

	ent->client->showbuy = false;
	ent->client->showinfo = false;
	ent->client->showmini = false;
	ent->client->showstart = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		ent->client->showend = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->showend = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer(ent, 'e');
}

//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
	int			index, cells;
	int			power_armor_type;

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->cash;

	//
	// ammo
	//
	if (!ent->client->ammo_index /* || !ent->client->pers.inventory[ent->client->ammo_index] */)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}
	
	//
	// armor
	//
	power_armor_type = PowerArmorType (ent);
	if (power_armor_type)
	{
		cells = ent->client->pers.inventory[ITEM_INDEX(FindItem ("cells"))];
		if (cells == 0)
		{	// ran out of cells for power armor
			ent->flags &= ~FL_POWER_ARMOR;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			power_armor_type = 0;;
		}
	}

	index = ArmorIndex (ent);
	if (power_armor_type && (!index || (level.framenum & 8) ) )
	{	// flash between power armor and other armor icon
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex ("i_powershield");
		ent->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index)
	{
		item = GetItemByIndex (index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_invulnerability");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_envirosuit");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum)/10;
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_rebreather");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum)/10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex (itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->pers.helpchanged && (level.framenum&8) )
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex ("i_help");
	else if ( (ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91)
		&& ent->client->pers.weapon)
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
	else
		ent->client->ps.stats[STAT_HELPICON] = 0;

	ent->client->ps.stats[STAT_SPECTATOR] = 0;
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + 
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}

//alex
