#ifndef SOUND_H
#define SOUND_H

void InitSound();
void ShutdownSound();

void PlayMusic(const char* fname);
void StopMusic();
int  GetMusicVolume();
void SetMusicVolume(int volume);
int  GetMusicPosition();
void SetMusicPosition(int pos);
int  CacheSound(const char* fname);
void FreeAllSounds();
void PlaySFX(int index,int vol,int pan);

#endif
