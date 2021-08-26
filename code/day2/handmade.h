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

#define HANDMADE_H
#endif
