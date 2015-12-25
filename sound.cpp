#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <D3dx9core.h>
#include <dinput.h>
#include <stdio.h>
#include <mmsystem.h>
#include <dsound.h>
#include <xaudio2.h>
#include "sound.h"

SOUND::SOUND()
{
	xaudio = NULL;
	pSourceVoice = NULL;
	wavdata = NULL;
}

SOUND::SOUND(IXAudio2 *xa)
{
	xaudio = xa;
}


SOUND::~SOUND()
{
	//pSourceVoice->DestroyVoice();
    free(wavdata);
}

unsigned long SOUND::playMIDIFile(HWND hWndNotify, wchar_t* MIDIFileName)
{
	UINT wDeviceID;
    DWORD dwReturn;
    MCI_OPEN_PARMS mciOpenParms;
    MCI_PLAY_PARMS mciPlayParms;
    MCI_STATUS_PARMS mciStatusParms;
    //MCI_SEQ_SET_PARMS mciSeqSetParms;
	wchar_t errortxt[129];
	
    // Open the device by specifying the device and filename.
    // MCI will attempt to choose the MIDI mapper as the output port.
    mciOpenParms.lpstrDeviceType = L"sequencer";
    mciOpenParms.lpstrElementName = MIDIFileName;
    if (dwReturn = mciSendCommand(NULL, MCI_OPEN,
        MCI_OPEN_TYPE | MCI_OPEN_ELEMENT,
        (DWORD)(LPVOID) &mciOpenParms))
    {
        // Failed to open device. Don't close it; just return error.
        return (dwReturn);
    }

    // The device opened successfully; get the device ID.
    wDeviceID = mciOpenParms.wDeviceID;

    // Check if the output port is the MIDI mapper.
    mciStatusParms.dwItem = MCI_SEQ_STATUS_PORT;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_STATUS, 
        MCI_STATUS_ITEM, (DWORD)(LPVOID) &mciStatusParms))
    {
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
        return (dwReturn);
    }

    // The output port is not the MIDI mapper. 
    // Ask if the user wants to continue.
    if ((short)LOWORD(mciStatusParms.dwReturn) != MIDI_MAPPER)
    {
		swprintf(errortxt, L"The MIDI mapper is not available. Continue? Error %hd", (short)LOWORD(mciStatusParms.dwReturn));
        if (MessageBox(NULL,
            errortxt,
            L"", MB_YESNO) == IDNO)
        {
            // User does not want to continue. Not an error;
            // just close the device and return.
            mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
            return 0;
        }
    }

    // Begin playback. The window procedure function for the parent 
    // window will be notified with an MM_MCINOTIFY message when 
    // playback is complete. At this time, the window procedure closes 
    // the device.
    mciPlayParms.dwCallback = (DWORD) hWndNotify;
    if (dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_NOTIFY, 
        (DWORD)(LPVOID) &mciPlayParms))
    {
        mciSendCommand(wDeviceID, MCI_CLOSE, 0, NULL);
        return (dwReturn);
    }

    return 0;
}

int SOUND::loadWAVFile(wchar_t* fileName)
{
	HRESULT hr;
	FILE* fp;
	WAVFILEHEADER wfh;
	wchar_t errormsg[256];
	//DSBUFFERDESC dsbufdesc;
	//dsbufdesc.dwBufferBytes;
	//dsound->CreateSoundBuffer(&dsbufdesc, &soundBuffer, NULL);

	fp = _wfopen(fileName, L"rb");
	fread(&wfh, sizeof(WAVFILEHEADER), 1, fp);
	wavdata = (unsigned char*)malloc(wfh.subchunkSize);
	fread((unsigned char*)wavdata, wfh.subchunkSize, 1, fp);
	fclose(fp);

	WAVEFORMATEX wfx;

	wfx.cbSize = wfh.cksize;
	wfx.nAvgBytesPerSec = wfh.nAvgBytesPerSec;
	wfx.nBlockAlign = wfh.nBlockAlign;
	wfx.nChannels = wfh.nChannels;
	wfx.nSamplesPerSec = wfh.nSamplesPerSec;
	wfx.wBitsPerSample = wfh.wBitsPerSample;
	wfx.wFormatTag = wfh.wFormatTag;

	
    if( FAILED( hr = xaudio->CreateSourceVoice( &pSourceVoice, &wfx ) ) )
    {
        swprintf(errormsg, L"Error %#X creating source voice\n", hr );
		MessageBoxW(NULL, errormsg, L"Error", MB_OK);
        free(wavdata);
        return hr;
    }

	// Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = wavdata;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = wfh.subchunkSize;

    if( FAILED( hr = pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
    {
        swprintf(errormsg, L"Error %#X submitting source buffer\n", hr );
		MessageBoxW(NULL, errormsg, L"Error", MB_OK);
        pSourceVoice->DestroyVoice();
        free(wavdata);
        return hr;
    }

   

    // Wait till the escape key is released
    //while( GetAsyncKeyState( VK_ESCAPE ) )
        //Sleep( 10 );

	return 1;
}

int SOUND::startWAVFile(void)
{
	HRESULT hr;
	 hr = pSourceVoice->Start(0, XAUDIO2_COMMIT_NOW) ;
	 isRunning = TRUE;
	 return 1;
}

int SOUND::playWAVFile(void)
{
	// Let the sound play
   
    //while( isRunning )
    //{
	if(isRunning)
	{
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState( &state );
        isRunning = ( state.BuffersQueued > 0 ) != 0;
	}
	else
	{
		pSourceVoice->Stop();
		return 0;
	}
    return 1;
}