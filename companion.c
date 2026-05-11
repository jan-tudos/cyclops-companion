// we want 'nanosleep'
#define _POSIX_C_SOURCE 199309L

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libusb-1.0/libusb.h>

constexpr size_t max_retries = 7; // arbitrary, nice number
constexpr size_t cmd_len = 60; // magic payload is always 60 bytes


struct action {
	char msg[100];
	unsigned char bytes[cmd_len];
};

static
char const *
get_file_name(char const * const path)
{
	char const * const sep = strrchr(path, '/');

	// ignore separator if any
	return (sep ? sep + 1 : path);
}


int
main(int const argc, char const * const * const argv)
{
	// device info
	uint16_t const vend_id = 0x6e30;
	uint16_t const prod_id = 0xfef4;
	int const interface_num = 0;  // video control interface


	// control transfer parameters
	uint8_t  const bmRequestType = 0x21;   // Direction = Host to Device | Type = Class | Recipient = Interface
	uint8_t  const bRequest      = 0x01;   // SET_CUR
	uint16_t const wValue        = 0x0200; // (control selector 0x02) << 8 | 0x00
	uint16_t const wIndex        = 0x0200; // (entity ID 0x02) << 8 | 0x00


	// sanity check
	if (argc != 1 || argv == NULL || argv[0] == NULL) {
		fprintf(stderr, "Invalid invocation (missing program name, parameters, ...).\n");
		return EXIT_FAILURE;
	}

	// Who are we?
	char const * const name = get_file_name(argv[0]);

	constexpr size_t max_acts = 4; // up to 4 is enough for us; KISS!
	struct action acts[max_acts];
	size_t num_acts = 0;   // how many labours are there for us to perform?

	// No need to leak the magic payloads into function scope...
	{
		static const unsigned char wake[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x35, 0x2c, 0xf1, 0x0, 0xe1, 0x13, 0xc2, 0x1, 0x1, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(wake) == cmd_len, "Unexpected magic payload size");
		static const unsigned char unlock[] = {0xaa, 0x0, 0xc, 0x10, 0x0, 0xc4, 0xcb, 0xed, 0x0, 0xe3, 0x30, 0x66, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(unlock) == cmd_len, "Unexpected magic payload size");
		static const unsigned char no_follow[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x28, 0xb9, 0x77, 0x0, 0xe3, 0x30, 0x90, 0x0, 0x0, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(no_follow) == cmd_len, "Unexpected magic payload size");
		static const unsigned char no_zoom[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x2b, 0xd9, 0x62, 0x0, 0xe3, 0x30, 0x90, 0x1, 0x0, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(no_zoom) == cmd_len, "Unexpected magic payload size");
		static const unsigned char follow[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x7, 0x48, 0x6e, 0x0, 0xe3, 0x30, 0x90, 0x0, 0x1, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(follow) == cmd_len, "Unexpected magic payload size");
		static const unsigned char zoom[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x2c, 0x29, 0x85, 0x0, 0xe3, 0x30, 0x90, 0x1, 0x1, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof(zoom) == cmd_len, "Unexpected magic payload size");
		static const unsigned char suspend[] = {0xaa, 0x0, 0xe, 0x10, 0x0, 0x34, 0x7d, 0x7d, 0x0, 0xe1, 0x13, 0xc2, 0x1, 0x3, 0x3e, 0x1, 0x2, 0x1, 0x0, 0x0, 0x0, 0x14, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
		static_assert(sizeof suspend == cmd_len, "Unexpected magic payload size");


		// What are we supposed to do? Depends on who we are...
		if (0 == strcmp(name, "siren")) {
			strncpy(acts[0].msg, "Wake up camera", sizeof(acts[0].msg));
			memcpy(acts[0].bytes, wake, sizeof wake);
			strncpy(acts[1].msg, "Unlock from any target the camera has", sizeof(acts[1].msg));
			memcpy(acts[1].bytes, unlock, sizeof unlock);
			strncpy(acts[2].msg, "Disable 'Follow' gesture", sizeof(acts[2].msg));
			memcpy(acts[2].bytes, no_follow, sizeof no_follow);
			strncpy(acts[3].msg, "Disable 'Zoom' gesture", sizeof(acts[3].msg));
			memcpy(acts[3].bytes, no_zoom, sizeof no_zoom);
			num_acts = 4;
		}
		else if (0 == strcmp(name, "panacea")) {
			strncpy(acts[0].msg, "Wake up camera", sizeof(acts[0].msg));
			memcpy(acts[0].bytes, wake, sizeof wake);
			strncpy(acts[1].msg, "Enable 'Follow' gesture", sizeof(acts[1].msg));
			memcpy(acts[1].bytes, follow, sizeof follow);
			strncpy(acts[2].msg, "Enable 'Zoom' gesture", sizeof(acts[2].msg));
			memcpy(acts[2].bytes, zoom, sizeof zoom);
			num_acts = 3;
		}
		else if (0 == strcmp(name, "morpheus")) {
			strncpy(acts[0].msg, "Suspend camera", sizeof(acts[0].msg));
			memcpy(acts[0].bytes, suspend, sizeof suspend);
			num_acts = 1;
		}
		else {
			fprintf(stderr, "Unknown binary name '%s'. That's not me.\n", name);
			return EXIT_FAILURE;
		}
	}

	// play it safe and force-terminate all message strings
	for (size_t cc = 0; cc < max_acts; ++cc) {
		acts[cc].msg[99] = '\0';
	}
	assert(num_acts <= max_acts && "Too many actions");


	// init libusb
	int ret = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (LIBUSB_SUCCESS != ret) {
		fprintf(stderr, "libusb_init failed: %s (%d)\n", libusb_strerror(ret), ret);
		return EXIT_FAILURE;
	}
	fprintf(stderr, "libusb init done\n");


	// find correct device
	struct libusb_device **devs;
	struct libusb_device *dev;

	if (0 >= libusb_get_device_list(/* ctx=*/ NULL, &devs)) {
		fprintf(stderr, "Failed to enumerate USB devices.\n");
		goto fail_init_done;
	}

	size_t cur_dev = 0;
	while (NULL != (dev = devs[cur_dev++])) {
		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(dev, &desc);
		if (LIBUSB_SUCCESS != ret) {
			fprintf(stderr, "Failed to get device descriptor: %s (%d)\n", libusb_strerror(ret), ret);
			goto fail_list_done;
		}

		if (desc.idVendor == vend_id && desc.idProduct == prod_id) {
			break;
		}
	}

	if (!dev) {
		fprintf(stderr, "Camera not found.\n");
		goto fail_list_done;
	}
	fprintf(stderr, "Camera found.\n");


	// open device
	libusb_device_handle * devh;
	ret = libusb_open(dev, &devh);
	if (LIBUSB_SUCCESS != ret) {
		fprintf(stderr, "Failed to open device: %s (%d)\n", libusb_strerror(ret), ret);
		fprintf(stderr, "Do you have *write* access on the USB device "
		 "(/dev/bus/usb/%03" PRIu8 "/%03" PRIu8 ")?\n",
		 libusb_get_bus_number(dev),
		 libusb_get_device_address(dev)
		);
		goto fail_list_done;
	}


	// enable automatic detaching and re-attaching of kernel driver
	ret = libusb_set_auto_detach_kernel_driver(/*dev_handle=*/devh, /*enable=*/1);
	if (LIBUSB_SUCCESS != ret) {
		fprintf(stderr, "Failed to setup auto detaching of kernel driver: %s (%d)\n", libusb_strerror(ret), ret);
		goto fail_dev_opened;
	}


	// claim interface (required before communication)
	ret = libusb_claim_interface(/*dev_handle=*/devh, interface_num);
	if (LIBUSB_SUCCESS != ret) {
		fprintf(stderr, "Failed to claim interface: %s (%d)\n", libusb_strerror(ret), ret);
		goto fail_dev_opened;
	}
	fprintf(stderr, "Device claimed.\n");


	// send control transfer packet
	for (size_t cc = 0; cc < num_acts; ++cc) {
		uint16_t const wLength = sizeof acts[cc].bytes; // payload length
		assert(cmd_len == wLength && "Weird payload length");

		size_t attempt = 0;
		do {
			ret = libusb_control_transfer(
				/*dev_handle=*/ devh,
				bmRequestType,
				bRequest,
				wValue,
				wIndex,
				acts[cc].bytes,
				wLength,
				/*timeout=*/ 1300 /*ms*/ // arbitrary, nice number
			);
			fprintf(stderr, "%s... (%d bytes)\n", acts[cc].msg, ret);
			if (ret < 0) {
				fprintf(stderr, "libusb_control_transfer failed: %s (%d)\n", libusb_strerror(ret), ret);
				goto fail_dev_claimed;
			}

			// give the camera a brief rest of 42ms (no error check...)
			struct timespec sleep_dur = {.tv_nsec = 42000000};
			nanosleep(&sleep_dur, /* rem= */ NULL);
		// retry on partial transfer but not more often then 'max_retries'
		} while (ret < wLength && (++attempt < max_retries));

		if (attempt >= max_retries) {
			fprintf(stderr, "Failed to fully send command in %zu attempts.\n", attempt);
		}
	}
	fprintf(stderr, "All commands complete.\n");


	// cleanup
	libusb_release_interface(/*dev_handle=*/devh, interface_num);

	// explicitly re-attach kernel driver
	// (should've already been done automatically but sometimes seems to fail)
	libusb_attach_kernel_driver(devh, interface_num);
	libusb_close(/*dev_handle=*/devh);
	libusb_free_device_list(devs, /*unref_devices=*/ 1);
	libusb_exit(/*ctx=*/NULL);
	fprintf(stderr, "Cleanup done\n");
	return EXIT_SUCCESS;


fail_dev_claimed:
	libusb_release_interface(/*dev_handle=*/devh, interface_num);
fail_dev_opened:
	libusb_close(/*dev_handle=*/devh);
fail_list_done:
	libusb_free_device_list(devs, /*unref_devices=*/ 1);
fail_init_done:
	libusb_exit(/*ctx=*/NULL);

	return EXIT_FAILURE;
}
