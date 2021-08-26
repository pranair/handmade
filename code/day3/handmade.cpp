#include "handmade.h"

internal void
GameFillSoundBuffer(game_sound_buffer *SoundBuffer, game_state *State) {
	int16 *SampleOut = (int16 *)SoundBuffer->Samples;
	//int ToneHz = 256;
	int16 WavePeriod = (int16)(SoundBuffer->SamplesPerSecond / State->ToneHz);
	int16 ToneVolume = 1000;
	local_persist real32 tSine;
	//if (tSine > 10000) {
	//tSine = 0;
	//}

	for(int i = 0; i < SoundBuffer->SampleCount; i++) {
		real32 SineValue = sinf(tSine);
		int16 Value = (int16)(SineValue * ToneVolume);
		*SampleOut++ = Value;
		*SampleOut++ = Value;
		tSine += (real32) (2.0f * Pi32 * 1.0f / (real32)WavePeriod);
	}
}

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, game_state *State) {
	uint8 *Row = (uint8 *)Buffer->Memory;

	for(int Y = 0; Y < Buffer->Height; Y++) {
		uint32 *Pixel = (uint32 *)Row;

		for(int X = 0; X < Buffer->Width; X++) {
			uint8 Blue = (uint8) (X + State->XOffset);
			uint8 Green = (uint8) (Y + State->YOffset);
			// 0xXXRRGGBB
			*Pixel++ = (Green << 8) | Blue;
		}

		Row = Row + Buffer->Width * Buffer->BytesPerPixel;
	}
}

internal void
GameUpdateAndRender (game_offscreen_buffer *Buffer, game_keyboard_input *KBInput,
					 game_sound_buffer *SoundBuffer, game_memory *Memory) {
	Assert(sizeof(game_state) <= Memory->PermenantStorageSize);
	local_persist game_state *State = (game_state *) Memory->PermenantStorage;

	/*
	debug_read_entire_file File = DEBUGPlatformReadEntireFile("C:\\Users\\Pratyush Nair\\Documents\\code\\boring\\build\\test.txt");
	if (File.Content) {
		DEBUGPlatformWriteEntireFile("C:\\Users\\Pratyush Nair\\Documents\\code\\boring\\build\\out.txt", File.ContentSize, File.Content);
		DEBUGPlatformFreeFileMemory(File.Content);
	} else {
		
	}
	*/
	
	if (!Memory->IsIntialized) {
		State->ToneHz = 256;
		Memory->IsIntialized = true;
	}

	if (KBInput->IsDown) {
		State->YOffset -= 20;
	}

	if (KBInput->IsUp) {
		State->YOffset += 20;
	}

	State->ToneHz = (int)(((real32)State->YOffset / 1000) * 256) + 250;

	GameFillSoundBuffer(SoundBuffer, State);
	RenderWeirdGradeint(Buffer, State);
}
