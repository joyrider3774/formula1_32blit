#include <stdlib.h>
#include <string.h>
#include <32blit.hpp>
#if defined(TARGET_32BLIT_HW) || defined(PICO_BUILD)
#include <malloc.h>
#endif
#include "sound.hpp"
#include "assets.hpp"

using namespace blit;

const int WINDOW_WIDTH = 240;
const int WINDOW_HEIGHT = 240;

enum GameStates {GSQuit,GSIntro,GSGame,GSGameOver};

const int FPS = 30;
Surface *Background,*Player,*Enemy;
GameStates GameState = GSIntro;
uint32_t NextTime;
bool EnemyStates[3][3];
bool PlayerStates[3];
int HitPosition, LivesLost;
long Score = 0;
int FlashesDelay = 0;
int Flashes = 0;
int Delay = 0;
bool CanMove = true;
bool CrashSoundPlayed = false;
float scale = 1.0f;
int xoffset = 0;
int yoffset = 0;
bool debugMode = false;
int GameFrames = 0;
const Font font(lcdfont);

struct SaveData {
	long HiScore = 0;
};

SaveData saveData;

uint32_t WaitForFrame()
{
	uint32_t Result;
	uint32_t CurrentTime = now();
	if (CurrentTime >= NextTime)
	{
		Result = 0;
		NextTime = CurrentTime + (uint32_t)(1000.0f/FPS);
	}
	else
		Result = NextTime - CurrentTime;
	return Result;
}

void LoadSettings()
{
	if(!read_save(saveData))
		saveData.HiScore = 0;
}

// Save the settings
void SaveSettings()
{
	write_save(saveData);
}

void setHiScore(long value)
{
	if(value > saveData.HiScore)
	{
    	saveData.HiScore = value;
	}
}

void DrawScoreBar(bool Empty, long ScoreIn, long HighScoreIn, int LivesLostIn)
{
	if(Empty)
		return;

	char ScoreStr[20];
	char Str[20];
	sprintf(ScoreStr,"%ld", ScoreIn);
	Rect DstRect;
	if (strlen(ScoreStr) > 0)
	{
		DstRect.x = xoffset + (200 - 100)*scale;
		DstRect.y = yoffset+ 30*scale;
		DstRect.w = 100*scale;
		DstRect.h = 15;
		screen.pen = Pen(0, 0, 0);
        screen.text(ScoreStr, font, DstRect, true, top_right);
	}
	sprintf(ScoreStr,"%ld", HighScoreIn);
	if (strlen(ScoreStr) > 0)
	{
		DstRect.x = xoffset + (140 - 100)*scale;
		DstRect.y = yoffset + 30*scale;
		DstRect.w = 100*scale;
		DstRect.h = 15;
		screen.pen = Pen(0, 0, 0);
        screen.text(ScoreStr, font, DstRect, true, top_right);		
	}
	if (LivesLostIn >=1)
	{
		for(int X = 0;X<LivesLostIn;X++)
			Str[X] = 'X';
		Str[LivesLostIn] = '\0';
		DstRect.x = xoffset + 50*scale;
		DstRect.y = yoffset + 30*scale;
		DstRect.w = 100*scale;
		DstRect.h = 15;
		screen.pen = Pen(0, 0, 0);
        screen.text(Str, font, DstRect, true, top_left);
	}
}

void DrawGame()
{
	int X,Y;
	Rect DstRect;
	for (X=0;X<3;X++)
		for (Y=0;Y<3;Y++)
			if (EnemyStates[X][Y])
			{
				DstRect.x = (50 + (X * 54))*scale + xoffset;
				DstRect.y = (48 + (Y * 47))*scale + yoffset;
				DstRect.w = (Enemy->bounds.w)*scale;
				DstRect.h = (Enemy->bounds.h)*scale;
				screen.stretch_blit(Enemy,Rect(0,0,Enemy->bounds.w, Enemy->bounds.h),DstRect);
			}

	for (X=0;X<3;X++)
	{
		if (PlayerStates[X])
		{
			DstRect.x = (50 + (X * 54))*scale + xoffset;			
			DstRect.y = 180*scale + yoffset;
			DstRect.w = Player->bounds.w;
			DstRect.h = Player->bounds.h;
			screen.stretch_blit(Player,Rect(0,0,Player->bounds.w, Player->bounds.h),DstRect);
		}
	}
}

