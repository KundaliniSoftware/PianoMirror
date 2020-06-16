
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
//		LoadWaveResources("wave1", "wave2", GetModuleHandle(NULL));
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

CRscWaveOut_INIT()
{
	m_WaveOutHandle = 0;
	m_uiWaveCount = 0;
}

// only call this when you are completely done 
CRscWaveOut_Destroy()
{
	if (m_WaveOutHandle) {
		waveOutReset(m_WaveOutHandle);			

		MMRESULT  mmresult = waveOutClose(m_WaveOutHandle);
		ASSERT(mmresult == MMSYSERR_NOERROR);

		for (UINT i = 0; i < m_uiWaveCount; i++) free(m_arrWaveHeader[i].lpData);
		free(m_arrWaveHeader);
	}
}

// load two wave resources, which must be linked in with a resource file
// for example: 
//			w1               WAVE                    "wav\\1.wav"
//			w2               WAVE                    "wav\\2.wav"

BOOL LoadWaveResources(LPCSTR resourceName1, LPCSTR resourceName2, HINSTANCE Nl)
{

	if (m_WaveOutHandle) return FALSE;

	m_Nl = Nl;

	//Initialize wave out device. Sample Waves are all  22050hz, 8bps 
	WAVEFORMATEX  waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 1;
	waveFormat.nSamplesPerSec = 22050;
	waveFormat.wBitsPerSample = 8;
	waveFormat.nBlockAlign = 1; // waveFormat.nChannels * (waveFormat.wBitsPerSample/8);
	waveFormat.nAvgBytesPerSec = 44100; // waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	MMRESULT mmresult = waveOutOpen(&m_WaveOutHandle, WAVE_MAPPER, &waveFormat, (DWORD)(VOID*)waveInProc, 0, CALLBACK_FUNCTION);
	ASSERT(mmresult == MMSYSERR_NOERROR);
	if (mmresult != MMSYSERR_NOERROR) {
		m_WaveOutHandle = 0;
		return FALSE;
	}

	// load wave sample into memory
	m_arrWaveHeader = malloc(sizeof(m_arrWaveHeader[0]) * 2);			// allocate enough memory for two wave headers 
	 
	BOOL bSuc0 = BuildWAVEHDR(resourceName1, &m_arrWaveHeader[0]);
	ASSERT(bSuc0);
	BOOL bSuc1 = BuildWAVEHDR(resourceName2, &m_arrWaveHeader[1]);
	ASSERT( bSuc1 );

	m_uiWaveCount = 2;
	
	if ( !bSuc0 && !bSuc1) {
		//TRACE("BuildWAVEHDR failed");

		MMRESULT  mmresult = waveOutClose(m_WaveOutHandle);
		ASSERT(mmresult == MMSYSERR_NOERROR);
		m_WaveOutHandle = 0;

		for (UINT i = 0; i < m_uiWaveCount; i++) free(m_arrWaveHeader[i].lpData);
		free(m_arrWaveHeader);
		return FALSE;
	}

	return TRUE;
}



// greatly helped by http://speech.korea.ac.kr/~kaizer/edu/Lowlevelaudio.htm
BOOL BuildWAVEHDR(LPCSTR resourceName, WAVEHDR *pWaveHeader)
{
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


void PlayWave(UINT uiWaveHeaderIndexToPlay)
{
	if (uiWaveHeaderIndexToPlay >= m_uiWaveCount) return;
	if (!m_WaveOutHandle) return;

	WAVEHDR * pWaveHeader = &m_arrWaveHeader[uiWaveHeaderIndexToPlay];

	// play the sound     	
	waveOutPrepareHeader(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));
	waveOutWrite(m_WaveOutHandle, pWaveHeader, sizeof(*pWaveHeader));

}


// MM_WOM_DONE handling callback 
static void CALLBACK  waveInProc(HWAVEOUT WaveOutHandle, UINT uMsg, long dwInstance, DWORD dwParam1, DWORD dwParam2)
{

	switch (uMsg)
	{
		case MM_WOM_DONE:
		{
			MMRESULT  mmresult = waveOutUnprepareHeader((HWAVEOUT)WaveOutHandle, (LPWAVEHDR)dwParam1, sizeof(WAVEHDR));
			ASSERT(mmresult == MMSYSERR_NOERROR);

			break;
		}

		case WIM_DATA:
		{
			break;
		}
	}
}