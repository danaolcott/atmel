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
with the sound is complete.  Play sound function has
argument for no overwrite.  This sets a flag to 
prevent sounds from overwritting the current sound
until that sound is done playing.  Useful for playing
longer sound arrays.

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
static uint8_t mSoundFlag = 0x00;	//flag to prevent sound overwrites

static void Sound_PlaySound(const SoundData *sound, uint8_t preventOverwrite);


//////////////////////////////////////////
//Sound is tied to timer_driver timer running
//at 11khz.   Timer1A ISR running
//at 11khz
//Sound off?
void Sound_Init(void)
{
    Timer0_Stop();
	mSoundFlag = 0x00;
    DAC_write(DAC_Channel_0, 0x00);
}

/////////////////////////////////////////////
//Sound_InterruptHandler
//Put this function call in the timer ISR.
//Typically wave files are 44khz.  Therefore, 
//play every 4th element with timer running
//at 11khz.
//NOTE:
//wave data file is scaled by 8 to boost up the
//dac output.  shifting up 4 makes it lower than
//shifting up by 3 as shown on scope output.  
//
void Sound_InterruptHandler(void)
{
	if (waveCounter > 4)
	{
		uint16_t output = (uint16_t)(*waveData) & 0xFF;
        DAC_write(DACC_CHANNEL_0, (output << 3));

		waveData+=4;            //increment the pointer
		waveCounter-=4;			//decrement the down counter
	}

	else
	{
		mSoundFlag = 0x00;
        Timer0_Stop();
        DAC_write(DACC_CHANNEL_0, 0x00);
	}
}


////////////////////////////////////////////////
//Play sound
//set the sound pointer to the appropriate array
//and reset the sound counter.  prevent overwrite
//sets a sound flag to prevent a new sound from
//playing over another sound until that sound it
//complete.  Using prevent overwrite is the only
//way to set the sound flag.  Otherwise, sounds
//always overwrite each other   
void Sound_PlaySound(const SoundData *sound, uint8_t preventOverwrite)
{
	//prevent overwrite and currently no sound flag is set
	//ie, not overwriting something that had the no overwrite set
	if ((preventOverwrite == 1) && (!mSoundFlag))
	{
		mSoundFlag = 1;
		waveData = (uint8_t*)sound->pSoundData;		//set the pointer
		waveCounter = sound->length;				//set the counter

		//start 11khz timer to call the sound handler
		Timer0_Start();
	}

	else if(!mSoundFlag)
	{
		waveData = (uint8_t*)sound->pSoundData;		//set the pointer
		waveCounter = sound->length;				//set the counter

		//start 11khz timer to call the sound handler
		Timer0_Start();
	}
}



void Sound_Play_PlayerFire(void)
{
	Sound_PlaySound(&sound_shootPlayer, 0);
}
void Sound_Play_EnemyFire(void)
{
	Sound_PlaySound(&sound_shootEnemy, 0);
}

void Sound_Play_PlayerExplode(void)
{
	Sound_PlaySound(&sound_explodePlayer, 1);
}

void Sound_Play_EnemyExplode(void)
{
	Sound_PlaySound(&sound_explodePlayer, 0);
}

void Sound_Play_GameOver(void)
{
	Sound_PlaySound(&sound_gameover, 1);
}

void Sound_Play_LevelUp(void)
{
	Sound_PlaySound(&sound_levelup, 1);
}


