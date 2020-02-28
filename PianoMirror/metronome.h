#pragma once

#include "windows.h"

int bpm;
int metronome_enabled;
int timer_flag;
int beat;
int measure;

void InitMetronome();
void KillMetronome();
void EnableMetronome();
void DisableMetronome();
void DoMetronome();
void CALLBACK MetronomeTimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);



