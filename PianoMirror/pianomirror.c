//
// PianoMirror.c
// https://www.kundalinisoftware.com/piano-mirror/
//
// Benjamin Pritchard / Kundalini Software
//
// Program to perform real-time MIDI remapping. 
//
// This is the Windows version of this code. It began life as a port of the ANSI C code I wrote under Linux, based on examples
// from portMidi. 
//
// Over the course of time, this gained more Windows-specific features. Additionally, as of version 1.4, I added support for user-defined
// .LUA scripts, which allowed this code to quit being "just a piano mirror", but instead to begin to support other creative uses of real-time
// MIDI remapping. What I am doing with it currently, is using it to implement "computer assisted dynamics".
//
// Version History 
//
//		1.0		9-Feb-2019		Initial Version
//		1.1		16-Feb-2019		Added ability to cycle through transposing modes using Low A on the piano
//		1.2		15-March-2019	Project cleanup, added version resource
//		1.3		4-April-2019	Updated to include icon
//		1.4		20-Feb-2020		Added LUA scripting language to process scripts during MIDI call backs
//		1.5		22-Feb-2020		Added metronome functionality
//		1.6		15-June-2020	Bug Fixes to metronome functionality; also added ability to use two .WAV files; one for the downbeat; and implemented ability to set beats per measure
//		1.7		22-June-2020	Added CMD_SET_SPLIT_POINT functionality
//

// This string must be updated here, as well as in PianoMirro.rc!!!
const char *VersionString = "1.7";				

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <conio.h> 
#include <windows.h>

#include "portmidi\portmidi.h"
#include "portmidi\pmutil.h"
#include "portmidi\porttime.h"

#include "lua\include\lua.h"
#include "lua\include\lualib.h"
#include "lua\include\lauxlib.h"

#include "metronome.h"

#pragma warning(disable:4996)			// need to use scanf MS Visual C

// update this eventually...
char SCRIPT_LOCATION[] = "C:\\kundalini\\lua scripts\\";

// message queues for the main thread to communicate with the call back
PmQueue *callback_to_main;
PmQueue *main_to_callback;

#define STRING_MAX	80

PmStream *midi_in;		// incoming midi data from piano
PmStream *midi_out;		// (transposed) outgoing midi 

char script_file[255];

#define IN_QUEUE_SIZE	1024		
#define OUT_QUEUE_SIZE	1024

// simple structure to pass messages back and forth between the main thread and our callback
// these messages are inserted into 
typedef struct {
	
	int cmdCode;
	int Param1;
	int Param2;

} CommandMessage;

// messages from the main thread
#define CMD_QUIT_MSG		1
#define CMD_SET_SPLIT_POINT	2
#define CMD_SET_MODE		3

// ackknowledgement of received message
#define	CMD_MSG_ACK			1000

// flag indicating 
//int callback_exit_flag;

// transposition modes we support
enum transpositionModes {NO_TRANSPOSITION, LEFT_ASCENDING, RIGHT_DESCENDING, MIRROR_IMAGE};

enum transpositionModes transpositionMode = NO_TRANSPOSITION;
int splitPoint = 62;	// default to middle d

lua_State* Lua_State;

int finished = FALSE;

// 0 means no threshold; just let through all notes... 
// otherwise, this number represents the highest velocity number that we will let through
int velocityThreshhold = 0;	

int callback_active = FALSE;

int script_is_loaded = FALSE;

int debug_mode = FALSE;

// takes an input node, and maps it according to current transposition mode
PmMessage TransformNote(PmMessage Note) {

	PmMessage	retval = Note;
	int			offset;
	
	switch (transpositionMode) {

	case NO_TRANSPOSITION:
		// do nothing, just return original note!
		retval = Note; 
		break;
		
	// make left hand ascend
	case LEFT_ASCENDING:
		if (Note < splitPoint) {
			offset = (splitPoint - Note);
			retval = splitPoint + offset;
		} // else do nothing;
		break;
		
	// make right hand descend
	case RIGHT_DESCENDING:
		if (Note > splitPoint) {
			offset = (Note - splitPoint);
			retval = splitPoint - offset;
		} // else do nothing;
		break;
	
	// completely reverse the keyboard
	case MIRROR_IMAGE:
		if (Note == splitPoint) {
			// do nothing
		}
		else if (Note < splitPoint) {
			offset = (splitPoint - Note);
			retval = splitPoint + offset;
		}
		else if (Note > splitPoint) {
			offset = (Note - splitPoint);
			retval = splitPoint - offset;
		}
		break;
	}

	return retval;
}

