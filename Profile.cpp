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

#include <iostream>
#include <sstream>
#include "KeyUsage.h"

#include "Profile.h"

Profile::Profile ()
{
}

Profile::~Profile ()
{
}

bool Profile::fromJsonValue (const Json::Value &profile)
{
	std::map<std::string, uint8_t>::const_iterator it;
	std::string key_str;

	if (!profile.isArray ()) {
		std::cerr << "profile is not an array" << std::endl;
		return false;
	}
	
	_keys.resize (profile.size ());
	for (unsigned int i = 0; i < profile.size (); ++i) {
		if (!profile[i].isMember ("key")) {
			std::cerr << "Missing \"key\" member in key " << i << std::endl;
			return false;
		}
		key_str = profile[i]["key"].asString ();
		it = key_usage.find (key_str);
		if (it == key_usage.end ()) {
			std::cerr << "Unknown key: " << key_str << std::endl;
			return false;
		}
		_keys[i].key_usage = it->second;

		if (profile[i].isMember ("repeat_mode")) {
			std::string repeat_mode = profile[i]["repeat_mode"].asString ();
			if (repeat_mode == "fixed")
				_keys[i].repeat_mode = RepeatFixed;
			else if (repeat_mode == "hold")
				_keys[i].repeat_mode = RepeatHold;
			else if (repeat_mode == "toggle")
				_keys[i].repeat_mode = RepeatToggle;
			else {
				std::cerr << "Unknown repeat mode: " << repeat_mode << std::endl;
				return false;
			}
		}
		else 
			_keys[i].repeat_mode = RepeatFixed;
		
		if (profile[i].isMember ("type")) {
			std::string type = profile[i]["type"].asString ();
			if (type == "none")
				_keys[i].bind_type = BindNone;
			else if (type == "key")
				_keys[i].bind_type = BindUsage;
			else if (type == "macro")
				_keys[i].bind_type = BindMacro;
			else {
				std::cerr << "Unknown type: " << type << std::endl;
				return false;
			}
		}
		else
			_keys[i].bind_type = BindMacro;

		switch (_keys[i].bind_type) {
		case BindNone:
			break;

		case BindUsage: {
			if (!profile[i].isMember ("new_key")) {
				std::cerr << "Missing \"new_key\" member for type \"key\"" << std::endl;
				return false;
			}
			key_str = profile[i]["new_key"].asString ();
			it = key_usage.find (key_str);
			if (it == key_usage.end ()) {
				std::cerr << "Unknown key: " << key_str << std::endl;
				return false;
			}
			_keys[i].target_usage = it->second;
			break;
		}

		case BindMacro: {
			if (profile[i].isMember ("repeat_count"))
				_keys[i].repeat_count = profile[i]["repeat_count"].asUInt ();
			else
				_keys[i].repeat_count = 1;

			if (!profile[i].isMember ("macro")) {
				std::cerr << "Missing \"macro\" member" << std::endl;
				return false;
			}
			Json::Value macro = profile[i]["macro"];
			if (!macro.isArray ()) {
				std::cerr << "\"macro\" must be an array" << std::endl;
				return false;
			}
			_keys[i].macro.resize (macro.size ());
			for (unsigned int j = 0; j < macro.size (); ++j) {
				if (macro[j].isMember ("key")) {
					_keys[i].macro[j].type = ItemKey;
					key_str = macro[j]["key"].asString ();
					it = key_usage.find (key_str);
					if (it == key_usage.end ()) {
						std::cerr << "Unknown key: " << key_str << std::endl;
						return false;
					}
					_keys[i].macro[j].key_event.usage = it->second;
					if (!macro[j].isMember ("pressed")) {
						std::cerr << "Missing \"pressed\" member in macro item" << std::endl;
						return false;
					}
					_keys[i].macro[j].key_event.pressed = macro[j]["pressed"].asBool ();
				}
				else if (macro[j].isMember ("delay")) {
					_keys[i].macro[j].type = ItemDelay;
					_keys[i].macro[j].delay = macro[j]["delay"].asUInt ();
				}
				else {
					std::cerr << "Invalid macro item" << std::endl;
				}
			}
			break;
		}
		}
	}

	return true;
}

template<typename T>
static void write (std::ostream &ostream, T data)
{
	ostream.write (reinterpret_cast<const char *> (&data), sizeof (T));
}

void Profile::buildData (std::string &keys_str, std::string &bindings_str, std::string &data_str) const
{
	std::stringstream keys, bindings, data;

	std::vector<unsigned int> addresses;
	for (const key_t &key: _keys) {
		addresses.push_back (data.tellp ());
		switch (key.bind_type) {
		case BindNone:
			break;

		case BindUsage:
			write (data, key.target_usage);
			break;

		case BindMacro:
			for (const macro_item_t &item: key.macro) {
				write (data, item.type);
				switch (item.type) {
				case ItemKey:
					write (data, item.key_event.usage);
					write (data, (uint8_t) (item.key_event.pressed ? 0x01 : 0x00));
					break;

				case ItemDelay:
					write (data, htobe16 (item.delay));
					break;

				case ItemEnd:
					// written after this loop, it should not be in this vector
					break;
				}
			}
			write (data, ItemEnd);
			write (data, htobe16 (key.repeat_count));
			break;
		}
	}
	addresses.push_back (data.tellp ());
	data_str = data.str ();

	write (keys, (uint8_t) _keys.size ());
	write (bindings, (uint8_t) _keys.size ());
	write (bindings, htobe16 (5 + 5*_keys.size ()));
	write (bindings, htobe16 (data_str.size ()));
	for (unsigned int i = 0; i < _keys.size (); ++i) {
		write (keys, _keys[i].key_usage);
		write (keys, _keys[i].repeat_mode);

		write (bindings, _keys[i].bind_type);
		if (_keys[i].bind_type == BindNone)
			write (bindings, (uint16_t) 0);
		else
			write (bindings, htobe16 (addresses[i]));
		write (bindings, htobe16 (addresses[i+1] - addresses[i]));
	}
	keys_str = keys.str ();
	bindings_str = bindings.str ();
}
