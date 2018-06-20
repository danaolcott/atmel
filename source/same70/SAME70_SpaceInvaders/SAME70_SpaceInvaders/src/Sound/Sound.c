/*////////////////////////////////////////////////////
Sound Function Definitions
Sound Output from home-brew binary weighted DAC
(missing the opamp component, just the resistors)

Good Source of Example sounds:
http://www.downloadfreesound.com/8-bit-sound-effects/

Sound converter from wav to c code:
http://ccgi.cjseymour.plus.com/wavtocode/wavtocode.htm

Sounds assumed to be sampled at 44khz.  To avoid jamming up the
interrupts, run the output at 11khz, so read every 4th sample

To use:
Configure a timer to run at 11khz.  Put 
Sound_InterrruptHandler in the timer isr
Timer is running anytime the sound is on, not running
with the sound is complete

Uses Timer0 - 11khz
DAC - DACC_CHANNEL_0

*/////////////////////////////////////////////////////
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "Sound.h"
#include "dac_driver.h"
#include "timer_driver.h"			//Timer0 - 11khz

static uint8_t* waveData;
static uint32_t waveCounter;
static void Sound_PlaySound(const SoundData *sound);


//////////////////////////////////////////
//Sound is tied to timer_driver timer running
//at 11khz.   Timer1A ISR running
//at 11khz
//Sound off?
void Sound_Init(void)
{
    Timer0_Stop();
    DAC_write(DAC_Channel_0, 0x00);
}

/////////////////////////////////////////////
//Sound_InterruptHandler
//Put this function call in the timer ISR.
//Typically wave files are 44khz.  Therefore, 
//play every 4th element with timer running
//at 11khz.
//
void Sound_InterruptHandler(void)
{
	if (waveCounter > 4)
	{
        DAC_write(DAC_Channel_0, *waveData);

		waveData+=4;            //increment the pointer
		waveCounter-=4;			//decrement the down counter
	}

	else
	{
        Timer0_Stop();
        DAC_write(DAC_Channel_0, 0x00);
	}
}


////////////////////////////////////////////////
void Sound_PlaySound(const SoundData *sound)
{
	waveData = (uint8_t*)sound->pSoundData;		//set the pointer
	waveCounter = sound->length;

    //start the timer that calls the sound isr
    Timer0_Start();
}



void Sound_Play_PlayerFire(void)
{
	Sound_PlaySound(&sound_shootPlayer);
}
void Sound_Play_EnemyFire(void)
{
	Sound_PlaySound(&sound_shootEnemy);
}

void Sound_Play_PlayerExplode(void)
{
	Sound_PlaySound(&sound_explodePlayer);
}

void Sound_Play_EnemyExplode(void)
{
	Sound_PlaySound(&sound_explodePlayer);
}


void Sound_Play_GameOver(void)
{
	Sound_PlaySound(&sound_gameover);
}

void Sound_Play_LevelUp(void)
{
	Sound_PlaySound(&sound_levelup);
}