void MoveEnemy()
{
	for (int X=0;X<3;X++)
		for (int Y=2;Y>=1;Y--)
			EnemyStates[X][Y] = EnemyStates[X][Y-1];
	for (int X=0;X<3;X++)
		EnemyStates[X][0] = false;
	for (int X=0;X<=1;X++)
		EnemyStates[rand()%3][0] = true;
}

void MoveLeft()
{
	for (int X=0;X<2;X++)
	{
		if (PlayerStates[X+1])
		{
			PlayerStates[X] = true;
			PlayerStates[X+1] = false;
		}
	}
}

void MoveRight()
{
	for (int X=2;X>=1;X--)
	{
		if (PlayerStates[X-1])
		{
			PlayerStates[X] = true;
			PlayerStates[X-1] = false;
		}
	}
}

bool IsCollided()
{
	bool Temp;
	Temp=false;
	for (int X = 0;X<3;X++)
		if (PlayerStates[X] && EnemyStates[X][2])
		{
			Temp = true;
			HitPosition = X;
		}
	return Temp;
}

void InitialiseStates()
{
	for(int X = 0;X<3;X++)
		for(int Y=0;Y<3;Y++)
			EnemyStates[X][Y] = false;
	for(int X=0;X<3;X++)
		PlayerStates[X] = false;
	PlayerStates[1] = true;
}

void Game_init()
{
	GameFrames = 25;
	FlashesDelay = 29;
	Flashes = 0;
	Delay = 120;
	Score = 0;
	LivesLost = 0;
	InitialiseStates();
}

void Game_render()
{
	screen.stretch_blit(Background, 
		Rect(0,0,Background->bounds.w, Background->bounds.h), 
		Rect(xoffset,yoffset, Background->bounds.w*scale, Background->bounds.h*scale));
	DrawGame();
	DrawScoreBar(false, Score, saveData.HiScore, LivesLost);
}


void Game_update()
{
	if (buttons.pressed & Button::DPAD_LEFT)
		if(CanMove)
			MoveLeft();

	if ((buttons.pressed & Button::DPAD_RIGHT) || (buttons.pressed & Button::A))
		if(CanMove)
			MoveRight();
			
	GameFrames++;
	if (GameFrames >= Delay)
	{
		if (!IsCollided() && CanMove)
		{			
			GameFrames = 0;
			for (int X = 0;X < 3;X++)
				if (EnemyStates[X][2])
				{
					Score += 10;
					setHiScore(Score);
					if ((Score % 100 ==0) && (Delay > 18))
						Delay--;
				}
			MoveEnemy();
			playTickSound();
		}
		else
		{
			if (!CrashSoundPlayed)
			{
				SelectMusic(musCrash, true);
				CrashSoundPlayed = true;
			}
			CanMove = false;
			FlashesDelay++;
			if (FlashesDelay == 30)
			{
				Flashes++;
				PlayerStates[HitPosition] = !PlayerStates[HitPosition];
				EnemyStates[HitPosition][2] = !EnemyStates[HitPosition][2];
				FlashesDelay = 0;
				if (Flashes == 6)
				{
					Flashes = 0;
					CanMove = true;
					GameFrames = 0;
					CrashSoundPlayed=false;
					EnemyStates[HitPosition][2] = false;
					LivesLost++;
					FlashesDelay = 29;
					if (LivesLost == 3)	
					{
						GameFrames = 0;
						GameState = GSGameOver;
						SaveSettings();
					}
				}
			}
		}
	}
}

