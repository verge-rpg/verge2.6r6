#ifndef NO_SOUND

#include "sound.h"
#include "log.h"
#include "misc.h"
#include "audiere.h"

#include <vector>
#include <string>

using namespace audiere;

namespace
{
    RefPtr<AudioDevice> device;
    RefPtr<OutputStream> musicStream;
    std::vector< RefPtr<OutputStream> > sfx;

    std::string curMusic;
};

void InitSound()
{
    device = OpenDevice();
    if (!device)
        Log::Write("audiere::OpenDevice failed.");
}

void ShutdownSound()
{
    sfx.clear();
    musicStream = 0;
    device = 0;
}

void PlayMusic(const std::string& fname)
{
    if (musicStream && !musicStream->isPlaying())
    {
        musicStream->play();
        return;
    }

    if (curMusic == fname)
        return;

    curMusic = fname;

    musicStream = OpenSound(device.get(), fname.c_str(), true);

    if (!musicStream)
    {
        Log::Write("audiere::OpenSound failed.");
        return;
    }

    musicStream->setRepeat(true);
    musicStream->play();
}

void StopMusic()
{
    if (musicStream)
        musicStream->stop();
}

int GetMusicVolume()
{
    return musicStream
        ? (int)(musicStream->getVolume() * 255)
        : 0;
}

void SetMusicVolume(int volume)
{
    if (musicStream)
        musicStream->setVolume(1.0f * volume / 255);
}

int CacheSound(const std::string& fname)
{
    RefPtr<OutputStream> s = OpenSound(device.get(), fname.c_str(), false);

    if (!s)
    {
        Log::Write("audiere::OpenSound failed");
        return -1;
    }

    sfx.push_back(s);
    return sfx.size() - 1;
}

int GetMusicPosition()
{
    return musicStream
        ? musicStream->getPosition()
        : 0;
}

void SetMusicPosition(int pos)
{
    if (musicStream)
        musicStream->setPosition(pos);
}

void FreeAllSounds()
{
    sfx.clear();
}

void PlaySFX(int index, int vol, int pan)
{
    if (index < 0 || index >= sfx.size())
        return;

    if (pan < 0) pan = 0;
    if (pan > 255) pan = 255;

    sfx[index]->setVolume(1.0f * vol / 255);
    sfx[index]->setPan((2.0f * pan / 255) - 1.0f);    // -1.0 to 0 to +1.0
    sfx[index]->play();
}

#else // defined(NO_SOUND)

#include <string>

// Audio stub follows.  #define NO_SOUND to disable all audio.
namespace
{
    int vol;
    int nSounds;
}

void InitSound(){}

void ShutdownSound(){}
void PlayMusic(const std::string& fname){}
void StopMusic(){}
int GetMusicVolume()
{
    return vol;
}

void SetMusicVolume(int volume)
{
    vol = volume;
}

int GetMusicPosition() { return 0; }
void SetMusicPosition(int) {}

int CacheSound(const std::string& fname)
{
    return nSounds++;
}

void FreeAllSounds(){ nSounds = 0; }

void PlaySFX(int index, int vol, int pan){}

#endif
