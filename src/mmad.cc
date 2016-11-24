/* main.cc
   Contains the main of the game. See README for a description of the game.

   Copyright (C) 2000-2006  Mathias Broxvall
              		    Yannick Perret

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

char *SHARE_DIR_DEFAULT=SHARE_DIR;
#include "general.h"
#include "gameMode.h"
#include "mainMode.h"
#include "editMode.h"
#include "guile.h"
#include "ball.h"
#include "font.h"
#include "glHelp.h"
#include "sound.h"
#include "menuMode.h"
#include "enterHighScoreMode.h"
#include "highScore.h"
#include <getopt.h>
#include "game.h"
#include "forcefield.h"
#include "hofMode.h"
#include <SDL/SDL_image.h>
#include <unistd.h>
#include <settingsMode.h>
#include <settings.h>
#include <setupMode.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include "pipe.h"
#include "pipeConnector.h"
#include "helpMode.h"
#include "map.h"
#include "calibrateJoystickMode.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shlobj.h>
#endif
using namespace std;

/* Important globals */
SDL_Surface *screen = NULL;
const char* program_name;
int silent=0;
int debug_joystick,repair_joystick;

char effectiveShareDir[256];
int screenResolutions[5][2] = {{640,480}, {800,600}, {1024,768}, {1280,1024}, {1600,1200}}, nScreenResolutions=5;

void changeScreenResolution() {
  screenWidth = screenResolutions[Settings::settings->resolution][0]; 
  screenHeight = screenResolutions[Settings::settings->resolution][1];

  if(Settings::settings->colorDepth == 16) {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
  } else {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  }
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

  screen = SDL_SetVideoMode(screenResolutions[Settings::settings->resolution][0], 
			    screenResolutions[Settings::settings->resolution][1], 
			    Settings::settings->colorDepth, 
			    SDL_SWSURFACE | SDL_OPENGL | (Settings::settings->is_windowed?0:SDL_FULLSCREEN));
  if(!screen) return;
  screenWidth = screen->w; screenHeight = screen->h;
  SDL_WarpMouse(screenWidth/2,screenHeight/2);	

  /* Use CapsLock key to determine if mouse should be hidden+grabbed */
  if(SDL_GetModState() & KMOD_CAPS) {
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_ShowCursor(SDL_ENABLE);
  } else {
    SDL_WM_GrabInput(SDL_GRAB_ON);
    SDL_ShowCursor(SDL_DISABLE);
  }

  resetTextures();
}

void print_usage (FILE* stream, int exit_code) {
  fprintf (stream, _("Usage: %s [-w, -m] [-e, -l -t <level>] [-r <width>] [-s <sensitivity>]\n"), program_name);
  fprintf (stream, _("   -h  --help            Display this usage information.\n"));
  fprintf (stream, _("   -e  --edit            Start as level editor.\n"));
  fprintf (stream, _("   -l  --level           Start from level.\n"));
  fprintf (stream, _("   -w  --windowed        Run in window (Default is fullscreen)\n"));
  fprintf (stream, _("   -m  --mute            Mute sound.\n"));
  fprintf (stream, _("   -r  --resolution      Set resolution to 640, 800 or 1024\n"));
  fprintf (stream, _("   -s  --sensitivity     Mouse sensitivity, default 1.0\n"));
  fprintf (stream, _("   -f  --fps             Displays framerate\n"));
  fprintf (stream, _("   -q  --quiet           Do not print anything to stdout\n"));
  fprintf (stream, _("   -v  --version         Prints current version number\n"));
  fprintf (stream, _("   -t  --touch           Updates a map to the latest format\n"));
  fprintf (stream, _("   -y  --low-memory      Attempt to conserve memory usage\n"));
  fprintf (stream, _("   -j  --repair-joystick Correct for bad joysticks\n"));
  fprintf (stream, "\n");
  fprintf (stream, _("Important keyboard shortcuts\n"));
  fprintf (stream, _("   Escape     Soft quit\n"));
  fprintf (stream, _("   CapsLock   Unhide mouse pointer\n"));
  fprintf (stream, _("   CTRL-q     Quit the game immediatly\n"));
  fprintf (stream, _("   CTRL-f     Toggle between fullscreen/windowed mode\n"));
  fprintf (stream, _("   k          Kill the ball\n"));

  exit (exit_code);
}

