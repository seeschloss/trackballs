/* player.cc
   Represent The(/A?) player.

   Copyright (C) 2000  Mathias Broxvall

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

#include "general.h"
#include "player.h"
#include "map.h"
#include "gameMode.h"
#include "mainMode.h"
#include "game.h"
#include "debris.h"
#include "math.h"
#include "sound.h"
#include "settings.h"
#include "gamer.h"
#include "splash.h"
#include "settings.h"

using namespace std;

Player::Player(Gamer *gamer) :Ball() {
  inTheAir=0;
  inPipe=0;
  lives = 3;
  realRadius = 0.3; radius = realRadius;

  ballResolution=BALL_HIRES;

  /* Change our color to red */
  primaryColor[0] = 1.0;
  primaryColor[1] = 0.2;
  primaryColor[2] = 0.2;

  specularColor[0] = 1.0;
  specularColor[1] = 1.0;
  specularColor[2] = 1.0;

  timeLeft = 5;
  bounceFactor=.7;
  crashTolerance = 7;
  playing = false;
  score=0;
  hasWon = 0;
  health=1.0;
  oxygen=1.0;
  moveBurst=0.0;

  lastJoyX=lastJoyY=0;
  setReflectivity(0.4,0);

  scoreOnDeath = Game::defaultScores[SCORE_PLAYER][0];
  timeOnDeath = Game::defaultScores[SCORE_PLAYER][1];
}

