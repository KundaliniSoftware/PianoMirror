
//
// Metronome.c
//
// Benjamin Pritchard / Kundalini Software
//
// Routines for dealing with a metronome, adopted from code on the internet
// This code uses the code in WAVE.C to actually play 'tick' sound on each beat
//
// Usage:
//	InitMetronome()
//	setBMP(180);
//	EnableMetronome();
//	While (1)
//		DoMetronome
//	
//

#include <windows.h>
#include "metronome.h"
#include "wave.h"

MMRESULT TimerID;

void InitMetronome()
{
	bpm = 60;
	timer_flag = 0;
	metronome_enabled = FALSE;
	measure = 0;
	beat = 0;

	CRscWaveOut_INIT();
	LoadWaveResource("TIC", GetModuleHandle(NULL));
}

void KillMetronome() {
	if (metronome_enabled)
		DisableMetronome();

	CRscWaveOut_Destroy();
}


void EnableMetronome() {
	TimerID = timeSetEvent(60 * 1000 / bpm, 0, MetronomeTimerProc, 0, TIME_PERIODIC);
	metronome_enabled = TRUE;
}

void DisableMetronome() {
	timeKillEvent(TimerID);
	metronome_enabled = FALSE;
}

// call this in a tight loop; it increases the beat count, and incremements the current measure, plus plays the .TIC sound
void DoMetronome() {
	if (metronome_enabled && timer_flag)
	{
		if (beat++ == 3) {
			beat = 0;
			measure++;
		}		
		PlayWave();

		timer_flag = 0;
	}
}

void setBMP(const int BMP)
{
	bpm = BMP;
}


// this function is called by timeSetEvent()
// it just sets the timer_flag, which DoMetronome() looks at
void CALLBACK MetronomeTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	timer_flag = 1;
}

