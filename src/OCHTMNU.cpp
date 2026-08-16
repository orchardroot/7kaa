/*
 * Seven Kingdoms: Ancient Adversaries
 *
 * Copyright 1997,1998 Enlight Software Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Filename    : OCHTMNU.CPP
// Description : in-game cheats menu

#include <OVGA.h>
#include <OVGABUF.h>
#include <vga_util.h>
#include <OSYS.h>
#include <OMOUSE.h>
#include <OMOUSECR.h>
#include <KEY.h>
#include <OPOWER.h>
#include <OREMOTE.h>
#include <ONATIONA.h>
#include <OINFO.h>
#include <OFONT.h>
#include <OMUSIC.h>
#include <OBOX.h>
#include <OCONFIG.h>
#include <ConfigAdv.h>
#include <OTECHRES.h>
#include <OGODRES.h>
#include <OWORLD.h>
#include <OTOWN.h>
#include <OFIRM.h>
#include <OUNIT.h>
#include <OSTR.h>
#include <OCHTMNU.h>
#include "gettext.h"

CheatMenu cheat_menu;

enum { CHEAT_MENU_WIDTH  = 400,
       CHEAT_MENU_HEIGHT = 336  };

enum { CHEAT_MENU_X1 = ZOOM_X1 + ( (ZOOM_X2-ZOOM_X1+1) - CHEAT_MENU_WIDTH ) / 2,
       CHEAT_MENU_Y1 = ZOOM_Y1 + ( (ZOOM_Y2-ZOOM_Y1+1) - CHEAT_MENU_HEIGHT ) / 2 };

enum { CHEAT_OPTION_HEIGHT = 26 };

enum { CHEAT_OPTION_X1 = CHEAT_MENU_X1 + 30,
       CHEAT_OPTION_Y1 = CHEAT_MENU_Y1 + 56,
       CHEAT_OPTION_X2 = CHEAT_MENU_X1 + CHEAT_MENU_WIDTH - 31 };

static const char* cheat_option_str[CheatMenu::CHEAT_OPTION_COUNT] =
{
	N_("+1000 Gold"),
	N_("+1000 Food"),
	N_("All Technology & Gods"),
	N_("Reveal Map"),
	N_("+10 Town Population"),
	N_("Repair Building"),
	N_("+20 Combat (Unit)"),
	N_("Immortal King"),
	N_("Fast Build"),
	N_("Done"),
};


CheatMenu::CheatMenu()
{
	active_flag = 0;
	refresh_flag = 0;
}


void CheatMenu::enter()
{
	if( active_flag )
		return;

	if( remote.is_enable() )       // no cheating in multiplayer games
		return;

	refresh_flag = 1;
	active_flag = 1;

	mouse_cursor.set_icon(CURSOR_NORMAL);

	power.win_opened = 1;

	info.save_game_scr();

	while( is_active() )
	{
		sys.yield();
		vga.flip();
		mouse.get_event();

		// display on front buffer
		char useBackBuf = vga.use_back_buf;
		vga.use_front();
		disp();
		if(useBackBuf)
			vga.use_back();

		sys.blt_virtual_buf();
		music.yield();
		detect();
	}
}


void CheatMenu::disp()
{
	if( !active_flag )
		return;

	if( !refresh_flag )
		return;

	vga_util.d3_panel_up( CHEAT_MENU_X1, CHEAT_MENU_Y1,
		CHEAT_MENU_X1+CHEAT_MENU_WIDTH-1, CHEAT_MENU_Y1+CHEAT_MENU_HEIGHT-1, 1 );

	font_bible.center_put( CHEAT_MENU_X1, CHEAT_MENU_Y1+14,
		CHEAT_MENU_X1+CHEAT_MENU_WIDTH-1, CHEAT_MENU_Y1+40, _("Cheats") );

	int y = CHEAT_OPTION_Y1;

	for( int b = 0; b < CHEAT_OPTION_COUNT; ++b, y += CHEAT_OPTION_HEIGHT )
	{
		String str( _(cheat_option_str[b]) );

		if( b == 7 )      // immortal king toggle shows its state
		{
			str += ": ";
			str += config.king_undie_flag ? _("ON") : _("OFF");
		}
		else if( b == 8 ) // fast build toggle shows its state
		{
			str += ": ";
			str += config.fast_build ? _("ON") : _("OFF");
		}

		font_san.center_put( CHEAT_OPTION_X1, y,
			CHEAT_OPTION_X2, y+CHEAT_OPTION_HEIGHT-1, str );

		if( !option_enabled(b+1) )
		{
			// darken disabled option
			Vga::active_buf->adjust_brightness(
				CHEAT_OPTION_X1, y,
				CHEAT_OPTION_X2, y+CHEAT_OPTION_HEIGHT-1, -6);
		}
	}

	refresh_flag = 0;
}


int CheatMenu::detect()
{
	if( !active_flag )
		return 0;

	int i, y=CHEAT_OPTION_Y1;

	for( i=1 ; i<=CHEAT_OPTION_COUNT ; i++, y+=CHEAT_OPTION_HEIGHT )
	{
		if( option_enabled(i) &&
			mouse.single_click( CHEAT_OPTION_X1, y, CHEAT_OPTION_X2, y+CHEAT_OPTION_HEIGHT-1 ) )
			break;

		if( i == CHEAT_OPTION_COUNT &&      // assume last option is 'done'
			(mouse.any_click(1) || mouse.key_code==KEY_ESC) )
			break;
	}

	if( i>CHEAT_OPTION_COUNT )
		return 0;

	if( i == CHEAT_OPTION_COUNT )    // Done
	{
		exit();
		return 1;
	}

	apply_option(i);

	refresh_flag = 1;       // toggles change their labels

	return 1;
}


// return whether option optionId (1-based) can currently be used
int CheatMenu::option_enabled(int optionId)
{
	if( optionId == CHEAT_OPTION_COUNT )      // Done
		return 1;

	if( !nation_array.player_recno || remote.is_enable() )
		return 0;

	switch( optionId )
	{
		case 5:     // town population
			return town_array.selected_recno != 0;

		case 6:     // repair building
			return firm_array.selected_recno != 0;

		case 7:     // unit combat level
			return unit_array.selected_recno != 0;

		default:
			return 1;
	}
}


// apply option optionId (1-based); guards mirror Sys::detect_cheat_key
void CheatMenu::apply_option(int optionId)
{
	if( !option_enabled(optionId) )
		return;

	switch( optionId )
	{
		case 1:     // add cash
			(~nation_array)->add_cheat((float)1000);
			break;

		case 2:     // add food
			(~nation_array)->add_food((float)1000);
			break;

		case 3:     // all technology & gods
			tech_res.inc_all_tech_level(nation_array.player_recno);
			god_res.enable_know_all(nation_array.player_recno);
			box.msg( _("Your technology has advanced.\nYou can now invoke all Greater Beings.") );
			break;

		case 4:     // reveal map
			world.unveil(0, 0, MAX_WORLD_X_LOC-1, MAX_WORLD_Y_LOC-1);
			world.visit(0, 0, MAX_WORLD_X_LOC-1, MAX_WORLD_Y_LOC-1, 0, 0);
			break;

		case 5:     // increase town population
		{
			Town* townPtr = town_array[town_array.selected_recno];
			int num = misc.random(config_adv.race_random_list_max);
			townPtr->init_pop( config_adv.race_random_list[num], 10, 100 );
			townPtr->auto_set_layout();
			break;
		}

		case 6:     // repair selected firm
		{
			Firm* firmPtr = firm_array[firm_array.selected_recno];
			firmPtr->hit_points = firmPtr->max_hit_points;
			break;
		}

		case 7:     // increase unit combat level
		{
			Unit* unitPtr = unit_array[unit_array.selected_recno];
			unitPtr->set_combat_level( MIN(100, unitPtr->skill.combat_level+20) );
			break;
		}

		case 8:     // immortal king toggle
			config.king_undie_flag = !config.king_undie_flag;
			break;

		case 9:     // fast build toggle
			config.fast_build = !config.fast_build;
			break;
	}

	(~nation_array)->cheat_enabled_flag = 1;
}


void CheatMenu::exit()
{
	info.rest_game_scr();

	power.win_opened = 0;
	active_flag = 0;
}


void CheatMenu::abort()
{
	power.win_opened = 0;
	active_flag = 0;
}
