
// Adapted from here:
//		https://www.codeproject.com/Articles/27183/Playing-Wave-Resources
//
// Original code was in C++, I converted back into standard C [so I could use it in this project]
// (make sure to have an embedded .wave file as a resource in your project)
//
// (The point of this code is that the Windows32 API function sndPlaySound() is too slow)
//
// Usage:
//		CRscWaveOut_INIT()
//		LoadWaveResource("TIC")				// pass in the name of your embedded wave resource
//		PlayWave();
//		CRscWaveOut_Destroy()


#include <windows.h>
#include <mmsystem.h>
#include <mmiscapi.h>
#include <stdio.h>

#include "wave.h"

void ASSERT(BOOL val) {
	if (!val) {
		printf("assertion failed!!");
	}
}

// constructor / destructor changed to just regular functions...

CRscWaveOut_INIT()
{
	// default constructor
	// this class must be initialized by LoadWaveResource
	m_WaveOutHandle = 0;
}

CRscWaveOut_Destroy()
{
	if (m_WaveOutHandle) {
		waveOutReset(m_WaveOutHandle);			

		MMRESULT  mmresult = waveOutClose(m_WaveOutHandle);
		ASSERT(mmresult == MMSYSERR_NOERROR);

		for (UINT i = 0; i < m_uiWaveCount; i++) free(m_arrWaveHeader[i].lpData);
		//free(m_arrWaveHeader);
	}
}

BOOL LoadWaveResource(LPCSTR resourceName, HINSTANCE Nl)
{

	if (m_WaveOutHandle) return FALSE;

	m_Nl = Nl;
	m_uiWaveCount = 1;

	//Initialize wave out device. Sample Waves are all  22050hz, 8bps 
	WAVEFORMATEX  waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 1;
	waveFormat.nSamplesPerSec = 22050;
	waveFormat.wBitsPerSample = 8;
	waveFormat.nBlockAlign = 1; // waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
	waveFormat.nAvgBytesPerSec = 44100; // waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	//MMRESULT  mmresult = waveOutOpen(&m_WaveOutHandle, WAVE_MAPPER, &waveFormat, (DWORD)hWnd, 0, CALLBACK_WINDOW);
	MMRESULT mmresult = waveOutOpen(&m_WaveOutHandle, WAVE_MAPPER, &waveFormat, (DWORD)(VOID*)waveInProc, 0, CALLBACK_FUNCTION);
	ASSERT(mmresult == MMSYSERR_NOERROR);
	if (mmresult != MMSYSERR_NOERROR) {
		m_WaveOutHandle = 0;
		return FALSE;
	}

	// load wave sample into memory
	m_arrWaveHeader = malloc(m_uiWaveCount); // new WAVEHDR[m_uiWaveCount];
	BOOL bSuc0 = BuildWAVEHDR(resourceName, &m_arrWaveHeader[0]);
	ASSERT(bSuc0);
	//BOOL bSuc1 = BuildWAVEHDR(IDR_WAVE_SNARE_HIGH, &m_arrWaveHeader[1]); ASSERT( bSuc1 );
	//BOOL bSuc2 = BuildWAVEHDR(IDR_WAVE_KICK_HIGH, &m_arrWaveHeader[2]);ASSERT( bSuc2 );
	//if ( !bSuc0 && !bSuc0 && !bSuc0 ) {
	if (!bSuc0) {
		//TRACE("BuildWAVEHDR failed");

		MMRESULT  mmresult = waveOutClose(m_WaveOutHandle);
		ASSERT(mmresult == MMSYSERR_NOERROR);
		m_WaveOutHandle = 0;

		for (UINT i = 0; i < m_uiWaveCount; i++) free(m_arrWaveHeader[i].lpData);
		//free(m_arrWaveHeader);
		return FALSE;
	}

	return TRUE;
}



