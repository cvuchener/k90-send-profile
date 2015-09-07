/*
 * Copyright 2015 Clément Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef PROFILE_H
#define PROFILE_H

#include <json/value.h>
#include <string>
#include <cstdint>

class Profile
{
public:
	Profile ();
	~Profile ();

	bool fromJsonValue (const Json::Value &profile);

	void buildData (std::string &keys, std::string &bindings, std::string &data) const;

private:
	enum BindType: uint8_t {
		BindNone = 0x00,
		BindUsage = 0x10,
		BindMacro = 0x20,
	};
	enum MacroItemType: uint8_t {
		ItemKey = 0x84,
		ItemEnd = 0x86,
		ItemDelay = 0x87,
	};
	enum RepeatMode: uint8_t {
		RepeatFixed = 1,
		RepeatHold = 2,
		RepeatToggle = 3,
	};

	struct macro_item_t {
		MacroItemType type;
		union {
			struct {
				uint8_t usage;
				bool pressed;
			} key_event;
			unsigned int delay;
		};
	};

	struct key_t {
		uint8_t key_usage;
		RepeatMode repeat_mode;
		BindType bind_type;
		uint8_t target_usage;
		unsigned int repeat_count;
		std::vector<macro_item_t> macro;
	};

	std::vector<key_t> _keys;
};

#endif

