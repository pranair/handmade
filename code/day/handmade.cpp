#include "handmade.h"

internal void
GameFillSoundBuffer(game_sound_buffer *SoundBuffer, int ToneHz) {
	int16 *SampleOut = (int16 *)SoundBuffer->Samples;
	//int ToneHz = 256;
	int16 WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
	int16 ToneVolume = 1000;
	local_persist real32 tSine;

	for(int i = 0; i < SoundBuffer->SampleCount; i++) {
		real32 SineValue = sinf(tSine);
		int16 Value = (int16)(SineValue * ToneVolume);
		*SampleOut++ = Value;
		*SampleOut++ = Value;
		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
	}
}

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = X + XOffset;
      uint8 Green = Y + YOffset;
      // 0xXXRRGGBB
      *Pixel++ = (Green << 8) | Blue;
    }

    Row = Row + Buffer->Width * Buffer->BytesPerPixel;
  }
}

internal void
GameUpdateAndRender (game_offscreen_buffer *Buffer, game_keyboard_input *KBInput, game_sound_buffer *SoundBuffer) {
	local_persist int XOffset;
	local_persist int YOffset;
	local_persist int ToneHz = 256;

	if (KBInput->IsDown) {
		YOffset -= 20;
	}

	if (KBInput->IsUp) {
		YOffset += 20;
	}

	ToneHz = (int)(((real32)YOffset / 1000) * 256) + 512;

	GameFillSoundBuffer(SoundBuffer, ToneHz);
	RenderWeirdGradeint(Buffer, XOffset, YOffset);
}