// greatly helped by http://speech.korea.ac.kr/~kaizer/edu/Lowlevelaudio.htm
BOOL BuildWAVEHDR(LPCSTR resourceName, WAVEHDR *pWaveHeader)
{
	//	HINSTANCE Nl= _Module.GetModuleInstance();  // ATL way
	//HRSRC hResInfo = FindResource(m_Nl, MAKEINTRESOURCE(uiIDofWaveRes), "WAVE");
	HRSRC hResInfo = FindResource(m_Nl, resourceName, "WAVE");
	HANDLE hWaveRes = LoadResource(m_Nl, hResInfo);
	ASSERT(hWaveRes);
	LPSTR lpWaveRes = (LPSTR)LockResource(hWaveRes);
	ASSERT(lpWaveRes);
	DWORD ressize = SizeofResource(m_Nl, hResInfo);

	//1) mem file open ; multimedia memory file ; http://msdn.microsoft.com/en-us/library/ms712837(VS.85).aspx
	HMMIO hmmio = 0;
	MMIOINFO mmioinfo;
	mmioinfo.pIOProc = NULL;
	mmioinfo.fccIOProc = FOURCC_MEM;
	mmioinfo.pchBuffer = lpWaveRes;
	mmioinfo.cchBuffer = ressize;
	mmioinfo.adwInfo[0] = (DWORD)NULL;
	mmioinfo.adwInfo[1] = (DWORD)NULL;
	mmioinfo.adwInfo[2] = (DWORD)NULL;
	mmioinfo.dwFlags = 0;	// from now on etc
	mmioinfo.wErrorRet = 0;
	mmioinfo.htask = 0;
	mmioinfo.pchNext = 0;
	mmioinfo.pchEndRead = 0;
	mmioinfo.pchEndWrite = 0;
	mmioinfo.lBufOffset = 0;
	mmioinfo.lDiskOffset = 0;
	mmioinfo.dwReserved1 = 0;
	mmioinfo.dwReserved2 = 0;
	mmioinfo.hmmio = 0;
	hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);
	ASSERT(hmmio);
	if (hmmio == 0) return FALSE;
	//  test with a real file on disc
	//	HMMIO hmmio = mmioOpen((LPSTR)"c:\\keyclick_mid.wav", 
	//	NULL ,                   // MMCKINFO
	//	MMIO_READ);        

	//2) FIND PARENT CHUNK
	MMRESULT  rc;
	MMCKINFO  MMCkInfoParent;
	MMCkInfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	rc = mmioDescend(hmmio, &MMCkInfoParent, NULL, MMIO_FINDRIFF);
	ASSERT(rc == MMSYSERR_NOERROR);
	if (rc != MMSYSERR_NOERROR) return FALSE;

	//3) FIND CHILD CHUNK
	MMCKINFO  MMCkInfoChild;
	MMCkInfoChild.ckid = mmioFOURCC('f', 'm', 't', ' ');
	rc = mmioDescend(hmmio, &MMCkInfoChild, &MMCkInfoParent, MMIO_FINDCHUNK);
	ASSERT(rc == MMSYSERR_NOERROR);
	if (rc != MMSYSERR_NOERROR) return FALSE;

	//4) READ WAVE FILE FORMAT
	PCMWAVEFORMAT  WaveRecord;
	LONG lByteReadFormat = mmioRead(hmmio, (LPSTR)&WaveRecord, MMCkInfoChild.cksize);
	if (lByteReadFormat == 0) return FALSE;
	//Ex) 22kHz , 8 bps , STEREO
	//WaveRecord.wf.wFormatTag  = WAVE_FORMAT_PCM;	
	//WaveRecord.wf.nChannels    = 1;    //1: mono   2 :stereo  
	//WaveRecord.wf.nSamplesPerSec = 22050;     // or 44100
	//WaveRecord.wBitsPerSample = 8;   //  8
	//WaveRecord.wf.nBlockAlign  = 1;    //  wBitsPerSample / 8	 = waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
	//WaveRecord.wf.nAvgBytesPerSec =  44100; //22050 * WaveRecord.wf.nChannels * WaveRecord.wf.nBlockAlign;				

	//5) MOVE TO PARENT CHUNK
	rc = mmioAscend(hmmio, &MMCkInfoChild, 0);
	ASSERT(rc == MMSYSERR_NOERROR);
	if (rc != MMSYSERR_NOERROR) return FALSE;

	//6) FIND DATA CHUNK
	MMCkInfoChild.ckid = mmioFOURCC('d', 'a', 't', 'a');
	rc = mmioDescend(hmmio, &MMCkInfoChild, &MMCkInfoParent, MMIO_FINDCHUNK);
	ASSERT(rc == MMSYSERR_NOERROR);
	if (rc != MMSYSERR_NOERROR) return FALSE;

	//7) GET DATA	
	DWORD  lDatasize = MMCkInfoChild.cksize;
	LPSTR  pWaveDataBlock;
	pWaveDataBlock = malloc(lDatasize); // new char[lDatasize];  // should be 'delete' ed later. for example in destructor function
	LONG lByteRead = mmioRead(hmmio, pWaveDataBlock, lDatasize);
	ASSERT(lByteRead == (long)lDatasize);
	//if ( lByteRead != lDatasize ) return FALSE;

	//8) build WAVEHDR 
	pWaveHeader->lpData = pWaveDataBlock;  // the DATA we finally got
	pWaveHeader->dwBufferLength = lDatasize;  // data size
	pWaveHeader->dwFlags = 0L;    // start position
	pWaveHeader->dwLoops = 0L;   // loop
	pWaveHeader->dwBytesRecorded = lDatasize;

	//9) 
	mmioClose(hmmio, 0);

	return TRUE;
}
void PlayWave()
{
	if (!m_WaveOutHandle) return;

	WAVEHDR * pWaveHeader = &m_arrWaveHeader[0];

	// play the sound     	
	waveOutPrepareHeader(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));
	waveOutWrite(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));

}

