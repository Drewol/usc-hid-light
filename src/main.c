#include <hidapi.h>
#include "LightPlugin.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <Windows.h>

static void (*Log)(char* msg) = NULL;

enum SupportedDevice
{
	None,
	PocketVoltex,
	FauceTwo,
	PocketSdvxPico
} currentDevice;

typedef struct
{
	uint8_t r, g, b;
} RGB_t;

typedef struct
{
	uint8_t report_id;
	RGB_t topLeftLeft,
		topLeft,
		topRight,
		topRightRight,
		left,
		reserved_0,
		right;
	uint8_t mode[3];
	uint8_t bta, btb, btc, btd, fxl, fxr, start;
} faucetwoLedReport;

typedef struct
{
	uint8_t report_id;
	RGB_t topleft,
		topright,
		upperleft,
		upperright,
		lowerleft,
		lowerright,
		bottomleft,
		bottomright;
	uint8_t bta, btb, btc, btd, fxl, fxr, start;
} pocketVoltexLedReport;

typedef struct
{
	uint8_t report_id;
	uint8_t bta, btb, btc, btd, fxl, fxr, st;
	RGB_t left;
	RGB_t right;
} pocketPicoReport;

static hid_device* led_device;
static pocketVoltexLedReport pv_report;
static faucetwoLedReport f2_report;
static pocketPicoReport pocket_pico_report;

char* GetName()
{
	return "Officially Supported";
}

#pragma region Pocket Voltex
struct hid_device* open_pocket_voltex_leds()
{
	struct hid_device_info* data = hid_enumerate(0, 0);
	while (data)
	{
		if (data->product_id == 2669 && data->vendor_id == 5840)
		{
			struct hid_device* dev = hid_open_path(data->path);
			if (dev == NULL)
			{
				data = data->next;
				continue;
			}
			wchar_t ps[1024];
			wchar_t ms[1024];
			hid_get_product_string(dev, ps, 1024);
			hid_get_manufacturer_string(dev, ms, 1024);
			if (wcsstr(ps, L"LED") != NULL)
			{
				// Found pocket voltex LEDs
				return dev;
			}
			hid_close(dev);
		}
		data = data->next;
	}
	return NULL;
}

void pv_SetButtons(uint32_t bitfield)
{
	pv_report.bta = bitfield & 1;
	pv_report.btb = (bitfield & (1 << 1)) > 0;
	pv_report.btc = (bitfield & (1 << 2)) > 0;
	pv_report.btd = (bitfield & (1 << 3)) > 0;
	pv_report.fxl = (bitfield & (1 << 4)) > 0;
	pv_report.fxr = (bitfield & (1 << 5)) > 0;
	pv_report.start = (bitfield & (1 << 6)) > 0;
}

void pv_SetLights(uint8_t left, uint32_t pos, RGB_t col)
{
	if (left == 1)
	{
		switch (pos)
		{
		case 0:
			pv_report.bottomleft = col;
			break;
		case 1:
			pv_report.lowerleft = col;
			break;
		case 2:
			pv_report.topleft = col;
			break;
		}
	}
	else
	{
		switch (pos)
		{
		case 0:
			pv_report.bottomright = col;
			break;
		case 1:
			pv_report.lowerright = col;
			break;
		case 2:
			pv_report.topright = col;
			break;
		}
	}
}
#pragma endregion

#pragma region FauceTwo
struct hid_device* open_faucetwo_leds()
{
	struct hid_device_info* data = hid_enumerate(0, 0);
	while (data)
	{
		if (data->product_id == 0x1118 && data->vendor_id == 0x0E8F)
		{
			struct hid_device* dev = hid_open_path(data->path);
			if (dev == NULL)
			{
				data = data->next;
				continue;
			}

			wchar_t ps[1024];
			wchar_t ms[1024];
			uint8_t report[128];
			int report_size = hid_get_input_report(dev, report, 128);

			char msg[512] = { 0 };
			sprintf_s(msg, 512, "Got F2 input report size %d", report_size);
			Log(msg);

			if (report_size > 0)
			{
				f2_report.mode[0] = 0;
				f2_report.mode[1] = 1;
				f2_report.mode[2] = 2;
				return dev;
			}
			hid_close(dev);
		}
		data = data->next;
	}
	return NULL;
}

void f2_SetButtons(uint32_t bitfield)
{
	f2_report.bta = bitfield & 1;
	f2_report.btb = (bitfield & (1 << 1)) > 0;
	f2_report.btc = (bitfield & (1 << 2)) > 0;
	f2_report.btd = (bitfield & (1 << 3)) > 0;
	f2_report.fxl = (bitfield & (1 << 4)) > 0;
	f2_report.fxr = (bitfield & (1 << 5)) > 0;
	f2_report.start = (bitfield & (1 << 6)) > 0;
}

