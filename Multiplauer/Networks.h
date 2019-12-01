#pragma once

#pragma comment(lib, "Ws2_32.lib")

#pragma comment (lib, "D3D11.lib")


////////////////////////////////////////////////////////////////////////
// USEFUL MACROS
////////////////////////////////////////////////////////////////////////

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(x) if ((x) == false) { *(int*)0 = 0; }

#define Kilobytes(x) (1024L * x)
#define Megabytes(x) (1024L * Kilobytes(x))
#define Gigabytes(x) (1024L * Megabytes(x))
#define Terabytes(x) (1024L * Gigabytes(x))

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define MAX_LOG_ENTRIES                                  256
#define MAX_LOG_ENTRY_LENGTH                    Kilobytes(1)
#define MAX_SCREENS                                       32
#define MAX_TASKS                                        128
#define MAX_TEXTURES                                     512
#define MAX_GAME_OBJECTS                                4096
#define MAX_COLLIDERS                       MAX_GAME_OBJECTS
#define MAX_CLIENTS                                       32
#define MAX_NETWORK_OBJECTS                              256

#define SCENE_TRANSITION_TIME_SECONDS                   1.0f
#define DISCONNECT_TIMEOUT_SECONDS                     10.0f
#define PACKET_DELIVERY_TIMEOUT_SECONDS                 0.5f
#define PACKET_SIZE                             Kilobytes(10)
#define PING_INTERVAL_SECONDS                           0.5f
#define REPLICATION_INTERVAL_SECONDS                    0.1f
#define INPUT_DELIVERY_INTERVAL_SECONDS					0.1f

////////////////////////////////////////////////////////////////////////
// BASIC TYPES
////////////////////////////////////////////////////////////////////////

// NOTE(jesus): These sizes are right for most desktop platforms, but we
// should be cautious about this because they could vary somewhere...

typedef char int8;
typedef short int int16;
typedef long int int32;
typedef long long int int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;
typedef unsigned long long int uint64;

typedef float real32;
typedef double real64;


////////////////////////////////////////////////////////////////////////
// FRAMEWORK TYPES
////////////////////////////////////////////////////////////////////////

struct GameObject;
struct Texture;
struct Collider;
struct Behaviour;
class Task;
class Screen;

enum class ColliderType
{
	None,
	Player,
	Laser
};


////////////////////////////////////////////////////////////////////////
// WINDOW
////////////////////////////////////////////////////////////////////////

struct WindowStruct
{
	int width = 0;
	int height = 0;
};

// NOTE(jesus): Global object to access the window
extern WindowStruct Window;


////////////////////////////////////////////////////////////////////////
// TIME
////////////////////////////////////////////////////////////////////////

struct TimeStruct
{
	double time = 0.0f;     // NOTE(jesus): Time in seconds since the application started
	float deltaTime = 0.0f; // NOTE(jesus): Fixed update time step (use this for calculations)
	float frameTime = 0.0f; // NOTE(jesus): Time spend during the last frame (don't use this)
};

// NOTE(jesus): Global object to access the time
extern TimeStruct Time;


////////////////////////////////////////////////////////////////////////
// INPUT
////////////////////////////////////////////////////////////////////////

enum ButtonState { Idle, Press, Pressed, Release };

struct InputController
{
	bool isConnected = false;

	float verticalAxis = 0.0f;
	float horizontalAxis = 0.0f;

	union
	{
		ButtonState buttons[8] = {};
		struct
		{
			ButtonState actionUp;
			ButtonState actionDown;
			ButtonState actionLeft;
			ButtonState actionRight;
			ButtonState leftShoulder;
			ButtonState rightShoulder;
			ButtonState back;
			ButtonState start;
		};
	};
};

// NOTE(jesus): Global object to access the input controller
extern InputController Input;

struct MouseController
{
	int16 x = 0;
	int16 y = 0;
	ButtonState buttons[5] = {};
};