void GameOver_update()
{

	GameFrames++;
	if(GameFrames < 75)
		return;
	if(GameFrames == 75)
	{
		SelectMusic(musGameOver, true);
	}
	if(GameFrames > 300)
		if((buttons.pressed & Button::A) || (buttons.pressed & Button::B) || (buttons.pressed & Button::X) || 
			(buttons.pressed & Button::Y) || (buttons.pressed & Button::DPAD_LEFT) || (buttons.pressed & Button::DPAD_RIGHT))
		{
			Game_init();
			GameState = GSGame;
		}
}

void GameOver_render()
{
	screen.stretch_blit(Background, 
		Rect(0,0,Background->bounds.w, Background->bounds.h), 
		Rect(xoffset,yoffset, Background->bounds.w*scale, Background->bounds.h*scale));
	DrawGame();
	DrawScoreBar(false, Score, saveData.HiScore, LivesLost);
}

void Intro_update()
{
	FlashesDelay++;
	if (FlashesDelay == 50)
	{
		FlashesDelay = 0;
		for (int X = 0;X < 3;X++)
			 for (int Y = 0;Y<3;Y++)
				 EnemyStates[X][Y] = ! EnemyStates[X][Y];
		 for (int X=0;X<3;X++)
			 PlayerStates[X] = ! PlayerStates[X];	
	}
	if((buttons.pressed & Button::A) || (buttons.pressed & Button::B) || (buttons.pressed & Button::X) || 
		(buttons.pressed & Button::Y) || (buttons.pressed & Button::DPAD_LEFT) || (buttons.pressed & Button::DPAD_RIGHT))
	{
		Game_init();
		GameState = GSGame;
	}
}

void Intro_render()
{
	screen.stretch_blit(Background, 
		Rect(0,0,Background->bounds.w, Background->bounds.h), 
		Rect(xoffset,yoffset, Background->bounds.w*scale, Background->bounds.h*scale));
	DrawGame();
	DrawScoreBar(!PlayerStates[0], 888888, 888888, 3);
}

void printDebugCpuRamFpsLoad(uint32_t start_frame, uint32_t end_frame)
{
    if(debugMode)
    {
 #if defined(TARGET_32BLIT_HW) || defined(PICO_BUILD)

        // memory stats
#ifdef TARGET_32BLIT_HW
        extern char _sbss, _end, __ltdc_start;

        auto static_used = &_end - &_sbss;
        auto heap_total = &__ltdc_start - &_end;
#else // pico
        extern char __bss_start__, end, __StackLimit;

        auto static_used = &end - &__bss_start__;
        auto heap_total = &__StackLimit - &end;
#endif

        auto heap_used = mallinfo().uordblks;

        auto total_ram = static_used + heap_total;

        Point pos(0, 0);
        int w = screen.bounds.w;
        int h = 10;

        screen.pen = {128, 128, 128};
        int static_px = static_used * w / total_ram;
        screen.rectangle({pos.x, pos.y, static_px, h});

        screen.pen = {255, 255, 255};
        int heap_px = heap_used * w / total_ram;
        screen.rectangle({pos.x + static_px, pos.y, heap_px, h});

        screen.pen = {64, 64, 64};
        screen.rectangle({pos.x + static_px + heap_px, pos.y, w - (static_px + heap_px), h});

        screen.pen = {0, 0, 0};
        screen.rectangle({pos.x, pos.y + h, w, h});

        screen.pen = {255, 255, 255};
        char buf[100];
        snprintf(buf, sizeof(buf), "Mem: %i + %i / %i", static_used, heap_used, total_ram);
        screen.text(buf, minimal_font, {pos.x, pos.y + h, w, h}, true, TextAlign::center_center);

#endif
        uint32_t us = end_frame - start_frame;
        if (us == 0)
            us = 1;   
        long int fps = 1000000.0 / us;
        char buf2[100];
        snprintf(buf2, sizeof(buf2), "FPS: %ld", fps);
        screen.pen = Pen(255,255,255);
        screen.rectangle(Rect(1, screen.bounds.h - 10,  12 * 6 + 2, 10));
        screen.pen = Pen(0,0,0);
        screen.text(buf2, minimal_font, {1, screen.bounds.h - 9}, true, TextAlign::top_left);
    }    
}

