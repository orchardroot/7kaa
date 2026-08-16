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

// Filename    : OCHTMNU.H
// Description : in-game cheats menu

#ifndef __OCHTMNU_H
#define __OCHTMNU_H

class CheatMenu
{
public:
	enum { CHEAT_OPTION_COUNT = 10 };      // 9 cheats + Done

	int	active_flag;
	int	refresh_flag;

public:
	CheatMenu();

	int	is_active()		{ return active_flag; }
	void	enter();
	void	disp();
	int	detect();
	void	exit();
	void	abort();

private:
	int	option_enabled(int optionId);
	void	apply_option(int optionId);
};

extern CheatMenu cheat_menu;

#endif