void f2_SetLights(uint8_t left, uint32_t pos, RGB_t col)
{
	if (left == 1)
	{
		switch (pos)
		{
		case 0:
			f2_report.left = col;
			break;
		case 1:
			f2_report.topLeft = col;
			break;
		case 2:
			f2_report.topLeftLeft = col;
		}
	}
	else
	{
		switch (pos)
		{
		case 0:
			f2_report.right = col;
			break;
		case 1:
			f2_report.topRight = col;
			break;
		case 2:
			f2_report.topRightRight = col;
		}
	}
}
#pragma endregion

#pragma region Pocket SDVX Pico
struct hid_device* open_pocket_pico()
{
	struct hid_device_info* data = hid_enumerate(0, 0);
	pocket_pico_report.report_id = 2;
	while (data)
	{
		if (data->product_id == 0x101c && data->vendor_id == 0x1ccf)
		{
			struct hid_device* dev = hid_open_path(data->path);
			if (dev == NULL)
			{
				data = data->next;
				continue;
			}

			wchar_t ps[1024];
			wchar_t ms[1024];
			uint8_t report[128];
			report[0] = 2;
			int report_size = hid_get_input_report(dev, report, 128);
			char msg[512] = { 0 };
			sprintf_s(msg, 512, "Got Pocket SDVX Pico input report size %d", report_size);
			Log(msg);

			if (report_size > 0)
			{
				return dev;
			}
			hid_close(dev);
		}
		data = data->next;
	}
}

void pocket_pico_SetButtons(uint32_t bitfield) {
	pocket_pico_report.bta = bitfield & 1;
	pocket_pico_report.btb = (bitfield & (1 << 1)) > 0;
	pocket_pico_report.btc = (bitfield & (1 << 2)) > 0;
	pocket_pico_report.btd = (bitfield & (1 << 3)) > 0;
	pocket_pico_report.fxl = (bitfield & (1 << 4)) > 0;
	pocket_pico_report.fxr = (bitfield & (1 << 5)) > 0;
	pocket_pico_report.st = (bitfield & (1 << 6)) > 0;
}

void pocket_pico_SetLights(uint8_t left, uint32_t pos, RGB_t color) {
	if (pos == 1)
	{
		if (left) {
			pocket_pico_report.left = color;
		}
		else {
			pocket_pico_report.right = color;
		}
	}
}

#pragma endregion

void SetButtons(uint32_t bitfield)
{
	switch (currentDevice)
	{
	case PocketVoltex:
		pv_SetButtons(bitfield);
		break;
	case FauceTwo:
		f2_SetButtons(bitfield);
		break;
	case PocketSdvxPico:
		pocket_pico_SetButtons(bitfield);
		break;
	default:
		break;
	}
}

void SetLights(uint8_t left, uint32_t pos, uint8_t r, uint8_t g, uint8_t b)
{
	RGB_t col;
	col.r = r;
	col.g = g;
	col.b = b;

	switch (currentDevice)
	{
	case PocketVoltex:
		pv_SetLights(left, pos, col);
		break;
	case FauceTwo:
		f2_SetLights(left, pos, col);
		break;
	case PocketSdvxPico:
		pocket_pico_SetLights(left, pos, col);
	default:
		break;
	}
}

void Tick(float deltaTime)
{
	int res = -1;
	switch (currentDevice)
	{
	case PocketVoltex:
		res = hid_write(led_device, &pv_report, sizeof(pv_report));
		break;
	case FauceTwo:
		res = hid_write(led_device, &f2_report, sizeof(f2_report));
		break;
	case PocketSdvxPico:
		res = hid_write(led_device, &pocket_pico_report, sizeof(pocket_pico_report));
		break;
	default:
		break;
	}

	if (res == -1) {
		wchar_t* err_msg = hid_error(led_device);
		char msg[512] = { 0 };
		sprintf_s(msg, 512, "Write Error: %ls", err_msg);
	}
}

int Init(void (*logFunc)(char*))
{
	currentDevice = None;
	Log = logFunc;
	hid_init();
	led_device = open_pocket_voltex_leds();
	if (led_device != NULL)
	{
		return 0;
		currentDevice = PocketVoltex;
	}

	led_device = open_faucetwo_leds();
	if (led_device != NULL)
	{
		currentDevice = FauceTwo;
		return 0;
	}

	led_device = open_pocket_pico();
	if (led_device != NULL)
	{
		currentDevice = PocketSdvxPico;
		return 0;
	}

	Log("Couldn't find any compatible LED device.\n");
	return 1;
}

int Close()
{
	if (led_device)
	{
		hid_close(led_device);
		led_device = NULL;
	}
	hid_exit();
	return 0;
}
