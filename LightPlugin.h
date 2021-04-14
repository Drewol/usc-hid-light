#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

typedef struct LightPlugin {
	char* (*GetName)();
	void (*SetButtons)(uint32_t bitfield);
	void (*Tick)(float deltaTime);

	//6 rgb sections, left/right + pos: (bottom/middle/top) = (0/1/2)
	void (*SetLights)(uint8_t left, uint32_t pos, uint8_t r, uint8_t g, uint8_t b);
	int (*Init)(void(*)(char*));
	int (*Close)();
} LightPlugin;

__declspec(dllimport) char* GetName();
__declspec(dllimport) void SetButtons(uint32_t bitfield);
__declspec(dllimport) void SetLights(uint8_t left, uint32_t pos, uint8_t r, uint8_t g, uint8_t b);
__declspec(dllimport) void Tick(float deltaTime);

//Return 0 on success
__declspec(dllimport) int Init(void(*)(char*));
__declspec(dllimport) int Close();

#ifdef __cplusplus
}
#endif