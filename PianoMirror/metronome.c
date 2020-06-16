
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
	bpm = 100;
	timer_flag = 0;
	metronome_enabled = FALSE;
	measure = 0;
	beat = 0;
	beats_per_measure = 0;

	CRscWaveOut_INIT();
	LoadWaveResources("wave1", "wave2", GetModuleHandle(NULL));	

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

		if (beat == 0 || beats_per_measure == 0)
			PlayWave(0);
		else
			PlayWave(1);

		if (beats_per_measure == 0)
		{ 
			beat++;
		} else
			if (beat++ == (beats_per_measure-1)) {
				beat = 0;
				measure++;
			}	

		timer_flag = 0;
	}
}

void setBeatsPerMinute(const int BPM)
{
	DisableMetronome();
	bpm = BPM;
	EnableMetronome();

}

void setBeatsPerMeasure(const int BeatsPerMeasure)
{
	beat = 0;
	measure = 0;
	beats_per_measure = BeatsPerMeasure;
}


// this function is called by timeSetEvent()
// it just sets the timer_flag, which DoMetronome() looks at
void CALLBACK MetronomeTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
	timer_flag = 1;
}

