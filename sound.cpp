#ifndef NO_SOUND

#include "sound.h"
#include "audiere.h"

#include <vector>
#include <string>

using namespace audiere;

namespace
{
    Context* pContext;
    Stream* pMusicstream;
    std::vector<Stream*> sfx;

    std::string sCurmusic;
};

void InitSound()
{
    pContext=CreateContext(0);
}

void ShutdownSound()
{
    FreeAllSounds();
    delete pMusicstream;
    delete pContext;

    pMusicstream=0;
    pContext=0;
}

void PlayMusic(const char* fname)
{
    if (pMusicstream && !pMusicstream->isPlaying())
    {
        pMusicstream->play();
        return;
    }

    if (!stricmp(sCurmusic.c_str(),fname)) //sCurmusic==fname)
        return;

    delete pMusicstream;
    pMusicstream=pContext->openStream(fname);

    if (!pMusicstream)
        return;

    pMusicstream->setRepeat(true);
    pMusicstream->play();
}

void StopMusic()
{
    pMusicstream->pause();
}

int GetMusicVolume()
{
    return pMusicstream->getVolume();
}

void SetMusicVolume(int volume)
{
    pMusicstream->setVolume(volume);
}

int CacheSound(const char* fname)
{
    Stream* s=pContext->openStream(fname);

    if (!s)
        return -1;

    s->setRepeat(false);

    sfx.push_back(s);
    return sfx.size()-1;
}

int GetMusicPosition()
{
    return 0;//pMusicstream->getPosition();
}

void SetMusicPosition(int pos)
{
    //pMusicstream->setPosition(pos);
}

void FreeAllSounds()
{
    for (int i=0; i<sfx.size(); i++)
        delete sfx[i];

    sfx.clear();
}

void PlaySFX(int index,int vol,int pan)
{
    if (index<0 || index>=sfx.size())
        return;

    sfx[index]->setVolume(vol);

    sfx[index]->play();
}

#else

namespace
{
    int vol;
    int nSounds;
}

void InitSound(){}

void ShutdownSound(){}
void PlayMusic(const char* fname){}
void StopMusic(){}
int GetMusicVolume()
{
    return vol;
}

void SetMusicVolume(int volume)
{
    vol=volume;
}

int GetMusicPosition() { return 0; }
void SetMusicPosition(int) {}

int CacheSound(const char* fname)
{
    return nSounds++;
}

void FreeAllSounds(){ nSounds=0; }

void PlaySFX(int index,int vol,int pan){}

#endif
