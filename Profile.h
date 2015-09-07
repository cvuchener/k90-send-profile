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