/*
void PlayWave(UINT uiWaveHeaderIndexToPlay)
{
	if (uiWaveHeaderIndexToPlay >= m_uiWaveCount) return;
	if (!m_WaveOutHandle) return;

	WAVEHDR * pWaveHeader = &m_arrWaveHeader[uiWaveHeaderIndexToPlay];

	// play the sound     	
	waveOutPrepareHeader(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));
	waveOutWrite(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));

}
*/

// use this if you specified CALLBACK_WINDOW in waveOutOpen(...);
// we need to chain the function to MM_WOM_DONE message handler of a Window class 
// you can create a simple invisible window class or just add handler to main window for that use
// then specify hWnd in Initialize(..., hWnd) correctly
BOOL OnWaveOutDone(WPARAM wParam, LPARAM lParam)
{
	ASSERT((UINT)wParam == (UINT)m_WaveOutHandle);
	//(LPWAVEHDR)lParam must be one of m_arrWaveHeader[3]
	MMRESULT  mmresult = waveOutUnprepareHeader((HWAVEOUT)wParam, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
	ASSERT(mmresult == MMSYSERR_NOERROR);

	return TRUE;
}

// MM_WOM_DONE handling callback 
static void CALLBACK  waveInProc(HWAVEOUT WaveOutHandle, UINT uMsg, long dwInstance, DWORD dwParam1, DWORD dwParam2)
{

	switch (uMsg)
	{
	case MM_WOM_DONE:
	{
		// same as OnWaveOutDone(dwParam1, dwParam2);
		//ASSERT( (UINT)wParam == (UINT)m_WaveOutHandle );
		//(LPWAVEHDR)lParam must be one of m_arrWaveHeader[3]
		MMRESULT  mmresult = waveOutUnprepareHeader((HWAVEOUT)WaveOutHandle, (LPWAVEHDR)dwParam1, sizeof(WAVEHDR));
		ASSERT(mmresult == MMSYSERR_NOERROR);

		break;
	}

	case WIM_DATA:
	{
		break;
	}
	} // end of switch
}