int testDir() {
  if(!strlen(effectiveShareDir)) return 0;
#ifdef WIN32
  // Really this is just done for asthetics
  for(int i=strlen(effectiveShareDir)-1;i>=0;i--)
    if( effectiveShareDir[i] == '\\' ) effectiveShareDir[i] = '/';
#endif
  DIR *dir = opendir(effectiveShareDir);
  //printf("Looking for %s\n", effectiveShareDir);
  if(!dir) return 0;
  else closedir(dir);
  char str[256];
  snprintf(str,sizeof(str),"%s/levels",effectiveShareDir);
  dir=opendir(str);
  if(!dir) return 0;
  else closedir(dir);
  /* TODO. Test for all other essential subdirectories */
  return 1;
}

void innerMain(void *closure,int argc,char **argv) { 
  int is_running=1;
  int editMode=0,touchMode=0;
  int audio=SDL_INIT_AUDIO;  
  SDL_Event event;
  char str[256],*touchName;
  int i;

  const char* const short_options = "he:l:t:wmr:s:fqvyj";
  const struct option long_options[] = {
         { "help",     0, NULL, 'h' },
         { "edit",     1, NULL, 'e' },
         { "level",    1, NULL, 'l' },
         { "windowed", 0, NULL, 'w' },
         { "mute",     0, NULL, 'm' },
		 { "resolution", 1, NULL, 'r'},
		 { "sensitivity", 1, NULL, 's'},
		 { "fps", 0, NULL, 'f'},
		 { "quiet", 0, NULL, 'q'},
		 { "version", 0, NULL, 'v'},
		 { "touch",   1, NULL, 't'},
		 { "low-memory",   0, NULL, 'y'},
		 { "debug-joystick", 0, NULL, '9' },
		 { "repair-joystick", 0, NULL, 'j' },
         { NULL,       0, NULL, 0   }
  };
  int next_option;
  
  program_name = argv[0];
 
  Settings::init();
  Settings *settings = Settings::settings;
  settings->doSpecialLevel=0;
  settings->setLocale();                 /* Start "correct" i18n as soon as possible */
  low_memory=0; debug_joystick=0; repair_joystick=0;
  do {

#if defined (__SVR4) && defined (__sun)
	next_option = getopt(argc,argv,short_options);
#else
	next_option = getopt_long (argc, argv, short_options, 
	long_options, NULL);
#endif

	/*
#ifdef solaris
	next_option = getopt(argc,argv,short_options);
#else
	next_option = getopt_long (argc, argv, short_options, 
	long_options, NULL);
#endif
	*/

	switch (next_option) {
	case 'h':
	  print_usage (stdout, 0);
	case 'e':
	  fprintf(stderr,_("Commandline switch -e is deprecated, use the editor button the main menu instead\n"));
	  break;
	  //editMode = 1;
	  // fall through to l
	case 'l':
	  snprintf(Settings::settings->specialLevel,sizeof(Settings::settings->specialLevel)-1,"%s",optarg);
	  Settings::settings->doSpecialLevel=1;
	  break;
	case 't':
	  touchMode = 1;
	  touchName=optarg;
	  audio=0; // no audio
	  break;
	case 'w': settings->is_windowed=1;  break;
	case 'm':
	  audio = 0;
	  break;
	case 'r':
	  for(i=0;i<nScreenResolutions;i++) if(screenResolutions[i][0] == atoi(optarg)) break;
	  if(i<nScreenResolutions) settings->resolution=i;
	  else { printf(_("Unknown screen resolution of width %d\n"),i); }
	  break;
	case 's':
	  Settings::settings->mouseSensitivity = atof(optarg);
	  break;
	case 'f':
	  Settings::settings->showFPS=1;
	  break;
	case '?':
	  print_usage (stderr, 1);
	case -1:
	  break;
	case 'q': silent=1;
	  break;
	case 'v': printf("%s v%s\n", PACKAGE, VERSION); exit(0); break;
	case 'y': low_memory=1; break;
	case '9': debug_joystick=1; break;
	case 'j': repair_joystick=1; break;
	default:
	  abort ();
	}
  }
  while (next_option != -1);
  
  if(!silent) {
	printf(_("Welcome to Trackballs. \n"));
	printf(_("Using %s as gamedata directory.\n"),SHARE_DIR);
  }

  /* Initialize SDL */
  if((SDL_Init(SDL_INIT_NOPARACHUTE|SDL_INIT_VIDEO|audio|SDL_INIT_JOYSTICK)==-1)) {
	printf(_("Could not initialize libSDL.\nError message: '%s'\n"), SDL_GetError());
	exit(-1);
  }  
  atexit(SDL_Quit);

  char buffer[1024];
  snprintf(buffer,sizeof(buffer),"/%s/ V%s", PACKAGE, VERSION);
  SDL_WM_SetCaption(buffer, buffer);

  snprintf(str,sizeof(str),"%s/icons/trackballs-32x32.png",SHARE_DIR);
  SDL_Surface *wmIcon = IMG_Load(str);
  if(wmIcon) {  SDL_WM_SetIcon(wmIcon, NULL); }

  // MB: Until here we are using 7 megs of memory
  changeScreenResolution();
  if(!screen) {
    printf(_("Could not initialize screen resolution.\nError message: '%s'\n"), SDL_GetError());
	exit(-1);
  }
  // MB: Until here we are using 42 megs of memory
  
  if(SDL_GetModState() & KMOD_CAPS) {
    printf("Warning - capslock is on, the mouse will be visible and not grabbed\n");
  }


  // set the name of the window

  double bootStart=((double)SDL_GetTicks()) /1000.0 ;
  snprintf(str,sizeof(str),"%s/images/splashScreen.jpg",SHARE_DIR);
  SDL_Surface *splashScreen = IMG_Load(str);
  if(!splashScreen) { printf("Error: failed to load %s\n",str); exit(0); }
  glViewport(0,0,screenWidth,screenHeight);

  // Draw the splash screen
  GLfloat texcoord[4];
  GLfloat texMinX, texMinY;
  GLfloat texMaxX, texMaxY;  
  GLuint splashTexture;
  LoadTexture(splashScreen,texcoord,0,&splashTexture);
  SDL_FreeSurface(splashScreen);
  texMinX = texcoord[0];
  texMinY = texcoord[1];
  texMaxX = texcoord[2];
  texMaxY = texcoord[3];  
  glColor3f(1.0,1.0,1.0);
  for(i=0;i<2;i++) {
    Enter2DMode();
    glBindTexture(GL_TEXTURE_2D, splashTexture);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(texMinX, texMinY); glVertex2i(0,   0  );
    glTexCoord2f(texMaxX, texMinY); glVertex2i(screenWidth, 0  );
    glTexCoord2f(texMinX, texMaxY); glVertex2i(0,   screenHeight);
    glTexCoord2f(texMaxX, texMaxY); glVertex2i(screenWidth, screenHeight);
    glEnd();
    Leave2DMode();
    glBindTexture(GL_TEXTURE_2D, textures[0]); // This is needed for mga_dri cards
    SDL_GL_SwapBuffers();
  }

  // MB: Until here we are using 47 megs of memory. 
  // splashscreen is using 5 megs but it is ok since we are freeing it before the real game


  /* Initialize all modules */
  initGuileInterface();  // MB: 0 megs
  generalInit();         // MB: 0 megs
  glHelpInit();          // MB: 1.5 megs

  // MB: Until here we are using 49 megs of memory. 

  if(audio != 0) soundInit();
  Settings::settings->loadLevelSets();

  // MB: Until here we are using 51 megs of memory. 

  /* Initialize and activate the correct gameModes */
  if(touchMode) { 
	char mapname[512];
	
	snprintf(mapname,sizeof(mapname)-1,"%s/.trackballs/levels/%s.map",getenv("HOME"),touchName);
	if(!fileExists(mapname))
	  snprintf(mapname,sizeof(mapname),"%s/levels/%s.map",SHARE_DIR,touchName);
	if(!fileExists(mapname))
	  snprintf(mapname,sizeof(mapname),"%s",touchName);
	printf("Touching map %s\n",mapname);
	Map *map = new Map(mapname);
	map->save(mapname,(int) map->startPosition[0],(int) map->startPosition[1]);
	exit(0);
  } else if(editMode) {
	EditMode::init();
	Font::init();
	GameMode::activate(EditMode::editMode);
  } else {

	// MB: Reminder 51 megs it was
	Font::init();	       // MB: Until here we are using 54 megs of memory. 
	MenuMode::init();      // MB: Until here we are using 55 megs of memory. 

	MainMode::init();
	HighScore::init();     // MB: 55
	EditMode::init();

	EnterHighScoreMode::init(); // MB: 58
	HallOfFameMode::init();     // MB: 62
	SettingsMode::init();       // MB: 64	
	GameMode::activate(MenuMode::menuMode);	// MB: 65
	GameHook::init();
	CalibrateJoystickMode::init();

	SetupMode::init();          // MB: 71
	Ball::init();
	ForceField::init();         // MB: 71
	HelpMode::init();           // MB: 74
	Pipe::init();
	PipeConnector::init();
	volumeChanged();
	
	// Until here 74 megs

  }
  if(!silent) printf("Trackballs initialization successfull\n");


  /* Make sure splahsscreen has been shown for atleast 2.5 seconds */
  double timeNow=((double)SDL_GetTicks()) /1000.0 ;
  while(timeNow < bootStart + 2.5) {
    Enter2DMode();
    glBindTexture(GL_TEXTURE_2D, splashTexture);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(texMinX, texMinY); glVertex2i(0,   0  );
    glTexCoord2f(texMaxX, texMinY); glVertex2i(screenWidth, 0  );
    glTexCoord2f(texMinX, texMaxY); glVertex2i(0,   screenHeight);
    glTexCoord2f(texMaxX, texMaxY); glVertex2i(screenWidth, screenHeight);
    glEnd();
    Leave2DMode();
    glBindTexture(GL_TEXTURE_2D, textures[0]); // This is needed for mga_dri cards
    SDL_GL_SwapBuffers();
    timeNow=((double)SDL_GetTicks()) /1000.0 ;
  }
  glDeleteTextures(1,&splashTexture);  

  /*                 */
  /* Main event loop */
  /*                 */


  double oldTime,t,td;
  oldTime=((double)SDL_GetTicks()) /1000.0 ;
  SDL_WarpMouse(screenWidth/2,screenHeight/2);

  /* Initialize random number generator */
  int seed=(int) getSystemTime();
  srand(seed);
  int keyUpReceived=1;

  while(is_running) {

	t=((double)SDL_GetTicks()) /1000.0 ;
	td = t - oldTime;
	oldTime = t;
	// reduced to 5 fps
	if(td > 0.2) td = 0.2;
	// we may also add a bottom limit to 'td' to prevent
	//  precision troubles in physic computation
	/*
	// limited to 75 fps
	if (td < 0.013) td = 0.013;
	*/

	/* update font system */
	Font::tick(td);

	/* Update world */
	if(GameMode::current) GameMode::current->idle(td);

	/* Make sure music is still playing */
	soundIdle();

	/* Draw world */
	glViewport(0,0,screenWidth,screenHeight);
	if(GameMode::current) GameMode::current->display();
	else {
	  glClear(GL_COLOR_BUFFER_BIT);
	}
	//Font::draw();
	SDL_GL_SwapBuffers();

	/* Expensive computations has to be done *after* tick+draw to keep world in good
	   synchronisation. */
	if(GameMode::current) GameMode::current->doExpensiveComputations();

	/*                */
	/* Process events */
	/*                */

	/* Joystick - this was a bug since joystick data are also queried using SDL_JoystickUpdate */
	/*
	if(Settings::settings->hasJoystick()) SDL_JoystickEventState(SDL_ENABLE); 
	else SDL_JoystickEventState(SDL_DISABLE); 
	*/

	SDL_MouseButtonEvent *e=(SDL_MouseButtonEvent*)&event;
	while(SDL_PollEvent(&event)) {
	  switch(event.type) {
	  case SDL_QUIT: 
	    is_running=0; 
	    break;
	  case SDL_MOUSEBUTTONDOWN:
	    if(GameMode::current) GameMode::current->mouseDown(e->button,e->x,e->y);
	    break;
	  case SDL_KEYUP:
	    /* Prevent repeated keys */
	    keyUpReceived=1;

	    /* Use Caps lock key to determine if mouse should be hidden+grabbed */
	    if(event.key.keysym.sym == SDLK_CAPSLOCK) {	  
	      if(SDL_GetModState() & KMOD_CAPS) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		SDL_ShowCursor(SDL_ENABLE);
	      } else {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		SDL_ShowCursor(SDL_DISABLE);
	      }
	    } else
	      GameMode::current->keyUp(event.key.keysym.sym);
	  case SDL_KEYDOWN:

	    /* Always quit if the 'q' key is pressed */
	    if(event.key.keysym.sym == 'q' && SDL_GetModState() & KMOD_CTRL) exit(0);

	    /* Change between fullscreen/windowed mode if the 'f' key
	       is pressed */
	    else if(event.key.keysym.sym == 'f' && SDL_GetModState() & KMOD_CTRL) {
	      Settings::settings->is_windowed = Settings::settings->is_windowed ? 0 : 1;
	      changeScreenResolution();
	      /* Flush all events that occured while switching screen
		 resolution */
	      while(SDL_PollEvent(&event)) {}	      
	    }

	    /* Use CapsLock key to determine if mouse should be hidden+grabbed */
	    else if(event.key.keysym.sym == SDLK_CAPSLOCK) {
	      if(SDL_GetModState() & KMOD_CAPS) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		SDL_ShowCursor(SDL_ENABLE);
	      } else {
		SDL_WM_GrabInput(SDL_GRAB_ON);
		SDL_ShowCursor(SDL_DISABLE);
	      }
	    }

	    else if(event.key.keysym.sym == SDLK_ESCAPE) {
	      if(editMode) {
		((EditMode*)GameMode::current)->askQuit();
	      } else if((GameMode::current && GameMode::current == MenuMode::menuMode))
		is_running=0;
	      else { GameMode::activate(MenuMode::menuMode); while(SDL_PollEvent(&event)) {} }

	    }
	    else if(GameMode::current) {
	      /* Prevent repeated keys */
	      if(!keyUpReceived) break;
	      keyUpReceived=0;

	      GameMode::current->key(event.key.keysym.sym);
	    }

	    break;
	  }
	}
  }

  Settings::settings->closeJoystick();
  Settings::settings->save();
  if(GameMode::current)
	delete(GameMode::current); 

  /* TODO. Delete all gamemodes, including the editor */
  SDL_Quit();
}