Player::~Player() {
}
void Player::draw() {
  if(!playing) return;
  Ball::draw();
}
void Player::tick(Real t) {
  double dx,dy;
  int superAccelerate=0;
  static time_t lastTick=0;

  /* Never let us drop below 0 points, it just looks silly */
  if(score<0) score=0;

  if(!Game::current) return;
  if(!playing) return;

  dx=0.0; dy=0.0;
  Map *map = Game::current->map;
  health+=t * 0.4; if(health > 1.0) health = 1.0;

  time_t thisTick = time(NULL);
  if(thisTick != lastTick && !hasWon) {
	timeLeft--;
	if(timeLeft < 15) playEffect(SFX_TIME_WARNING);
  }
  lastTick=thisTick;
  if(timeLeft < 1) { 
    /* Only die from running out of time if we are not running
       in sandbox mode. */
    if(Settings::settings->sandbox == 0)
      die(DIE_TIMEOUT); 
    else {
      /* DEPRACATED: This is instead handled by animated::die and the
	 scoreOnDeath/timeOnDeath variables */
      /*
      score -= 100;
      if(score < 0) score=0;
      timeLeft += 60;*/
    }
    return; 
  }

  /* Check for oxygen by seeing if we are below water level. However:
     when we are in a pipe we might be below ground and thus below the water level even though 
     it's not supposed to be water here.
  */
  if(map->getWaterHeight(position[0],position[1]) > position[2]+radius*0.75 &&
	 (!(inPipe && position[2] < map->getHeight(position[0],position[1])))) {
	oxygen -= (t / (12.0 - 2.0 * Settings::settings->difficulty)) / Game::current->oxygenFactor;
	if(oxygen <= 0.0) die(DIE_OTHER);
  } else oxygen = min(1.0,oxygen+(t/4.0)/Game::current->oxygenFactor);  

  /* Joysticks */
  if(Settings::settings->hasJoystick()) {
	if(Settings::settings->joystickButton(0)) key(' ');
	if(Settings::settings->joystickButton(1)) superAccelerate=1;

	double joyX=Settings::settings->joystickX();
	double joyY=Settings::settings->joystickY();
	/* Help keep the joysticks centered if neccessary */
	if(joyX < 0.1 && joyX > -0.1) joyX=0.0;
	if(joyY < 0.1 && joyY > -0.1) joyY=0.0;

	dx = ((double) joyX) * Settings::settings->mouseSensitivity * t * 200.0;
	dy = ((double) joyY) * Settings::settings->mouseSensitivity * t * 200.0;
  }

  /* Give only *relative* mouse movements */
  if(!Settings::settings->ignoreMouse &&
     !(SDL_GetModState() & KMOD_CAPS)) {
	int mouseX, mouseY;

	Uint8 mouseState=SDL_GetMouseState(&mouseX,&mouseY);
	mouseX -= screenWidth/2;
	mouseY -= screenHeight/2;
	SDL_WarpMouse(screenWidth/2,screenHeight/2);

	/*
	Uint8 mouseState=SDL_GetRelativeMouseState(&mouseX,&mouseY);
	*/

 /* DEBUGGING */
#ifdef FOO
	printf("Screen Width/2=%d, screenHeight/2=%d\n",screenWidth/2,screenHeight/2);
	{
	  int tmp1,tmp2; 
	  SDL_GetMouseState(&tmp1,&tmp2); 
	  if(tmp1 != screenWidth/2 || tmp2 != screenHeight/2) fprintf(stderr,"Error - broken SDL_warp %d %d\n",tmp1,tmp2);
	  if(tmp1 != screenWidth/2 || tmp2 != screenHeight/2) fprintf(stderr,"Again error - broken SDL_warp %d %d\n",tmp1,tmp2);
	}
#endif

	if(mouseState & SDL_BUTTON_RMASK) superAccelerate=1;
	if((mouseX || mouseY)) {
	  dx = mouseX * Settings::settings->mouseSensitivity * 0.1;
	  dy = mouseY * Settings::settings->mouseSensitivity * 0.1;
	}
  }

  /* Handle keyboard steering */
  Uint8 *keystate = SDL_GetKeyState(NULL);
  int shift = SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT);
  double kscale=shift?200.0:100.0;

  if(keystate[SDLK_UP] || keystate[SDLK_KP8]) { dy=-kscale*t; }
  if(keystate[SDLK_DOWN] || keystate[SDLK_KP2]) { dy=kscale*t; }
  if(keystate[SDLK_LEFT] || keystate[SDLK_KP4]) { dx=-kscale*t; }
  if(keystate[SDLK_RIGHT] || keystate[SDLK_KP6]) { dx=kscale*t; }
  if(keystate[SDLK_KP7] || keystate[SDLK_q]) { dx=-kscale*t; dy=-kscale*t; }
  if(keystate[SDLK_KP9] || keystate[SDLK_w]) { dx=kscale*t; dy=-kscale*t; }
  if(keystate[SDLK_KP1] || keystate[SDLK_a]) { dx=-kscale*t; dy=kscale*t; }
  if(keystate[SDLK_KP3] || keystate[SDLK_s]) { dx=kscale*t; dy=kscale*t; }
  if(keystate[SDLK_KP_ENTER] || keystate[SDLK_RETURN]) superAccelerate=1;

  /* rotate control as by settings->rotateArrows */
  {
	double angle=Settings::settings->rotateSteering*M_PI/4.0;
	double tmp=dx*cos(angle)-dy*sin(angle);
	dy=dy*cos(angle)+dx*sin(angle); 
	dx=tmp;
  }

  /* rotate controls if the camera perspective is rotated */
  double angle=((MainMode*)GameMode::current)->xyAngle*M_PI/2.;
  if(angle) {
	double tmp=dx*cos(angle)-dy*sin(angle);
	dy=dy*cos(angle)+dx*sin(angle); 
	dx=tmp;
  }

  /* Cap is the maximum normal acceleration we are allowed to perform (on average) per second */
  double cap=140.0 - 10.0 * Settings::settings->difficulty;
  if(modTimeLeft[MOD_SPEED]) {
	dx = (int) (dx*1.5); dy = (int) (dy*1.5); cap = 200.0;
  }
  if(modTimeLeft[MOD_DIZZY]) cap = cap / 2.0;

  /* Limit size of actual movement according to "cap" */
  /* Uses a trick to avoid capping away movements from integer only devices (eg. mouse) while having a high FPS */
  moveBurst=min(cap*0.2,moveBurst+cap*t);
  double len=sqrt(dx*dx+dy*dy);
  if(len>moveBurst) {
	dx=dx*moveBurst/len;
	dy=dy*moveBurst/len;
	len=moveBurst;
  }
  moveBurst -= len;

  /*
  if(modTimeLeft[MOD_DIZZY]) {
	double tmp = dx * sin(Game::current->gameTime*M_PI*0.1) + dy * cos(Game::current->gameTime*M_PI*0.1);
	dy = dx * cos(Game::current->gameTime*M_PI*0.1) - dy * sin(Game::current->gameTime*M_PI*0.1);
	dx = tmp;
	}*/

  /* Do the movement. Also, if nitro is active then create debris after the ball */

  if(!modTimeLeft[MOD_FROZEN] && !hasWon && is_on) {
    rotation[0] -= 0.025 * (dy - dx);
    rotation[1] -= 0.025 * (dx + dy);
    
    if(superAccelerate && modTimeLeft[MOD_NITRO]) {
      double dirX=rotation[0];
      double dirY=rotation[1];
      double len=sqrt(dirX*dirX+dirY*dirY);
      static double nitroDebris=0.0;
      nitroDebris+=t;
      while(nitroDebris > 0.1) {
		nitroDebris-=0.1;
		Coord3d pos,vel;
		Debris *d=new Debris(this,position,velocity,1.0+frandom()*2.0);
		d->position[0] += (frandom()-0.5)*radius - velocity[0]*0.1;
		d->position[1] += (frandom()-0.5)*radius - velocity[1]*0.1;
		d->position[2] += (frandom()-0.5)*radius;
		d->modTimeLeft[MOD_GLASS]=-1.0;
		d->primaryColor[0]=0.1;
		d->primaryColor[1]=0.6;
		d->primaryColor[2]=0.1;
      }
      if(len>0.0) {
		dirX /= len;
		dirY /= len;
		rotation[0] += dirX * t * 10.0;
		rotation[1] += dirY * t * 10.0;
      }
    }
  }

  if(!hasWon) friction = 1.0;
  else friction = 10.0;
  Ball::tick(t);
}
void Player::key(int c) {
  switch(c) {
  case ' ':  jump(); break;
  case 'k': die(DIE_OTHER); break;
  }
}
void Player::jump() { 
  if(!inTheAir) {
	if(Game::current->map->cell((int)position[0],(int)position[1]).flags & CELL_ACID) 
	  velocity[2] += 1.0 * Game::current->jumpFactor * (1.2 - 0.1*Settings::settings->difficulty);
	else if(modTimeLeft[MOD_JUMP])
	  velocity[2] += 5.0 * Game::current->jumpFactor * (1.2 - 0.1*Settings::settings->difficulty);
	else
	  velocity[2] += 2.0 * Game::current->jumpFactor * (1.2 - 0.1*Settings::settings->difficulty);
	double dh = position[2] - Game::current->map->getHeight(position[0],position[1]);
	if(dh > 0.20) dh=0.20; // we are not supposed to be not "inTheAir" and also have dh > 0.20!
	position[2] += min(0.2,0.20 - dh);
	inTheAir=true;
  } 	
}
void Player::die(int how) {
  Ball::die(how);

  Map *map=Game::current->map;
  if(hasWon) return;
  if(!playing) return;
  if(Game::current->map->isBonus) {
	MainMode::mainMode->bonusLevelComplete();
	return;
  }
  /* Only remove lives if we are not running in sandbox mode */
  if(Settings::settings->sandbox == 0)
    lives--;
  else {
    score -= 100;
    if(score < 0) score=0;
  }
  playing = false;
  health = 0.0;

  int i,j;
  Color color = {1.0, 0.2, 0.2};
  Coord3d pos,vel;
  if(how == DIE_ACID) {
	GLfloat acidColor[4]={0.1,0.8,0.1,0.5};
	Coord3d vel={0.0,0.0,0.0};
	Coord3d center; assign(position,center); center[2]=map->getHeight(center[0],center[1]);					  
	new Splash(center,vel,acidColor,32.0,radius);
  } else
	for(i=0;i<4;i++)
	  for(j=0;j<4;j++) {
		Real a = i/4.0 * M_PI2;
		Real b = (j+0.5)/4.0 * M_PI;
		pos[0] = position[0] + cos(a) * 0.25 * sin(b) * 2.0;
		pos[1] = position[1] + sin(a) * 0.25 * sin(b) * 2.0;
		pos[2] = position[2] + 0.25 * cos(b) + 0.5;
		vel[0] = velocity[0] + (sink?0.1:0.5) * 1/2048.0 * ((rand()%2048)-1024);
		vel[1] = velocity[1] + (sink?0.1:0.5) * 1/2048.0 * ((rand()%2048)-1024);
		vel[2] = velocity[2] + (sink?0.01:0.5) * 1/2048.0 * ((rand()%2048)-1024);
		Debris *d = new Debris(this,pos,vel,2.0+8.0*frandom());
	  }
  
  if(how == DIE_CRASH)
    playEffect(SFX_PLAYER_DIES);
  else if(how == DIE_TIMEOUT) 
    playEffect(SFX_TIMEOUT);
  else if(how == DIE_FF)
    playEffect(SFX_FF_DEATH);

  if(lives <= 0) ((MainMode*)GameMode::current)->playerLoose();
  else ((MainMode*)GameMode::current)->playerDie();
  alive=0;
}

void Player::setStartVariables() {
  sink=0.0;
  rotation[0] = rotation[1] = 0.;
  zero(velocity);
  playing = true;
  alive=1;
  hasWon=0;
  health=1.0;
  oxygen=1.0;
  inTheAir=0;
  inPipe=0;
  moveBurst=0.0;
}

void Player::restart(Coord3d pos) {
  int i;

  setStartVariables();
  assign(pos,position);
  position[2] = Game::current->map->getHeight(position[0],position[1]) + radius;
  modTimeLeft[MOD_DIZZY]=0.0;

  /* reset all mods */
  /*for(i=0;i<NUM_MODS;i++)
    modTimeLeft[i] = 0.0;*/
}
void Player::mouse(int state,int x,int y) {}
void Player::newLevel() {
  Ball::balls->insert(this);
}
void Player::setHealth(Real d) { if(d < health) health = d; if(health < 0.0) health = 0.0; }
Boolean Player::crash(Real speed) {
  double espeed=modTimeLeft[MOD_GLASS] ? (1.5*speed) / crashTolerance : speed / crashTolerance;
  setHealth(1.0-espeed);
  this->Ball::crash(speed);
}