// cycles through the transposition modes in turn
// this routine is called when we detect a LOW A on the piano [which isn't used much, so we can just use it for input like this]
void DoNextTranspositionMode() {

	switch (transpositionMode) {

	case NO_TRANSPOSITION:
		transpositionMode = LEFT_ASCENDING;
		printf("Left hand ascending mode active\n");
		break;

	case LEFT_ASCENDING:
		transpositionMode = RIGHT_DESCENDING;
		printf("Right Hand Descending mode active\n");
		break;

	case RIGHT_DESCENDING:
		transpositionMode = MIRROR_IMAGE;
		printf("Keyboard mirring mode active\n");
		break;

	case MIRROR_IMAGE:
		transpositionMode = NO_TRANSPOSITION;
		printf("no tranposition active\n");
		break;

	}

}

void exit_with_message(char *msg)
{
	char line[STRING_MAX];
	printf("%s\nType ENTER...", msg);
	fgets(line, STRING_MAX, stdin);
	exit(1);
}

void ProcessOldAutoLegatos() {

}

// callback function 
void process_midi(PtTimestamp timestamp, void *userData)
{

	PmError result;
	PmEvent buffer;

	CommandMessage cmd;					// incoming message from main()
	CommandMessage response;			// our responses back to main()

	// if we're not intialized, do nothing
	if (!callback_active) {
		return;
	}

	DoMetronome();

	// process messages from the main thread
	do {
		result = Pm_Dequeue(main_to_callback, &cmd);
		if (result) {
			switch (cmd.cmdCode) {
				case CMD_QUIT_MSG:
					response.cmdCode = CMD_MSG_ACK;
					Pm_Enqueue(callback_to_main, &response);
					callback_active = FALSE;
					return;
					//no break needed; above statement just exits function
				case CMD_SET_SPLIT_POINT:
					splitPoint = (cmd.Param1);
					response.cmdCode = CMD_MSG_ACK;
					Pm_Enqueue(callback_to_main, &response);
					break;
				case CMD_SET_MODE:
					transpositionMode = (cmd.Param1);
					response.cmdCode = CMD_MSG_ACK;
					Pm_Enqueue(callback_to_main, &response);
					break;
			}
		}
	} while (result);

	// process incoming midi data, performing transposion as necessary
	do {
		result = Pm_Poll(midi_in);
		if (result) {
			int status, data1, data2;
			if (Pm_Read(midi_in, &buffer, 1) == pmBufferOverflow)
				continue;

			status = Pm_MessageStatus(buffer.message);
			data1 = Pm_MessageData1(buffer.message);
			data2 = Pm_MessageData2(buffer.message);

			if (debug_mode) {
				printf("status = %d, data1 = %d, data2 = %d \n", status, data1, data2);
			}

			// do transposition logic
			PmMessage NewNote = TransformNote(data1);

			// do logic associated with quite mode
			int shouldEcho = (data2 < velocityThreshhold) || (velocityThreshhold == 0);

			// Run any user-defined .LUA scripts

			if (Lua_State) {

				// Push the fib function on the top of the lua stack
				lua_getglobal(Lua_State, "process_midi");

				lua_pushnumber(Lua_State, status);
				lua_pushnumber(Lua_State, data1);
				lua_pushnumber(Lua_State, data2);

				lua_call(Lua_State, 3, 3);

				// Get the result from the lua stack
				status = (int)lua_tointeger(Lua_State, -3);
				data1 = (int)lua_tointeger(Lua_State, -2);
				data2 = (int)lua_tointeger(Lua_State, -1);

				// Clean up.  If we don't do this last step, we'll leak stack memory.
				lua_pop(Lua_State, 3);

			}

			buffer.message =
				Pm_Message(status, NewNote, data2);

			if (shouldEcho)
				Pm_Write(midi_out, &buffer, 1);

			// very bottom note on the piano can be used to toggle through the transpotion modes...
			// (it is not used very much anyway)
			if (data1 == 21 && data2 == 0) {
				DoNextTranspositionMode();
			}

		}
	} while (result);

}