// NOTE(jesus): Global object to access the mouse
extern MouseController Mouse;


////////////////////////////////////////////////////////////////////////
// LOG
////////////////////////////////////////////////////////////////////////

// NOTE(jesus):
// Use log just like standard printf function.
// Example: LOG("New user connected %s\n", usernameString);

enum { LOG_TYPE_INFO, LOG_TYPE_WARN, LOG_TYPE_ERROR, LOG_TYPE_DEBUG };

struct LogEntry {
	double time;
	int type;
	char message[MAX_LOG_ENTRY_LENGTH];
};

void log(const char file[], int line, int type, const char* format, ...);
LogEntry getLogEntry(uint32 entryIndex);
uint32 getLogEntryCount();

#define LOG(format, ...)  log(__FILE__, __LINE__, LOG_TYPE_INFO,  format, __VA_ARGS__)
#define WLOG(format, ...) log(__FILE__, __LINE__, LOG_TYPE_WARN,  format, __VA_ARGS__)
#define ELOG(format, ...) log(__FILE__, __LINE__, LOG_TYPE_ERROR, format, __VA_ARGS__)
#define DLOG(format, ...) log(__FILE__, __LINE__, LOG_TYPE_DEBUG, format, __VA_ARGS__)


////////////////////////////////////////////////////////////////////////
// RANDOM NUMBER
////////////////////////////////////////////////////////////////////////

class RandomNumberGenerator
{
public:

	RandomNumberGenerator(uint32 seed = 987654321)
	{
		// NOTE(jesus): VERY IMPORTANT
		// The initial seeds z1, z2, z3, z4  MUST be larger than
		// 1, 7, 15, and 127 respectively.
		z1 = seed, z2 = seed, z3 = seed, z4 = seed;
	}

	float next(void)
	{
		uint32 b;
		b = ((z1 << 6) ^ z1) >> 13;
		z1 = ((z1 & 4294967294U) << 18) ^ b;
		b = ((z2 << 2) ^ z2) >> 27;
		z2 = ((z2 & 4294967288U) << 2) ^ b;
		b = ((z3 << 13) ^ z3) >> 21;
		z3 = ((z3 & 4294967280U) << 7) ^ b;
		b = ((z4 << 3) ^ z4) >> 12;
		z4 = ((z4 & 4294967168U) << 13) ^ b;
		float result = (float)((z1 ^ z2 ^ z3 ^ z4) * 2.3283064365386963e-10);
		ASSERT(result >= 0.0f && result <= 1.0f);
		return result;
	}

private:

	uint32 z1, z2, z3, z4;
};

// NOTE(jesus): Global random generation object
extern RandomNumberGenerator Random;


////////////////////////////////////////////////////////////////////////
// FRAMEWORK HEADERS
////////////////////////////////////////////////////////////////////////

#include "Maths.h"
#include "Messages.h"
#include "ByteSwap.h"
#include "MemoryStream.h"
#include "DeliveryManager.h"
#include "ReplicationCommand.h"
#include "ReplicationManagerClient.h"
#include "ReplicationManagerServer.h"
#include "Module.h"
#include "ModuleNetworking.h"
#include "ModuleNetworkingCommons.h"
#include "ModuleNetworkingClient.h"
#include "ModuleNetworkingServer.h"
#include "ModuleLinkingContext.h"
#include "ModuleGameObject.h"
#include "ModuleCollision.h"
#include "ModulePlatform.h"
#include "ModuleRender.h"
#include "ModuleResources.h"
#include "ModuleScreen.h"
#include "ModuleTaskManager.h"
#include "ModuleTextures.h"
#include "ModuleUI.h"
#include "Screen.h"
#include "ScreenLoading.h"
#include "ScreenBackground.h"
#include "ScreenOverlay.h"
#include "ScreenMainMenu.h"
#include "ScreenGame.h"
#include "Application.h"
#include "Behaviours.h"
