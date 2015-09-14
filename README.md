Sending hardware profiles to the Corsair K90 keyboards
======================================================

Building
--------

This program depends on **jsoncpp** and **libusb-1.0**.

Use `make` to build the program.


Usage
-----

```
./k90-send-profile [options] profile_number [file]
```

The program sends the profile to the macro profile `profile_number` on the K90 keyboard.

The `profile_number` must be between 1 and 3. If no file is provided, the profile is read from stdin.

Options are:
 - `-l<layout>`, `--layout [layout]` Use `<layout>` when reading keys (see KeyUsage.cpp). Make writing macro for non QWERTY-US keyboards, easier.


Profile file format
-------------------

Profile file use JSON syntax. The profile is an array of objects where each object describe the settings for a key.

The member for a key settings are:
 - **key**: the key to configure (usually `G*`). This member is mandatory.
 - **repeat_mode**: how the macro is repeated. Accepted values are:
   - *fixed* (default): the macro is played a fixed number of time when the key is pressed.
   - *hold*: the macro is repeated while the key is held.
   - *toggle*: the macro is repeated until the key is pressed again.
 - **type**: the type of settings for the key. Accepted values are:
   - *none*: the key produces its normal key code.
   - *key*: the key produces another key's code.
   - *macro* (default): the key plays a macro.
 - **new_key**: the key whose key code will be used with *key* type.
 - **repeat_count**: the number of time the macro is played in *fixed* mode. Only used by *macro* type. Default is 1.
 - **macro**: an array of macro items. Mandatory for *macro* type. Members for macro items are:
   - **key**: for a key event, the value is the key whose event is played. The item must have a **pressed** member if this one is set.
   - **pressed**: a boolean: *true* for press event, *false* for a release event.
   - **delay**: create a delay in the macro. The delay is given in milliseconds.

See the list of accepted keys in [KeyUsage.cpp](KeyUsage.cpp).

See [default.json](default.json) or [example.json](example.json) for example profiles.

Because of the hardware, the **key** member is not always used as it should.  It is only used in the first sixteen items. The 17th and 18th always match G17 and G18 and following do nothing. Also if the *Gn* key is not configured but there is a *n* th settings item it will be used for both configured key and the *Gn* key.

License
-------

This program is licensed under GNU GPL.