void initialize()
{
	const PmDeviceInfo *info;
	int id;

	/* make the message queues */
	main_to_callback = Pm_QueueCreate(IN_QUEUE_SIZE, sizeof(CommandMessage));
	assert(main_to_callback != NULL);
	callback_to_main = Pm_QueueCreate(OUT_QUEUE_SIZE, sizeof(CommandMessage));
	assert(callback_to_main != NULL);

	Pt_Start(1, &process_midi, 0);
	Pm_Initialize();

	// open default output device
	id = Pm_GetDefaultOutputDeviceID();
	info = Pm_GetDeviceInfo(id);
	if (info == NULL) {
		printf("Could not open default output device (%d).", id);
		exit_with_message("");
	}
	printf("Opening output device %s %s\n", info->interf, info->name);

	Pm_OpenOutput(&midi_out,
		id,
		NULL,
		OUT_QUEUE_SIZE,
		NULL,
		NULL,
		0);

	// open default midi input device
	id = Pm_GetDefaultInputDeviceID();
	info = Pm_GetDeviceInfo(id);
	if (info == NULL) {
		printf("Could not open default input device (%d).", id);
		exit_with_message("");
	}
	printf("Opening input device %s %s\n", info->interf, info->name);
	Pm_OpenInput(&midi_in,
		id,
		NULL,
		0,
		NULL,
		NULL);

	Pm_SetFilter(midi_in, PM_FILT_ACTIVE | PM_FILT_CLOCK);

	callback_active = TRUE;
}

void shutdownSystem()
{

	KillMetronome();

	// close down our lua interpreter 
	if (Lua_State) {
		lua_close(Lua_State);
	}

	// shut down our .MIDI interface
	// we just ignore all errors; nothing we can do anyway...

	Pt_Stop();
	Pm_QueueDestroy(callback_to_main);
	Pm_QueueDestroy(main_to_callback);

	Pm_Close(midi_in);
	Pm_Close(midi_out);

	Pm_Terminate();

}

// send a quit message to the callback function
// then wait around until it sends us an ACK back
void signalExitToCallBack() {

	int gotFinalAck;

	CommandMessage msg;
	CommandMessage response;

	// send a quit message to the callback
	msg.cmdCode = CMD_QUIT_MSG;
	Pm_Enqueue(main_to_callback, &msg);

	// wait for the callback to send back acknowledgement
	gotFinalAck = FALSE;
	do {
		if (Pm_Dequeue(callback_to_main, &response) == 1) {
			if (response.cmdCode == CMD_MSG_ACK) {
				int i = 1;
				gotFinalAck = TRUE;
			}
		}
	} while (!gotFinalAck);

}

// send change mode command 
// then wait around until it sends us an ACK back
void set_transposition_mode(enum transpositionModes newmode) {

	int receivedAck;

	CommandMessage msg;
	CommandMessage response;

	msg.cmdCode = CMD_SET_MODE;
	msg.Param1 = newmode;
	Pm_Enqueue(main_to_callback, &msg);

	// wait for the callback to send back acknowledgement
	receivedAck = FALSE;
	do {
		if (Pm_Dequeue(callback_to_main, &response) == 1) {
			if (response.cmdCode == CMD_MSG_ACK) {
				receivedAck = TRUE;
			}
		}
	} while (!receivedAck);

}

// send change split point command
// then wait around until it sends us an ACK back
void set_keyboard_splitpoint(const int new_split_point) {

	int receivedAck;

	CommandMessage msg;
	CommandMessage response;

	msg.cmdCode = CMD_SET_SPLIT_POINT;
	msg.Param1 = new_split_point;
	Pm_Enqueue(main_to_callback, &msg);

	// wait for the callback to send back acknowledgement
	receivedAck = FALSE;
	do {
		if (Pm_Dequeue(callback_to_main, &response) == 1) {
			if (response.cmdCode == CMD_MSG_ACK) {
				receivedAck = TRUE;
			}
		}
	} while (!receivedAck);

}

