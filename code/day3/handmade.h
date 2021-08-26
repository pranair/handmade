#if !defined(HANDMADE_H)

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

struct game_memory {
	uint32 PermenantStorageSize;
	void *PermenantStorage;
	uint32 TempStorageSize;
	void *TempStorage;
	bool IsIntialized;
};

struct game_state {
	int XOffset;
	int YOffset;
	int ToneHz;
};


uint32 SafeConvertUInt64(uint64 Value) {
	Assert(Value <= 0xFFFFFFFF);
	uint32 ret = (uint32)Value;
	return ret;
}

#if HANDMADE_INTERNAL
struct debug_read_entire_file {
	uint32 ContentSize;
	void* Content;
};

internal debug_read_entire_file DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Filememory); 
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory); 
#endif


#define HANDMADE_H
#endif
