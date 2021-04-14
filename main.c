#include "hidapi/hidapi/hidapi.h"
#include "LightPlugin.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <Windows.h>

static void (*Log)(char* msg) = NULL;

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
				//Found pocket voltex LEDs
				return dev;
			}
			hid_close(dev);
		}
		data = data->next;
	}
	return NULL;
}

typedef struct {
	uint8_t r, g, b;
} RGB_t;

typedef struct {
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
} ledReport;

static hid_device* pv_leds;
static ledReport report;

char* GetName()
{
	return "Pocket Voltex LEDs";
}

void SetButtons(uint32_t bitfield)
{
	report.bta = bitfield & 1;
	report.btb = (bitfield & (1 << 1)) > 0;
	report.btc = (bitfield & (1 << 2)) > 0;
	report.btd = (bitfield & (1 << 3)) > 0;
	report.fxl = (bitfield & (1 << 4)) > 0;
	report.fxr = (bitfield & (1 << 5)) > 0;
	report.start = (bitfield & (1 << 6)) > 0;
}

void SetLights(uint8_t left, uint32_t pos, uint8_t r, uint8_t g, uint8_t b)
{
	if (left == 1)
	{
		switch (pos)
		{
		case 0:
			report.bottomleft.r = r;
			report.bottomleft.g = g;
			report.bottomleft.b = b;
			break;
		case 1:
			report.lowerleft.r = r;
			report.lowerleft.g = g;
			report.lowerleft.b = b;
			break;
		case 2:
			report.topleft.r = r;
			report.topleft.g = g;
			report.topleft.b = b;
		}
	}
	else
	{
		switch (pos)
		{
		case 0:
			report.bottomright.r = r;
			report.bottomright.g = g;
			report.bottomright.b = b;
			break;
		case 1:
			report.lowerright.r = r;
			report.lowerright.g = g;
			report.lowerright.b = b;
			break;
		case 2:
			report.topright.r = r;
			report.topright.g = g;
			report.topright.b = b;
		}
	}
}


void Tick(float deltaTime)
{
	int res = hid_write(pv_leds, &report, sizeof(report));
}


int Init(void(*logFunc)(char*))
{
	Log = logFunc;
	hid_init();
	pv_leds = open_pocket_voltex_leds();
	if (pv_leds == NULL)
	{
		printf("Couldn't find pocket voltex LEDs\n");
		return 1;
	}
	return 0;
}

int Close()
{
	if (pv_leds)
	{
		hid_close(pv_leds);
		pv_leds = NULL;
	}
	hid_exit();
	return 0;
}