/*
* Check if a file exist using fopen() function
* return 1 if the file exist otherwise return 0
*/
int fileexists(const char * filename) {
	/* try to open file to read */
	FILE *file;
	if (file = fopen(filename, "r")) {
		fclose(file);
		return 1;
	}
	return 0;
}

void LoadLuaScript()
{
	
	char tmp[255];

	if (Lua_State) {
		lua_close(Lua_State);
	}

	// each time we call this function, we create a new environment
	// this is so that we can have a script loaded... then change it, and reload our changes 
	Lua_State = luaL_newstate();
	luaL_openlibs(Lua_State);

	printf("Enter lua script: ");
	
	if (scanf("%s", tmp) == 1) {

		strcpy(script_file, SCRIPT_LOCATION);
		strcat(script_file, tmp);

		if (fileexists(script_file))
		{
			luaL_dofile(Lua_State, script_file);
			script_is_loaded = TRUE;
		}
		else
			printf("error loading lau script: %s\n", script_file);
	}

}

// resets the LUA state, and reloads [restarts] the last script we had loaded
void ReLoadLuaScript()
{

	if (Lua_State) {
		lua_close(Lua_State);
	}

	// each time we call this function, we create a new environment
	// this is so that we can have a script loaded... then change it, and reload our changes 
	Lua_State = luaL_newstate();
	luaL_openlibs(Lua_State);

	if (fileexists(script_file))
		luaL_dofile(Lua_State, script_file);
	else
		printf("error loading lau script\n");
	
}

// returns TRUE if the loaded script has been modified, so that we can reload it
int ShouldReloadFile(char* filename)
{

	static int isFirstTime = TRUE;
	static FILETIME old_Write;

	FILETIME ftCreate, ftAccess, ftWrite;
	HANDLE hFile;

	hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	// Retrieve the file times for the file.
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
		CloseHandle(hFile);
		return FALSE;
	}

	// if this is the first time we are checking, then we definitely shouldn't reload the file
	if (isFirstTime) {
		isFirstTime = FALSE;
		old_Write.dwLowDateTime = ftWrite.dwLowDateTime;
		old_Write.dwHighDateTime = ftWrite.dwHighDateTime;
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);
	int retval = (old_Write.dwLowDateTime != ftWrite.dwLowDateTime || old_Write.dwHighDateTime != ftWrite.dwHighDateTime);

	old_Write.dwLowDateTime = ftWrite.dwLowDateTime;
	old_Write.dwHighDateTime = ftWrite.dwHighDateTime;

	return retval;

}

void PrintHelp()
{
	printf("commands:\n"
	" 0  [enter] for no transposing \n"
	" 1  [enter] for left ascending mode \n"
	" 2  [enter] for right hand descending mode \n"
	" 3  [enter] for mirror image mode\n"
	" 4  [enter] for quiet mode\n"
	" 5  [enter] to load lua script\n"
	" 6  [enter] clear lua state\n"
	" 7  [enter] reload last script\n"
	" 8  [enter] set metronome BMP\n"
	" 9  [enter] set time signature\n"
	" 10 [enter] enable\\disable metronome\n"
	" 11 [enter] set keyboard split point\n"
	" q  [enter] to quit\n");

}

