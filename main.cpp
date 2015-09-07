#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <json/json.h>
#include <json/reader.h>

extern "C" {
#include <unistd.h>
#include <libusb.h>
}

#include "Profile.h"

#define K90_VID	0x1b1c
#define K90_PID 0x1b02

#define DELAY	200000

#define REQUEST_SET_MODE	2
#define REQUEST_GET_MODE	5
#define REQUEST_BINDINGS	16
#define REQUEST_DATA	18
#define REQUEST_KEYS	22

#define MODE_HARDWARE	0x01

int main (int argc, char *argv[])
{
	int profile_number;
	uint8_t mode[2];

	if (argc < 2 || argc > 3) {
		fprintf (stderr, "Usage: %s profile_number [file]\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	profile_number = strtol (argv[1], nullptr, 0);
	if (profile_number < 1 || profile_number > 3) {
		fprintf (stderr, "Invalid profile number\n");
		return EXIT_FAILURE;
	}

	/*
	 * Load profile
	 */
	Json::Value profile_json;
	Json::Reader reader;
	bool ok;
	if (argc == 3) {
		std::ifstream file (argv[2], std::ifstream::in);
		ok = reader.parse (file, profile_json);
	}
	else
		ok = reader.parse (std::cin, profile_json);
	if (!ok) {
		fprintf (stderr, "Error while parsing JSON:\n"
		                 "%s",
		         reader.getFormattedErrorMessages ().c_str ());
		return EXIT_FAILURE;
	}

	Profile profile;
	if (!profile.fromJsonValue (profile_json)) {
		fprintf (stderr, "Invalid profile structure\n");
		return EXIT_FAILURE;
	}

	std::string keys, bindings, data;
	profile.buildData (keys, bindings, data);

	/*
	 * Init libusb context and device
	 */
	int ret;
	bool error = false;
	libusb_context *context;
	if (0 != (ret = libusb_init (&context))) {
		fprintf (stderr, "Failed to initialize libusb: %s\n", libusb_error_name (ret));
		return EXIT_FAILURE;
	}
	libusb_set_debug (context, LIBUSB_LOG_LEVEL_WARNING);

	libusb_device_handle *device;
	if (!(device = libusb_open_device_with_vid_pid (context, K90_VID, K90_PID))) {
		fprintf (stderr, "Device not found\n");
		return EXIT_FAILURE;
	}
	libusb_config_descriptor *config;
	if (0 != (ret = libusb_get_active_config_descriptor (libusb_get_device (device), &config))) {
		fprintf (stderr, "Could not get config descriptor: %s\n", libusb_error_name (ret));
		return EXIT_FAILURE;
	}

	bool driver_active[config->bNumInterfaces];
	for (int i = 0; i < config->bNumInterfaces; ++i) {
		driver_active[i] = libusb_kernel_driver_active (device, i);
		if (driver_active[i]) {
			if (0 != (ret = libusb_detach_kernel_driver (device, i))) {
				fprintf (stderr, "Cannot detach kernel driver for interface %d: %s\n", i, libusb_error_name (ret));
				return EXIT_FAILURE;
			}
		}
	}

	/*
	 * Sending data
	 */
	for (auto tuple: std::array<std::tuple<const std::string *, int>, 3> {
			std::make_tuple (&bindings, REQUEST_BINDINGS),
			std::make_tuple (&data, REQUEST_DATA),
			std::make_tuple (&keys, REQUEST_KEYS) }) {
		const std::string *packet;
		int request;
		std::tie (packet, request) = tuple;

		if (request == REQUEST_DATA && packet->size () == 0)
			continue;

		unsigned char *data = new unsigned char [packet->size ()];	// libusb need writable data, ...
		memcpy (data, packet->data (), packet->size ());	// ...so we copy it
		ret = libusb_control_transfer (device,
					       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
					       request, 0, profile_number, data, packet->size (), 0);
		delete[] data;

		if (ret < 0) {
			fprintf (stderr, "Failed to send data for request %d to keyboard: %s (%s)\n",
				 request, libusb_error_name (ret), strerror (errno));
			error = true;
		}
		else if ((unsigned int) ret != packet->size ()) {
			fprintf (stderr, "Incomplete control transfer\n");
			error = true;
		}

		usleep (DELAY);

		// Check status
		ret = libusb_control_transfer (device,
					       LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
					       REQUEST_GET_MODE, 0, 0, mode, sizeof (mode), 0);
		if (ret < 0) {
			fprintf (stderr, "Failed to get hardware mode: %s (%s)\n",
				 libusb_error_name (ret), strerror (errno));
			error = true;
		}
		if (mode[1] != 0x01) {
			fprintf (stderr, "Transfer error (going too fast?)\n");
		}

		usleep (DELAY);
	}

	/*
	 * Clean up libusb
	 */
	for (int i = 0; i < config->bNumInterfaces; ++i) {
		if (driver_active[i]) {
			if (0 != (ret = libusb_attach_kernel_driver (device, i))) {
				fprintf (stderr, "Cannot attach kernel driver for interface %d: %s\n", i, libusb_error_name (ret));
				error = true;
			}
		}
	}
	libusb_close (device);
	libusb_exit (context);

	return (error ? EXIT_FAILURE : EXIT_SUCCESS);
}
