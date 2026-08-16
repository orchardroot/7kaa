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

//Filename    : OF_TURR.CPP
//Description : FirmTurret - defensive structure that fires arrows at
//              nearby hostile units. Fires slowly unmanned; garrisoned
//              soldiers increase the rate of fire.

#include <OINFO.h>
#include <OBUTT3D.h>
#include <OF_TURR.h>
#include <OMP_CRC.h>

//--------- Define static vars ----------//

static Button3D	button_vacate_firm;

//--------- Begin of function FirmTurret::FirmTurret ---------//
//
FirmTurret::FirmTurret()
{
	fire_delay_count = 0;
}
//----------- End of function FirmTurret::FirmTurret -----------//


//--------- Begin of function FirmTurret::~FirmTurret ---------//
//
FirmTurret::~FirmTurret()
{
}
//----------- End of function FirmTurret::~FirmTurret -----------//


//--------- Begin of function FirmTurret::put_info ---------//
//
void FirmTurret::put_info(int refreshFlag)
{
	disp_basic_info(INFO_Y1, refreshFlag);

	if( !should_show_info() )
		return;

	disp_worker_list(INFO_Y1+52, refreshFlag);
	disp_worker_info(INFO_Y1+116, refreshFlag);

	//------ display mobilize button -------//

	int x = INFO_X1;

	if( own_firm() )
	{
		if( refreshFlag == INFO_REPAINT )
		{
			button_vacate_firm.paint(INFO_X1, INFO_Y1+174, 'A', "RECRUIT");
			button_vacate_firm.set_help_code("MOBILIZE");
		}

		if( have_own_workers() )
			button_vacate_firm.enable();
		else
			button_vacate_firm.disable();

		x += BUTTON_ACTION_WIDTH;
	}

	//---------- display spy button ----------//

	disp_spy_button(x, INFO_Y1+174, refreshFlag);
}
//----------- End of function FirmTurret::put_info -----------//


//--------- Begin of function FirmTurret::detect_info ---------//
//
int FirmTurret::detect_info()
{
	if( detect_basic_info() )
		return 1;

	if( detect_worker_list() )
	{
		disp_worker_info(INFO_Y1+116, INFO_UPDATE);
		return 1;
	}

	if( detect_spy_button() )
		return 1;

	if( !own_firm() )
		return 0;

	if( button_vacate_firm.detect() )
	{
		mobilize_all_workers(COMMAND_PLAYER);
		return 1;
	}

	return 0;
}
//----------- End of function FirmTurret::detect_info -----------//


//--------- Begin of function FirmTurret::next_day ---------//
//
void FirmTurret::next_day()
{
	//----- call next_day() of the base class -----//

	Firm::next_day();
}
//----------- End of function FirmTurret::next_day -----------//


//--------- Begin of function FirmTurret::process_animation ---------//
//
void FirmTurret::process_animation()
{
	Firm::process_animation();

	// firing logic is added in a later commit
}
//----------- End of function FirmTurret::process_animation -----------//


//--------- Begin of function FirmTurret::find_target ---------//
//
// return: recno of the nearest hostile mobile unit in range, or 0
//
short FirmTurret::find_target()
{
	return 0;      // implemented in a later commit
}
//----------- End of function FirmTurret::find_target -----------//


//--------- Begin of function FirmTurret::current_fire_delay ---------//
//
// return: no. of frames between shots, given the current garrison
//
int FirmTurret::current_fire_delay()
{
	return 0;      // implemented in a later commit
}
//----------- End of function FirmTurret::current_fire_delay -----------//


//--------- Begin of function FirmTurret::turret_attack_info ---------//
//
// return: the AttackInfo whose ballistic data the turret fires with
//
AttackInfo* FirmTurret::turret_attack_info()
{
	return NULL;   // implemented in a later commit
}
//----------- End of function FirmTurret::turret_attack_info -----------//


// Note: crc8/clear_ptr/init_crc live in OMP_CRC.cpp with the other firms'.