int main(int argc,char **argv) {
  int i;
  char guileLoadPath[256+16];/*longest effective share directory plus"GUILE_LOAD_PATH="*/
  program_name = argv[0];

  /*** Autmatic detection of SHARE_DIR ***/
  effectiveShareDir[0]=0;
  /* From environment variable */
  char *evar=getenv("TRACKBALLS"); 
  if(evar && strlen(evar) > 0)
       snprintf(effectiveShareDir,sizeof(effectiveShareDir)-1,"%s",evar);
  //printf("Looking for %s\n", effectiveShareDir);
  if(!testDir()) {     
       char thisDir[256];
       /* From arg0/share/trackballs  */
       snprintf(thisDir,sizeof(thisDir),"%s",program_name);
       for(i=strlen(thisDir)-1;i>=0;i--)
         if(thisDir[i] == '/'
#ifdef WIN32
            || thisDir[i] == '\\'
#endif
         ) break;
       if ( i >= 0 ) thisDir[i] = 0;

       /*If no directory breaks are found just use the current directory*/
       if( i <= 0 ) snprintf(thisDir,sizeof(thisDir),".");

       snprintf(effectiveShareDir,sizeof(effectiveShareDir),"%s/share/trackballs",thisDir);

       if(!testDir()) {
         /* From arg0/../share/trackballs */
         snprintf(effectiveShareDir,sizeof(effectiveShareDir),"%s/../share/trackballs",thisDir);

         if(!testDir()) {
           /* From arg0/share */
           snprintf(effectiveShareDir,sizeof(effectiveShareDir),"%s/share",thisDir);

           if(!testDir()) {
             /* From arg0/../share */
             snprintf(effectiveShareDir,sizeof(effectiveShareDir),"%s/../share",thisDir);

             if(!testDir()) {
               /* From compilation default */
               snprintf(effectiveShareDir,sizeof(effectiveShareDir),"%s",SHARE_DIR_DEFAULT);

               if(!testDir()) {
                 printf("Error. Could not find resource directory(%s)\n",effectiveShareDir);
                 exit(0);
               }

             }

           }

         }

       }

  }

  if(NULL == getenv("GUILE_LOAD_PATH"))
  {
     snprintf(guileLoadPath,sizeof(guileLoadPath),"GUILE_LOAD_PATH=%s",effectiveShareDir);       
     putenv(guileLoadPath);
  }

#ifdef WIN32
  if(NULL == getenv("HOME")){
    char homePath[MAX_PATH];
    char homeEnv[MAX_PATH + 5];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, homePath);
    snprintf(homeEnv,sizeof(homeEnv),"HOME=%s", homePath);
    for(int i=strlen(homeEnv)-1;i>=0;i--)
      if( homeEnv[i] == '\\'
	  ) homeEnv[i] = '/';    
    //printf("'%s'\n", homeEnv);
    putenv(homeEnv);
  }
#endif

  /* Initialialize i18n with system defaults, before anything else (eg. settings) have been loaded */
  setlocale(LC_ALL, "");
  char localedir[512];
  
  char *ret;

#ifdef LOCALEDIR
  snprintf(localedir,511,"%s",LOCALEDIR);
#else
  snprintf(localedir,511,"%s/locale",effectiveShareDir);
#endif
  bindtextdomain(PACKAGE,localedir);
  textdomain(PACKAGE);

  scm_boot_guile(argc,argv,innerMain,0);
  return 0;
}
