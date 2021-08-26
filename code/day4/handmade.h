#if !defined(HANDMADE_H)

#include <stdint.h>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32 bool32;
typedef float real32;
typedef double real64;

#define Pi32 3.14159265359
#define global_variable static
#define local_persist static
#define internal static

#if HANDMADE_SLOW
#define Assert(Expr) if(!(Expr)) {*(int *)0 = 1;}
#else
#define Assert(Expr) 
#endif

struct game_offscreen_buffer {
  void *Memory;
  int BytesPerPixel;
  int Width;
  int Height;
};

struct game_sound_buffer {
	int16 *Samples;
	int16 SampleCount;
	uint16 SamplesPerSecond;
};

struct game_keyboard_input {
	bool IsUp; // up key
	bool IsDown; // down key
	bool IsLeft; // left key
	bool IsRight; // right key
	bool IsQ; // left key
	bool IsE; // right key
};

struct game_state {
	int XOffset;
	int YOffset;
	int ToneHz;
	real32 tSine;
};


uint32 SafeConvertUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	uint32 ret = (uint32)Value;
	return ret;
}

#if HANDMADE_INTERNAL
struct debug_read_entire_file {
	uint32 ContentSize;
	void* Content;
};

//internal debug_read_entire_file DEBUGPlatformReadEntireFile(char *Filename);
//internal void DEBUGPlatformFreeFileMemory(void *Filememory); 
//internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);

#define PLATFORM_READ_ENTIRE_FILE(name) debug_read_entire_file name(char *Filename)
#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Filememory) 
#define PLATFORM_WRITE_ENTIRE_FILE(name) bool name(char *Filename, uint32 MemorySize, void *Memory)


typedef PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
typedef PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
typedef PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
#endif

struct game_memory {
	uint32 PermenantStorageSize;
	void *PermenantStorage;
	uint32 TempStorageSize;
	void *TempStorage;
	bool IsIntialized;

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};


#define GAME_UPDATE_RENDER(name) void name(game_offscreen_buffer *Buffer, game_keyboard_input *KBInput, game_sound_buffer *SoundBuffer, game_memory *Memory)

//extern "C" GAME_UPDATE_RENDER(GameUpdateAndRender);

#define HANDMADE_H
#endif