void loadGraphics()
{
  Background = Surface::load(background);
  Player = Surface::load(player);
  Enemy = Surface::load(enemy);
}

//intialisation of game & global variables
void init() 
{
	set_screen_mode(ScreenMode::hires);
    scale = (float)screen.bounds.w / WINDOW_WIDTH;
    if ((float)WINDOW_HEIGHT * scale > screen.bounds.h)
        scale = (float)screen.bounds.h / WINDOW_HEIGHT;
    xoffset = (screen.bounds.w - ((float)WINDOW_WIDTH*scale)) / 2;
    yoffset = (screen.bounds.h - ((float)WINDOW_HEIGHT*scale)) / 2;
	initSound();
	initMusic();
	setSoundOn(true);
	setMusicOn(true);
	LoadSettings();
    loadGraphics();	
}

void render(uint32_t time) 
{
	uint32_t start = now_us();
	
	screen.pen = Pen(0,0,0);
	screen.clear();

	switch(GameState)
	{
		case GSIntro :
			Intro_render();
			break;
		case GSGame :
			Game_render();
			break;
		case GSGameOver:
			GameOver_render();
			break;
		default:
			break;
	}
	printDebugCpuRamFpsLoad(start, now_us());
}

void update(uint32_t time) 
{
	switch(GameState)
	{
		case GSIntro :
			Intro_update();
			break;
		case GSGame :
			Game_update();
			break;
		case GSGameOver:
			GameOver_update();
			break;
		default:
			break;
	}

}


// int main(int argc, char **argv)
// {
// 	bool useSoftwareRenderer = true;
//     bool useFullScreenAtStartup = false;
// 	for (int i=0; i < argc; i++)
// 	{
// 		if(strcasecmp(argv[i], "-?") == 0)
// 		{
// 			printf("Usage: formula [Options]\n\n");
// 			printf("  Possible options are:\n");
// 			printf("    -?: show this help message\n");
// 			printf("    -a: Use Acclerated Renderer\n");
// 			printf("    -f: Run fullscreen at startup (by default starts up windowed)\n");
// 			printf("    -fps: Show FPS\n"); 
// 			printf("    -nd: No frame delay (run as fast as possible)\n");
// 			printf("    -s[x]: x = 1-5 scale window by this factor\n");
// 			exit(0);
// 		}
// 		if(strcasecmp(argv[i], "-a") == 0)
// 			useSoftwareRenderer = false;
// 		if(strcasecmp(argv[i], "-nd") == 0)
// 			noDelay = true;
// 		if(strcasecmp(argv[i], "-fps") == 0)
// 			ShowFps = true;
// 		if(strcasecmp(argv[i], "-f") == 0)
// 			useFullScreenAtStartup = true;
// 		if(strcasecmp(argv[i], "-s2") == 0)
// 		{
// 			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 2;
// 			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 2;
// 		}

// 		if(strcasecmp(argv[i], "-s3") == 0)
// 		{
// 			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 3;
// 			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 3;
// 		}

// 		if(strcasecmp(argv[i], "-s4") == 0)
// 		{
// 			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 4;
// 			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 4;
// 		}

// 		if(strcasecmp(argv[i], "-s5") == 0)
// 		{
// 			WINDOW_WIDTH = ORIG_WINDOW_WIDTH * 5;
// 			WINDOW_HEIGHT = ORIG_WINDOW_HEIGHT * 5;
// 		}

// 		if(strcasecmp(argv[i], "-w") == 0)
// 			if(i+1 < argc)
// 				WINDOW_WIDTH = atoi(argv[i+1]);
		
// 		if(strcasecmp(argv[i], "-h") == 0)
// 			if(i+1 < argc)
// 				WINDOW_HEIGHT = atoi(argv[i+1]);
// 	}

