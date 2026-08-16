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

//Filename    : OF_TURR.H
//Description : Header of FirmTurret, a defensive structure firing arrows

#ifndef __OF_TURR_H
#define __OF_TURR_H

#ifndef __OFIRM_H
#include <OFIRM.h>
#endif

struct FirmTurretCrc;

//------- Define class FirmTurret --------//

#pragma pack(1)
class FirmTurret : public Firm
{
public:
	short fire_delay_count;       // frames until the next shot may be fired

public:
	FirmTurret();
	~FirmTurret();

	void 	put_info(int refreshFlag);
	int	detect_info();

	void	next_day();
	virtual void process_animation();

	//-------------- multiplayer checking codes ---------------//
	virtual	uint8_t crc8();
	virtual	void	clear_ptr();
	virtual	void	init_crc(FirmTurretCrc *c);

private:
	void	disp_soldier_info(int dispY1, int refreshFlag);
	short	find_target();
	int	current_fire_delay();
	AttackInfo*	turret_attack_info();
};
#pragma pack()

//--------------------------------------//

#endif
