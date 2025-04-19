#include <cstring>
#include <stdint.h>
#include <32blit.hpp>

#include "sound.hpp"

using namespace blit;

blit::Timer timer_music;

//I (joyrider3774) created the music in this tool : https://onlinesequencer.net

uint8_t prev_music, music_on, sound_on, music_pause, music_selecting;
uint16_t track[255];
uint16_t music_note, music_tempo, tracklen;
uint8_t music_loop;

constexpr uint8_t sfxSustain = 100 * 15 / 18;

const uint16_t freq = 1100;
const uint16_t freq2 = 1200;
const uint16_t freq3 = 1400;
const uint16_t freq4 = 1000;
const uint16_t pause = 16;

const uint16_t music_gameover[] ={
    freq3, pause,
    freq3, pause,
    0, pause,
    freq, pause,
    freq, pause,
    freq2, pause,
    freq2, pause,
    freq, pause,
    freq, pause,
    0,pause,
    0,pause,
    freq4, pause,
    freq4, pause,
    freq4, pause,
    freq4, pause,
    freq4, pause,
    0,1
};

const uint16_t music_crash[] ={
    freq, pause,
    freq, pause,
    freq, pause,
    freq, pause,
    0,pause,
    freq, pause,
    freq, pause,
    0,pause,
    freq, pause,
    freq, pause,
    0,pause,
    freq, pause,
    freq, pause,
    0,1
};

void playMusicTone(uint16_t tone, uint16_t sustain)
{
    if((tone == 0) || music_selecting)
        channels[1].off();
    else
    {
        channels[1].waveforms   = Waveform::SQUARE;
        channels[1].attack_ms   = 5;
        channels[1].decay_ms    = 60*sustain;
        channels[1].sustain     = 1;
        channels[1].release_ms  = 1;
        channels[1].volume      = 0xffff;
        channels[1].frequency   = tone;
        channels[1].trigger_attack();
    }
}

void playNote()
{   
    if(music_selecting)
        return;

    if(music_note < tracklen)
    {
        //Set the new delay to wait
        music_tempo = track[music_note+1];
        playMusicTone(track[music_note],music_tempo);
        //printf("%d %d\n",track[music_note], track[music_note+1]);
        //Skip to the next note
        music_note += 2;
               
        if (music_note > tracklen - 1)
        {
            if(music_loop)
            {
                music_note = 0;
            }
        }
    }
}

void pauseMusic()
{
    music_pause = 1;
    stopMusic();
}

void unpauseMusic()
{
    music_pause = 0;
}

void musicTimer(blit::Timer &t)
{
    if(music_selecting)
        return;
    if(music_pause)
        return;
    //Play some music
    if (music_tempo == 0)
    {
        if(music_on)
        {
            playNote();
        }
    }
    //Else wait for the next note to play
    else 
    {
        music_tempo--;
    }
}

void stopMusic()
{
    channels[1].off();  
}

void setMusicOn(uint8_t value)
{
    music_on = value;
    if(music_on)
    {
        if (prev_music != 0)
            SelectMusic(prev_music, 1);
    }
    else
    {
        channels[1].off();
    }
}

void setSoundOn(uint8_t value)
{
    sound_on = value;
}

uint8_t isMusicOn()
{
    return music_on;
}

uint8_t isSoundOn()
{
    return sound_on;
}

void initSound()
{
    sound_on = 0;
}

void SelectMusic(uint8_t musicFile, uint8_t force)
{
    if (((prev_music != musicFile) || force) && music_on)
    {
        music_selecting = 1;
        music_pause = 0;
        prev_music = musicFile;
        channels[1].off();
        memset(track, 0, sizeof(track));
        switch (musicFile) 
        {
            case musCrash:
                memcpy(track, music_crash, sizeof(music_crash));
                tracklen = sizeof(music_crash) / sizeof(uint16_t);
                music_loop = false;
                break;
            case musGameOver:
                memcpy(track, music_gameover, sizeof(music_gameover));
                tracklen = sizeof(music_gameover) / sizeof(uint16_t);
                music_loop = false;
                break;
        }
        music_note = 0;
        music_tempo = 0;
        music_selecting = 0;
    }
}

void playSound(uint16_t tone)
{
    if(!sound_on)
        return;
    
    channels[0].waveforms   = Waveform::SQUARE;
    channels[0].attack_ms   = 1;
    channels[0].decay_ms    = 50;
    channels[0].sustain     = 0;
    channels[0].release_ms  = 0;
    channels[0].volume      = 0xffff;
    channels[0].frequency   = tone;
    channels[0].trigger_attack();
}

void initMusic()
{
    music_on = 0;
    prev_music = 0;
    timer_music.init(musicTimer, 1, -1);
    timer_music.start();
}

void playTickSound()
{
   playSound(1100);
}