void HandleKeyboard()
{
	int len;
	char line[STRING_MAX];

	fgets(line, STRING_MAX, stdin);
	len = strlen(line);
	if (len > 0) line[len - 1] = 0;

	if (strcmp(line, "q") == 0) {
		signalExitToCallBack();
		finished = TRUE;
	}

	if (strcmp(line, "?") == 0) {
		PrintHelp();
	}

	if (strcmp(line, "") == 0) {
		PrintHelp();
	}

	if (strcmp(line, "0") == 0) {
		set_transposition_mode(NO_TRANSPOSITION);
		printf("no tranposition active\n");
	}

	if (strcmp(line, "1") == 0) {
		set_transposition_mode(LEFT_ASCENDING);
		printf("Left hand ascending mode active\n");
	}

	if (strcmp(line, "2") == 0) {
		set_transposition_mode(RIGHT_DESCENDING);
		printf("Right Hand Descending mode active\n");
	}

	if (strcmp(line, "3") == 0) {
		set_transposition_mode(MIRROR_IMAGE);
		printf("Keyboard mirring mode active\n");
	}

	if (strcmp(line, "4") == 0) {
		printf("Enter volocity threshold, or 0 to disable quiet mode: ");
		int n;
		if (scanf("%d", &n) == 1) {
			velocityThreshhold = n;
			if (n == 0) printf("quiet mode turned off\n");
			else printf("threshold set to %d\n", n);
		}
	}

	if (strcmp(line, "5") == 0) {
		LoadLuaScript();
	}

	if (strcmp(line, "6") == 0) {
		if (Lua_State) {
			lua_close(Lua_State);
			Lua_State = 0;
		}
	}

	if (strcmp(line, "7") == 0) {
		ReLoadLuaScript();
	}

	if (strcmp(line, "8") == 0) {
		printf("Enter bpm: ");
		int n;
		if (scanf("%d", &n) == 1) {
			setBeatsPerMinute(n);							// this will reinitialize the timer for the metronome to the right frequency for us...
			printf("bmp set to %d\n", n);
		}
	}

	if (strcmp(line, "9") == 0) {

		printf("\nTime Signatures:\n"
			" 0 [enter] none (just click on each beat) \n"
			" 1 [enter] 2/4 \n"
			" 2 [enter] 3/4 \n"
			" 3 [enter] 4/4 \n"
			" 4 [enter] 5/4 \n"
			" 5 [enter] 6/8 \n");

		int timeSig;
		if (scanf("%d", &timeSig) == 1) {

			switch (timeSig) {
			case 0:
				printf("none\n");
				setBeatsPerMeasure(0);
				break;
			case 1:
				printf("2/4\n");
				setBeatsPerMeasure(2);
				break;
			case 2:
				printf("3/4\n");
				setBeatsPerMeasure(3);
				break;
			case 3:
				printf("4/4\n");
				setBeatsPerMeasure(4);
				break;
			case 4:
				printf("5/4\n");
				setBeatsPerMeasure(5);
				break;
			case 5:
				printf("6/8\n");
				setBeatsPerMeasure(6);
				break;
			}

		}

	}

	if (strcmp(line, "10") == 0) {
		if (metronome_enabled) {
			DisableMetronome();
			printf("metronome disabled\n");
		}
		else {
			EnableMetronome();
			printf("metronome enabled\n");
		}

	}

	if (strcmp(line, "11") == 0) {
		printf("\Example Split Points:\n"
			" 62 = middle d [default] \n"
			" 56 = a# below middle a \n"
			" 68 = a# above middle d \n");

		printf("Enter split point: ");
		int n;
		if (scanf("%d", &n) == 1) {
			set_keyboard_splitpoint(n);
			printf("split point set to %d\n", n);
		}
	}

}

int main(int argc, char *argv[])
{

	DWORD  last_count = 0;

	/* determine what type of test to run */
	printf("Kundalini Piano Mirror version %s, written by Benjamin Pritchard\n", VersionString);
	printf("NOTE: Make sure to turn off local echo mode on your digital piano.\n");

	initialize();

	printf("no tranposition active\n");

	PrintHelp();

	last_count = GetTickCount();

	InitMetronome();

	finished = FALSE;
	while (!finished) {

		// once every 5 seconds, check to see any loaded .LUA scrips have been modified [externally, i.e. in a text editor]
		// and re-load them if so
		if (GetTickCount() > (last_count + 5000)) {
			last_count = GetTickCount();

			//printf("%d:%d\n", measure, beat);

			if (script_is_loaded)
				if (ShouldReloadFile(script_file))
				{
					printf("script modified...\n");
					ReLoadLuaScript();
				}
		}

		// (this is not portable, but that is OK because this project has become the "Windows" version of this software
		// (note that while we are doing this, we won't be able to process the metronome)
		if (kbhit()) {
			HandleKeyboard();
		}
		
	} // while (!finished)

	shutdownSystem();
	return 0;
}