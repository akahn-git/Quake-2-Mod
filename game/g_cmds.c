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
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);

	//set grapple flag to in use
	if (strcmp(s, "Grapple") == 0)
	{
		ent->client->grapuse = true;
	}
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
	ent->client->grapuse = false;

	if (strcmp(s, "Grapple") == 0)
	{
		ent->client->grapuse = true;
	}
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;
	char	string[1024];

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	Com_sprintf(string, sizeof(string), 
		"xv 32 yv 8 picn help"
		"xv -70 yv 30 cstring2 \"Testing\" ");

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		sprintf(st, "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

//alex

void GiveCash(edict_t	*ent, int num)
{
	int cur = ent->cash;
	if (cur + num > ent->maxcash)
	{
		ent->cash = ent->maxcash;
		return;
	}

	else
	{
		ent->cash += num;
	}
}
void Cmd_GiveMaxCash_f(edict_t	*ent)
{
	GiveCash(ent, 1000);
}

//to use any of the upgrade commands the help computer must be visible
//the upgrade part should first check which pole is currently equipt, 
//then see if the next pole is purchased or available for purchase,
//if the next pole is purchased then switch to the next pole and return; else check to see if cash is sufficient for upgrade
//if cash is sufficient upgrade the part based on the char specified
void UpgradePart(edict_t	*ent, char part)
{
	int cur;
	int next;
	int ismax; //0 is false, 1 is true
	int nextUpgrade;


	if (!ent->client->showhelp || !ent->client->showbuy)
	{
		return;
	}

	switch (part)
	{
		case ('p'):
			cur = pole->value;
			ismax = maxpole->value;
			break;

		case ('r'):
			cur = reel->value;
			ismax = maxreel->value;
			break;

		case ('h'):
			cur = hook->value;
			ismax = maxhook->value;
			break;

		case ('w'):
			cur = weight->value;
			ismax = maxweight->value;
			break;

		case ('b'):
			cur = bait->value;
			ismax = maxbait->value;
			break;

		case ('t'):
			cur = tackle->value;
			ismax = maxtackle->value;
			break;

		default:
			cur = 0;
			ismax = 0;
	}

	next = cur + 1;
	nextUpgrade = 50 * next;

	if (ismax == 1)
	{
		if (next == 3)
		{
			next = 0;
		}

		switch (part)
		{
			case ('p'):
				pole->value=next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('r'):
				reel->value = next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('h'):
				hook->value = next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('w'):
				weight->value = next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('b'):
				bait->value = next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('t'):
				tackle->value = next;

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			default:
				break;
		}
	}

	else
	{

		switch (part)
		{
			case ('p'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					pole->value = next;

					if (next == 2)
					{
						maxpole->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Pole upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Pole upgrade\n");
				}

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('r'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					reel->value = next;

					if (next == 2)
					{
						maxreel->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Reel upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Reel upgrade\n");
				}

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('h'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					hook->value = next;

					if (next == 2)
					{
						maxhook->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Hook upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Hook upgrade\n");
				}

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('w'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					weight->value = next;

					if (next == 2)
					{
						maxweight->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Weight upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Weight upgrade\n");
				}

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('b'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					bait->value = next;

					if (next == 2)
					{
						maxbait->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Bait upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Bait upgrade\n");
				}
				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			case ('t'):
				if (ent->cash >= nextUpgrade)
				{
					ent->cash -= nextUpgrade;
					tackle->value = next;

					if (next == 2)
					{
						maxtackle->value = 1;
					}

					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Tackle upgrade purchased\n");
				}

				else
				{
					gi.multicast(ent->s.origin, MULTICAST_PVS);
					gi.bprintf(PRINT_HIGH, "Insufficent funds can not buy Tackle upgrade\n");
				}

				Cmd_HelpBuy_f(ent);
				Cmd_HelpBuy_f(ent);
				break;

			default:
				break;
		}
	}
	
}

void Cmd_UpgradePole_f(edict_t	*ent)
{
	UpgradePart(ent, 'p');
}

void Cmd_UpgradeReel_f(edict_t	*ent)
{
	UpgradePart(ent, 'r');
}

void Cmd_UpgradeHook_f(edict_t	*ent)
{
	UpgradePart(ent, 'h');
}

void Cmd_UpgradeWeight_f(edict_t	*ent)
{
	UpgradePart(ent, 'w');
}

void Cmd_UpgradeBait_f(edict_t	*ent)
{
	UpgradePart(ent, 'b');
}

void Cmd_UpgradeTackle_f(edict_t	*ent)
{
	UpgradePart(ent, 't');
}

void Cmd_LeftPull_f(edict_t	*ent)
{
	if (!ent->client->showhelp || !ent->client->showmini || ent->client->miniover)
	{
		return;
	}

	int loc = olocation->value;
	int pulled = loc - 1;

	if (pulled == 0)
	{
		pulled = 1;
	}
	olocation->value = pulled;

	Cmd_HelpMini_f(ent);
	Cmd_HelpMini_f(ent);
}

void Cmd_RightPull_f(edict_t* ent)
{
	if (!ent->client->showhelp || !ent->client->showmini || ent->client->miniover)
	{
		return;
	}

	int loc = olocation->value;
	int pulled = loc + 1;

	if (pulled == 32)
	{
		pulled = 31;
	}
	olocation->value = pulled;

	Cmd_HelpMini_f(ent);
	Cmd_HelpMini_f(ent);
}

void Cmd_Yoink_f(edict_t* ent)
{
	if (!ent->client->showhelp || !ent->client->showmini || ent->client->miniover)
	{
		return;
	}

	if (olocation->value == 16)
	{
		pulls->value = pulls->value + 1;
		//redetermine starting location
		int r = 1 + (rand() % 32);
		if (r == 16)
		{
			//cant start at middle so start at 1
			r = 1;
		}
		olocation->value = r;
	}

	Cmd_HelpMini_f(ent);
	Cmd_HelpMini_f(ent);
}

void Cmd_LastPage_f(edict_t* ent)
{
	if (!ent->client->showhelp || !ent->client->showinfo)
	{
		return;
	}

	int pg = page->value;
	int prev = pg - 1;

	if (prev == 0)
	{
		prev = 1;
	}
	page->value = prev;


	Cmd_Help_f(ent);
	Cmd_Help_f(ent);
	
}

void Cmd_NextPage_f(edict_t* ent)
{
	if (!ent->client->showhelp || !ent->client->showinfo)
	{
		return;
	}

	int pg = page->value;
	int next = pg + 1;


	if (next == 9)
	{
		next = 8;
	}
	page->value = next;

	
	Cmd_Help_f(ent);
	Cmd_Help_f(ent);
	
}

//will assume no invalid calls where gi.args() comes in the form pg#
void Cmd_SwitchPage_f(edict_t* ent)
{
	if (!ent->client->showhelp || !ent->client->showinfo)
	{
		return;
	}

	char* pg = gi.args();
	char pgnum = pg[2];
	
	switch (pgnum)
	{
		case ('1'):
			page->value = 1;
			break;

		case ('2'):
			page->value = 2;
			break;

		case ('3'):
			page->value = 3;
			break;

		case ('4'):
			page->value = 4;
			break;

		case ('5'):
			page->value = 5;
			break;

		case ('6'):
			page->value = 6;
			break;

		case ('7'):
			page->value = 7;
			break;

		case ('8'):
			page->value = 8;
			break;

	}

	Cmd_Help_f(ent);
	Cmd_Help_f(ent);
}

int getFish(edict_t* ent)
{
	int fish = 0;
	int dist;
	int size;
	int rare;

	//mack and cod are always present
	int possible2[2] = { 0,1 };

	int possible3[3] = { 0,0,0 };
	int possible4[4] = { 0,0,0,0 };
	int possible5[5] = { 0,0,0,0,0 };

	qboolean mackFlag = true;
	qboolean codFlag = true;
	qboolean swordFlag = true;
	qboolean rainbFlag = true;
	qboolean tunaFlag = true;

	//mack and cod are always present
	int flagCount = 2;
	//used to check which fish should be put into the array
	int cur=0;

	float x = minidist->value;
	if (x > 550)
	{
		dist = 2;
	}
	else if (x > 350)
	{
		dist = 1;
	}
	else
	{
		dist = 0;
	}

	size = weight->value;
	rare = bait->value;

	
	//check dist
	if (dist < 1)
	{
		swordFlag = false;
		rainbFlag = false;
		tunaFlag = false;
	}
	else if (dist < 2)
	{
		rainbFlag = false;
		tunaFlag = false;
	}

	//check size
	if (size < 1)
	{
		rainbFlag = false;
		tunaFlag = false;
	}
	else if (size < 2)
	{
		tunaFlag = false;
	}
	//check rare
	if (rare < 1)
	{
		swordFlag = false;
		rainbFlag = false;
		tunaFlag = false;
	}
	else if (rare < 2)
	{
		rainbFlag = false;
		tunaFlag = false;
	}
	
	//set the flag count
	if (swordFlag)
	{
		flagCount++;
	}
	if (rainbFlag)
	{
		flagCount++;
	}
	if (tunaFlag)
	{
		flagCount++;
	}

	qboolean fishFlags[5] = { mackFlag,codFlag,swordFlag,rainbFlag,tunaFlag };
	int fishNums[5] = { 0,1,2,3,4 };

	//initialize the correct array
	for (int i = 0;i < flagCount;i++)
	{
		if (!fishFlags[cur])
		{
			cur++;

		}
		else
		{
			switch (flagCount)
			{

				case(2):
					possible2[i]=fishNums[cur];
					break;

				case(3):
					possible3[i] = fishNums[cur];
					break;

				case(4):
					possible4[i] = fishNums[cur];
					break;

				case(5):
					possible5[i] = fishNums[cur];
					break;

			}
			cur++;
		}
	}

	int pick = rand() % flagCount;
	switch (flagCount)
	{

		case(2):
			fish=possible2[pick];
			break;

		case(3):
			fish = possible3[pick];
			break;

		case(4):
			fish = possible4[pick];
			break;

		case(5):
			fish = possible5[pick];
			break;

	}
	
	

	
	return fish;
}


//draws the whole mini game
void Cmd_MiniGame_f(edict_t* ent)
{
	//first should show the start screen
	//then would show the mini game
	//then if won show end screen

	int pg = minipage->value;

	if (pg == 1)
	{
		Cmd_HelpStart_f(ent);
	}

	else if (pg == 2)
	{
		Cmd_HelpMini_f(ent);
	}

	else if (pg == 3)
	{
		Cmd_HelpEnd_f(ent);
	}
}

//button to change through mini screens
void Cmd_ChangeMiniScreen_f(edict_t* ent)
{
	//you need to be in at least one of the screens associated with the minigame - technically the 3 screens can never all be true at the same time
	if (!ent->client->showhelp || (!ent->client->showmini && !ent->client->showend && !ent->client->showstart))
	{
		return;
	}

	int pg = minipage->value;

	//if you are on the first page you do not need a check, increment and call MiniGame(), set the variables for the mini game in this call
	//if you are on the minigame you need to check if the mini game is over to press this button, and if the mini game is over decide what to do, make sure to reset variables if closing screen
	//if you are on the end page you should close the mini game and reset the page and variables

	//first page
	if (pg == 1)
	{
		int baseReel = 30;

		//determines the reel time
		if (reel->value == 0)
		{
			minireel->value = 1 * baseReel;
		}
		else if (reel->value == 1)
		{
			minireel->value = 2 * baseReel;
		}
		else if (reel->value == 2)
		{
			minireel->value = 3 * baseReel;
		}
		else
		{
			minireel->value = 4 * baseReel;
		}

		//determine starting location
		int r = 1 +(rand() % 32);
		if (r == 16)
		{
			//cant start at middle so start at 1
			r = 1;
		}
		if (r == 32)
		{
			//index out of range
			r = 31;
		}
		olocation->value = r;

		minitime->value = level.time;
		minifish->value = getFish(ent);

		//if specific fish requested use that
		if (minifishspecific->value > -1)
		{
			minifish->value = minifishspecific->value;
		}

		minipage->value = minipage->value + 1;
		Cmd_MiniGame_f(ent);
		Cmd_MiniGame_f(ent);
	}

	//second page
	else if (pg == 2)
	{
		//if the mini game is over and you won the mini game
		if (ent->client->miniover && ent->client->wonmini)
		{
			minipage->value = minipage->value + 1;

			int reward = (minifish->value+1) * 20 * (tackle->value + 1);
			GiveCash(ent,reward);

			Cmd_MiniGame_f(ent);
			Cmd_MiniGame_f(ent);
		}

		//if the mini game is over and you didnt win 
		else if (ent->client->miniover)
		{
			minipage->value = 1;
			pulls->value = 0;
			fishpulls->value = 0;
			minifishspecific->value = -1;
			ent->client->miniover = false;
			ent->client->wonmini = false;
			Cmd_HelpMini_f(ent);
		}

		//
		else
		{
			return;
		}
	}

	//third page
	else if (pg == 3)
	{
		Cmd_HelpEnd_f(ent);
		minipage->value = 1;
		pulls->value = 0;
		fishpulls->value = 0;
		minifishspecific->value = -1;
		ent->client->miniover = false;
		ent->client->wonmini = false;
	}

}

//specific fish spawns
void Cmd_SpawnMack_f(edict_t* ent)
{
	minifishspecific->value = 0;
	Cmd_MiniGame_f(ent);
}

void Cmd_SpawnCod_f(edict_t* ent)
{
	minifishspecific->value = 1;
	Cmd_MiniGame_f(ent);
}

void Cmd_SpawnSword_f(edict_t* ent)
{
	minifishspecific->value = 2;
	Cmd_MiniGame_f(ent);
}

void Cmd_SpawnRainb_f(edict_t* ent)
{
	minifishspecific->value = 3;
	Cmd_MiniGame_f(ent);
}

void Cmd_SpawnTuna_f(edict_t* ent)
{
	minifishspecific->value = 4;
	Cmd_MiniGame_f(ent);
}

void Cmd_GodGear_f(edict_t	*ent)
{
	pole->value = 4;
	reel->value = 4;
	hook->value = 4;
	weight->value = 4;
	bait->value = 4;
	tackle->value = 4;
}

void Cmd_ResetGear_f(edict_t* ent)
{
	pole->value = 0;
	reel->value = 0;
	hook->value = 0;
	weight->value = 0;
	bait->value = 0;
	tackle->value = 0;
}

void Cmd_GiveGrapple_f(edict_t* ent)
{
	char* name;
	gitem_t* it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t* it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = "NONE";

	
	give_all = true;

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo(ent, it, 1000);
		}
		if (!give_all)
			return;
	}
}

//used for init
void Cmd_UseFishing_f(edict_t* ent)
{
	int	index;
	gitem_t* it;
	char* s;

	s = "Grapple";
	it = FindItem(s);

	if (!it)
	{
		gi.cprintf(ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf(ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use(ent, it);
	ent->client->grapuse = true;
}

//-------------------Grapple---------------------

static void P_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}

// self is grapple, not player
void ResetGrapple(edict_t* self)
{
	if (self->owner->client->ctf_grapple) {
		float volume = 1.0;
		gclient_t* cl;

		if (self->owner->client->silencer_shots)
			volume = 0.2;

		gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON, gi.soundindex("weapons/grapple/grreset.wav"), volume, ATTN_NORM, 0);
		cl = self->owner->client;
		cl->ctf_grapple = NULL;
		cl->ctf_grapplereleasetime = level.time;
		cl->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
		cl->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
		G_FreeEdict(self);
	}
}
// ent is player
void PlayerResetGrapple(edict_t* ent)
{
	if (ent->client && ent->client->ctf_grapple)
		CTFResetGrapple(ent->client->ctf_grapple);
}
void GrappleTouch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->ctf_grapplestate != CTF_GRAPPLE_STATE_FLY)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		CTFResetGrapple(self);
		return;
	}

	VectorCopy(vec3_origin, self->velocity);

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage) {
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, 0, MOD_GRAPPLE);
		CTFResetGrapple(self);
		return;
	}

	self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

	if (self->owner->client->silencer_shots)
		volume = 0.2;

	gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON, gi.soundindex("weapons/grapple/grpull.wav"), volume, ATTN_NORM, 0);
	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/grapple/grhit.wav"), volume, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(self->s.origin);
	if (!plane)
		gi.WriteDir(vec3_origin);
	else
		gi.WriteDir(plane->normal);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void GrappleDrawCable(edict_t* self)
{

	gi.multicast(self->s.origin, MULTICAST_PVS);
	gi.bprintf(PRINT_HIGH, "In draw cable\n");

	vec3_t	offset, start, end, f, r;
	vec3_t	dir;
	float	distance;

	AngleVectors(self->owner->client->v_angle, f, r, NULL);
	VectorSet(offset, 16, 16, self->owner->viewheight - 8);
	P_ProjectSource(self->owner->client, self->owner->s.origin, offset, f, r, start);

	VectorSubtract(start, self->owner->s.origin, offset);

	VectorSubtract(start, self->s.origin, dir);
	distance = VectorLength(dir);
	// don't draw cable if close
	if (distance < 64)
		return;

#if 0
	if (distance > 256)
		return;

	// check for min/max pitch
	vectoangles(dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 45)
		return;

	trace_t	tr; //!!

	tr = gi.trace(start, NULL, NULL, self->s.origin, self, MASK_SHOT);
	if (tr.ent != self) {
		CTFResetGrapple(self);
		return;
	}
#endif

	// adjust start for beam origin being in middle of a segment
//	VectorMA (start, 8, f, start);

	VectorCopy(self->s.origin, end);
	// adjust end z for end spot since the monster is currently dead
//	end[2] = self->absmin[2] + self->size[2] / 2;

	gi.WriteByte(svc_temp_entity);
#if 1 //def USE_GRAPPLE_CABLE
	gi.WriteByte(TE_GRAPPLE_CABLE);
	gi.WriteShort(self->owner - g_edicts);
	gi.WritePosition(self->owner->s.origin);
	gi.WritePosition(end);
	gi.WritePosition(offset);
#else
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(end);
	gi.WritePosition(start);
#endif
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

void SV_AddGravity(edict_t* ent);

void GrapplePull(edict_t* self)
{
	vec3_t hookdir, v;
	float vlen;

	if (strcmp(self->owner->client->pers.weapon->classname, "weapon_grapple") == 0 &&
		!self->owner->client->newweapon &&
		self->owner->client->weaponstate != WEAPON_FIRING &&
		self->owner->client->weaponstate != WEAPON_ACTIVATING) {
		CTFResetGrapple(self);
		return;
	}

	if (self->enemy) {
		if (self->enemy->solid == SOLID_NOT) {
			CTFResetGrapple(self);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX) {
			VectorScale(self->enemy->size, 0.5, v);
			VectorAdd(v, self->enemy->s.origin, v);
			VectorAdd(v, self->enemy->mins, self->s.origin);
			gi.linkentity(self);
		}
		else
			VectorCopy(self->enemy->velocity, self->velocity);
		if (self->enemy->takedamage &&
			!CheckTeamDamage(self->enemy, self->owner)) {
			float volume = 1.0;

			if (self->owner->client->silencer_shots)
				volume = 0.2;

			T_Damage(self->enemy, self, self->owner, self->velocity, self->s.origin, vec3_origin, 1, 1, 0, MOD_GRAPPLE);
			gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/grapple/grhurt.wav"), volume, ATTN_NORM, 0);
		}
		if (self->enemy->deadflag) { // he died
			CTFResetGrapple(self);
			return;
		}
	}

	CTFGrappleDrawCable(self);

	if (self->owner->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY) {
		// pull player toward grapple
		// this causes icky stuff with prediction, we need to extend
		// the prediction layer to include two new fields in the player
		// move stuff: a point and a velocity.  The client should add
		// that velociy in the direction of the point
		vec3_t forward, up;

		AngleVectors(self->owner->client->v_angle, forward, NULL, up);
		VectorCopy(self->owner->s.origin, v);
		v[2] += self->owner->viewheight;
		VectorSubtract(self->s.origin, v, hookdir);

		vlen = VectorLength(hookdir);

		if (self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL &&
			vlen < 64) {
			float volume = 1.0;

			if (self->owner->client->silencer_shots)
				volume = 0.2;

			self->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			gi.sound(self->owner, CHAN_RELIABLE + CHAN_WEAPON, gi.soundindex("weapons/grapple/grhang.wav"), volume, ATTN_NORM, 0);
			self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_HANG;
		}

		VectorNormalize(hookdir);
		VectorScale(hookdir, CTF_GRAPPLE_PULL_SPEED, hookdir);
		VectorCopy(hookdir, self->owner->velocity);
		SV_AddGravity(self->owner);
	}
}

void FireGrapple(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{

	edict_t* grapple;
	trace_t	tr;

	VectorNormalize(dir);

	grapple = G_Spawn();
	VectorCopy(start, grapple->s.origin);
	VectorCopy(start, grapple->s.old_origin);
	vectoangles(dir, grapple->s.angles);
	VectorScale(dir, speed, grapple->velocity);
	grapple->movetype = MOVETYPE_FLYMISSILE;
	grapple->clipmask = MASK_SHOT;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	VectorClear(grapple->mins);
	VectorClear(grapple->maxs);
	grapple->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
	//	grapple->s.sound = gi.soundindex ("misc/lasfly.wav");
	grapple->owner = self;
	grapple->touch = GrappleTouch;
	//	grapple->nextthink = level.time + FRAMETIME;
	//	grapple->think = CTFGrappleThink;
	grapple->dmg = damage;
	self->client->ctf_grapple = grapple;
	self->client->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
	gi.linkentity(grapple);

	qboolean water = false;

	float x = grapple->velocity[0];
	float y = grapple->velocity[1];
	float z = grapple->velocity[2];

	float range;

	tr = gi.trace(self->s.origin, NULL, NULL, grapple->s.origin, grapple, MASK_SHOT);

	if (pole->value == 0)
	{
		range = 350;
	}
	else if (pole->value == 1)
	{
		range = 550;
	}
	else if (pole->value == 2)
	{
		range = 630;
	}
	else
	{
		range = 9999;
	}

	char* message;
	if (x < 10)
	{
		self->client->graphit = false;
		message = "that was a really bad cast";
	}
	else if (y > 210 || y<-180 || z>-170 || z < -650)
	{
		self->client->graphit = false;
		message = "cast out of water";
	}
	else if (x > range)
	{
		self->client->graphit = false;
		message = "special message";

		// special error messages
		if (x > 550 && pole->value == 2)
		{
			gi.bprintf(PRINT_HIGH, "cast: %.0Sfm out of range: %.0fm\n", x + 120, range + 120);
			return;
		}
		else if (x > 550)
		{
			gi.bprintf(PRINT_HIGH, "cast: %.0fm out of range: %.0fm\n", x + 120, range);
			return;
		}
		else if (pole->value == 2)
		{
			gi.bprintf(PRINT_HIGH, "cast: %.0fm out of range: %.0fm\n", x, range + 120);
			return;
		}
		else
		{
			gi.bprintf(PRINT_HIGH, "cast: %.0fm out of range: %.0fm\n", x, range);
			return;
		}

	}
	else
	{
		minidist->value = x;
		self->client->graphit = true;
		message = "successful cast";
	}

	gi.multicast(self->s.origin, MULTICAST_PVS);
	gi.bprintf(PRINT_HIGH, "%s\n", message);

	if (tr.fraction < 1.0)
	{
		VectorMA(grapple->s.origin, -10, dir, grapple->s.origin);
		grapple->touch(grapple, tr.ent, NULL, NULL);


	}

}

void GrappleFire(edict_t* ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	float volume = 1.0;

	if (ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
		return; // it's already out

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	//	VectorSet(offset, 24, 16, ent->viewheight-8+2);
	VectorSet(offset, 24, 8, ent->viewheight - 8 + 2);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	if (ent->client->silencer_shots)
		volume = 0.2;

	gi.sound(ent, CHAN_RELIABLE + CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), volume, ATTN_NORM, 0);

	ent->client->grapstart = start;
	ent->client->grapend = forward;

	FireGrapple(ent, start, forward, damage, CTF_GRAPPLE_SPEED, effect);

#if 0
	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_BLASTER);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
#endif

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Grapple_Fire(edict_t* ent)
{
	int		damage;

	damage = 10;
	GrappleFire(ent, vec3_origin, damage, 0);
	ent->client->ps.gunframe++;
}

//----------------------------------------------------------------------


void Cmd_FireFishing_f(edict_t* ent)
{
	
	if (!ent->client->grapuse)
	{
		gi.bprintf(PRINT_HIGH, "not using grapple\n");
		return;
	}
	
	if (ent->client->showstart || ent->client->showmini || ent->client->showend)
	{
		gi.bprintf(PRINT_HIGH, "cant cast while mini game is active\n");
		return;
	}

	//delete old hook
	if (ent->client->ctf_grapple)
	{
		G_FreeEdict(ent->client->grapcur);
	}

	Weapon_Grapple_Fire(ent);
	
	if (!ent->client->graphit)
	{
		return;
	}

	Cmd_MiniGame_f(ent);
}

//acts as Init for fishing mod
void Cmd_Fishing_f(edict_t	*ent)
{
	ent->cash = 0;
	ent->maxcash = 999;

	ent->client->showmini = false;
	ent->client->showbuy = false;
	ent->client->showend = false;
	ent->client->showinfo = false;
	ent->client->showstart = false;

	ent->client->timerstarted = false;

	ent->client->wonmini = false;
	ent->client->miniover = false;
	ent->client->graphit = false;
	ent->client->grapuse = false;

	vec3_t org = { -775,1229,18 };
	VectorCopy(org, ent->s.origin);

	Cmd_GiveGrapple_f(ent);
	Cmd_UseFishing_f(ent);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (Q_stricmp(cmd, "buy") == 0)
	{
		Cmd_HelpBuy_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "pole") == 0)
	{
		Cmd_UseFishing_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "endscreen") == 0)
	{
		Cmd_HelpEnd_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "startscreen") == 0)
	{
		Cmd_HelpStart_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "changescreen") == 0)
	{
		Cmd_ChangeMiniScreen_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "joe") == 0)
	{
		Cmd_HelpJoe_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "fish") == 0)
	{
		Cmd_Fishing_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "leftpull") == 0)
	{
		Cmd_LeftPull_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "rightpull") == 0)
	{
		Cmd_RightPull_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "yoink") == 0)
	{
		Cmd_Yoink_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "nextpage") == 0)
	{
		Cmd_NextPage_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "switchpage") == 0)
	{
		Cmd_SwitchPage_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "lastpage") == 0)
	{
		Cmd_LastPage_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "mini") == 0)
	{
		Cmd_MiniGame_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "spawnmack") == 0)
	{
		Cmd_SpawnMack_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "spawncod") == 0)
	{
		Cmd_SpawnCod_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "spawnsword") == 0)
	{
		Cmd_SpawnSword_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "spawnrainb") == 0)
	{
		Cmd_SpawnRainb_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "spawntuna") == 0)
	{
		Cmd_SpawnTuna_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "fire") == 0)
	{
		Cmd_FireFishing_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_pole") == 0)
	{
		Cmd_UpgradePole_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_reel") == 0)
	{
		Cmd_UpgradeReel_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_hook") == 0)
	{
		Cmd_UpgradeHook_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_weight") == 0)
	{
		Cmd_UpgradeWeight_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_bait") == 0)
	{
		Cmd_UpgradeBait_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "upgrade_tackle") == 0)
	{
		Cmd_UpgradeTackle_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "givecash") == 0)
	{
		Cmd_GiveMaxCash_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "godgear") == 0)
	{
		Cmd_GodGear_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "resetgear") == 0)
	{
		Cmd_ResetGear_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
