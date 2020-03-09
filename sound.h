#ifndef SOUND_H
#define SOUND_H

#include <string>

void InitSound();
void ShutdownSound();

void PlayMusic(const std::string& fname);
void StopMusic();
int  GetMusicVolume();
void SetMusicVolume(int volume);
int  GetMusicPosition();
void SetMusicPosition(int pos);
int  CacheSound(const std::string& fname);
void FreeAllSounds();
void PlaySFX(int index, int vol, int pan);

#endif