// #ifdef FUNKEY		
// 	useSoftwareRenderer = false;
// 	useFullScreenAtStartup = true;	
// #endif

	
// 	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0)
// 	{
// 		printf("SDL Succesfully initialized\n");
// 		uint32_t Flags = 0;
//         if (useFullScreenAtStartup) 
// 		{
// 			WINDOW_WIDTH = 0;
// 			WINDOW_HEIGHT = 0;
//             Flags |= SDL_FULLSCREEN;
//         }
// 		if (useSoftwareRenderer) 
// 			Flags |= SDL_SWSURFACE;
// 		else
// 			Flags |= SDL_HWSURFACE;
// 		Screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT,0, Flags );
// 		if(Screen)
// 		{
// 			WINDOW_WIDTH = Screen->w;
// 			WINDOW_HEIGHT = Screen->h;
// 		    SDL_Surface * Tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, ORIG_WINDOW_WIDTH, ORIG_WINDOW_HEIGHT,0,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
// 			Buffer = SDL_DisplayFormat(Tmp);
// 			SDL_FreeSurface(Tmp);
// 			printf("Succesfully Set %dx%dx32\n",WINDOW_WIDTH,WINDOW_HEIGHT);
// 			SDL_ShowCursor(SDL_DISABLE);
// 			if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0)
// 			{
// 				GlobalSoundEnabled = false;
// 				printf("Failed to initialise sound!\n");
// 			}
// 			else
// 			{
// 				printf("Audio Succesfully initialised!\n");
// 			}
// 			if (TTF_Init() == 0)
// 			{
// 				printf("Succesfully initialized TTF\n");
// 				font = TTF_OpenFont("./filesystem/font.ttf",14);
// 				if (font)
// 				{
// 					printf("Succesfully Loaded font\n");
// 					TTF_SetFontStyle(font,TTF_STYLE_NORMAL);
// 					srand(time(NULL));
// 					TickSound = Mix_LoadWAV("./filesystem/sounds/tick.wav");
// 					CrashSound = Mix_LoadWAV("./filesystem/sounds/crash.wav");
// 					Tmp = IMG_Load("./filesystem/graphics/background.png");
// 					Background = SDL_DisplayFormat(Tmp);
// 					SDL_FreeSurface(Tmp);
// 					Tmp = IMG_Load("./filesystem/graphics/player.png");
// 					Player = SDL_DisplayFormat(Tmp);
// 					SDL_FreeSurface(Tmp);
// 					Tmp = IMG_Load("./filesystem/graphics/enemy.png");
// 					Enemy = SDL_DisplayFormat(Tmp);
// 					SDL_FreeSurface(Tmp);
// 					SDL_ShowCursor(SDL_DISABLE);
// 					LoadSettings();
// 					while (GameState != GSQuit)
// 					{
// 						switch(GameState)
// 						{
// 							case GSIntro :
// 								Intro();
// 								break;
// 							case GSGame :
// 								Game();
// 								break;
// 							case GSGameOver:
// 								GameOver();
// 								break;
// 							default:
// 								break;
// 						}
// 					}
// 					SaveSettings();
// 					TTF_CloseFont(font);
// 					font=NULL;
// 				}
// 				else
// 				{
// 					printf("Failed to Load font\n");
// 				}
// 				TTF_Quit();
// 			}
// 			else
// 			{
// 				printf("Failed to initialize TTF\n");
// 			}
// 			Mix_FreeChunk(CrashSound);
// 			Mix_FreeChunk(TickSound);
// 			SDL_FreeSurface(Screen);
// 			SDL_FreeSurface(Background);
// 			SDL_FreeSurface(Player);
// 			SDL_FreeSurface(Enemy);
// 			Screen=NULL;
// 			Mix_CloseAudio();
// 		}
// 		else
// 		{
// 			printf("Failed to Set Videomode %dx%dx32\n",WINDOW_WIDTH, WINDOW_HEIGHT);
// 		}
// 		SDL_Quit();
// 	}
// 	else
// 	{
// 		printf("Couldn't initialise SDL!\n");
// 	}
// 	return 0;
// }


