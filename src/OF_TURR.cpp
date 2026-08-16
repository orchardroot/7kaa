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
#include <OUNIT.h>
#include <OBULLET.h>
#include <ONATIONA.h>
#include <OF_TURR.h>

//--------- Define combat constants ----------//

enum { TURRET_RANGE = 8,               // firing range in locations
       TURRET_SCAN_DELAY = 10,         // frames between target scans when idle
       TURRET_MAX_GARRISON_BONUS = 4   // no. of garrisoned soldiers that speed up firing
     };

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
// Called every frame. Fires an arrow at the nearest hostile unit in
// range whenever the fire delay has elapsed.
//
void FirmTurret::process_animation()
{
	Firm::process_animation();

	if( under_construction || !nation_recno )
		return;

	if( fire_delay_count > 0 )
	{
		fire_delay_count--;
		return;
	}

	AttackInfo* attackInfo = turret_attack_info();

	if( !attackInfo )
		return;

	short targetRecno = find_target();

	if( !targetRecno )
	{
		fire_delay_count = TURRET_SCAN_DELAY;
		return;
	}

	if( worker_count == 0 )
	{
		//------ unmanned turrets fire at reduced damage ------//

		static AttackInfo weakAttack;

		weakAttack = *attackInfo;
		weakAttack.attack_damage = attackInfo->attack_damage * 3 / 5;

		bullet_array.add_bullet(this, unit_array[targetRecno], &weakAttack);
	}
	else
	{
		bullet_array.add_bullet(this, unit_array[targetRecno], attackInfo);
	}

	fire_delay_count = current_fire_delay();
}
//----------- End of function FirmTurret::process_animation -----------//


//--------- Begin of function FirmTurret::find_target ---------//
//
// return: recno of the nearest hostile mobile unit in range, or 0
//
short FirmTurret::find_target()
{
	int	bestDist  = TURRET_RANGE+1;
	short	bestRecno = 0;

	for( short i=1 ; i<=unit_array.size() ; i++ )
	{
		if( unit_array.is_deleted(i) )
			continue;

		Unit* unitPtr = unit_array[i];

		if( !unitPtr->is_visible() )
			continue;

		if( !nation_array.should_attack(nation_recno, unitPtr->nation_recno) )
			continue;

		int dist = misc.points_distance( center_x, center_y,
						unitPtr->next_x_loc(), unitPtr->next_y_loc() );

		if( dist < bestDist )
		{
			bestDist  = dist;
			bestRecno = i;
		}
	}

	return bestRecno;
}
//----------- End of function FirmTurret::find_target -----------//


//--------- Begin of function FirmTurret::current_fire_delay ---------//
//
// return: no. of frames between shots, given the current garrison.
//
// Unmanned, the turret fires at a third of the archer's rate. Each
// garrisoned soldier speeds it up -- ranged soldiers twice as much --
// down to the archer's own rate at a full garrison.
//
int FirmTurret::current_fire_delay()
{
	AttackInfo* attackInfo = turret_attack_info();

	int baseDelay = attackInfo ? attackInfo->attack_delay : 30;

	int bonus = 0;

	for( int i=0 ; i<worker_count && i<TURRET_MAX_GARRISON_BONUS ; i++ )
	{
		if( worker_array[i].max_attack_range() > 1 )
			bonus += baseDelay/2;      // ranged soldiers help twice as much
		else
			bonus += baseDelay/4;
	}

	return MAX( baseDelay, baseDelay*3 - bonus );
}
//----------- End of function FirmTurret::current_fire_delay -----------//


//--------- Begin of function FirmTurret::turret_attack_info ---------//
//
// return: the AttackInfo whose ballistic data the turret fires with --
//         the Norman soldier's ranged attack, or failing that the first
//         ranged attack with a projectile sprite in the database.
//
AttackInfo* FirmTurret::turret_attack_info()
{
	static AttackInfo* cachedAttackInfo = NULL;

	if( cachedAttackInfo )
		return cachedAttackInfo;

	UnitInfo*   unitInfo   = unit_res[UNIT_NORMAN];
	AttackInfo* attackInfo = unit_res.get_attack_info(unitInfo->first_attack);

	for( int i=0 ; i<unitInfo->attack_count ; i++, attackInfo++ )
	{
		if( attackInfo->attack_range > 1 && attackInfo->bullet_sprite_id )
		{
			cachedAttackInfo = attackInfo;
			return cachedAttackInfo;
		}
	}

	//----- fallback: any ranged attack with a projectile sprite -----//

	attackInfo = unit_res.attack_info_array;

	for( int i=0 ; i<unit_res.attack_info_count ; i++, attackInfo++ )
	{
		if( attackInfo->attack_range >= 6 && attackInfo->bullet_sprite_id )
		{
			cachedAttackInfo = attackInfo;
			return cachedAttackInfo;
		}
	}

	return NULL;
}
//----------- End of function FirmTurret::turret_attack_info -----------//


// Note: crc8/clear_ptr/init_crc live in OMP_CRC.cpp with the other firms'.
