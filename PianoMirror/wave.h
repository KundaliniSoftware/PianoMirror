#include <Windows.h>
#include <Mmsystem.h>


CRscWaveOut_INIT();
CRscWaveOut_Destroy();

BOOL LoadWaveResource(LPCSTR resourceName, HINSTANCE Nl);
void PlayWave();
/*void PlayWave(UINT uiWaveResourceID);*/
BOOL OnWaveOutDone(WPARAM wParam, LPARAM lParam);
BOOL BuildWAVEHDR(LPCSTR resourceName/*in*/, WAVEHDR *pWaveHeader/*out*/);
static void CALLBACK waveInProc(HWAVEOUT WaveOutHandle, UINT uMsg, long dwInstance, DWORD dwParam1, DWORD dwParam2);

UINT m_uiWaveCount;
HINSTANCE m_Nl;
HWAVEOUT	m_WaveOutHandle;
WAVEHDR	* m_arrWaveHeader;
