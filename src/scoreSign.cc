/* 

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
#include "glHelp.h"
#include "animated.h"
#include "sign.h"
#include "scoreSign.h"
#include "game.h"
#include "player.h"

using namespace std;

ScoreSign::ScoreSign(int points,Coord3d position) 
  :Sign("++++",4.0,1.0,100.0,position)
{
  init(points,position,SCORESIGN_SCORE); 
}

ScoreSign::ScoreSign(int points,Coord3d position,int type) 
  :Sign("++++",4.0,1.0,100.0,position)
{  
  init(points,position,type);
}

void ScoreSign::init(int points,Coord3d position,int type) {
  this->type = type;
  this->points = 0.0;
  pointsLeft = points;
  pointsPerSecond = points / 4.0;
  lastLife=life + 1.0;

  if(type == SCORESIGN_SCORE) {
  } else if(type == SCORESIGN_TIME) {
    primaryColor[0] = 0.8;
    primaryColor[1] = 0.8;
    primaryColor[2] = 0.2;
  } 
}

void ScoreSign::tick(Real t) {
  char str[256];
  int oldPoints = (int) points;
  if(life < t) t = life;
  points = pointsPerSecond * (4.0 - life + t);
  int p = ((int) points) - oldPoints;

  /* Limit number of recreated textures to 10 textures/second */
  if(life < lastLife - 0.1) {
	lastLife=life;
	
	if(type == SCORESIGN_SCORE) {
	  if(points >= 0)
	    snprintf(str,sizeof(str),"+%d",(int) points);
	  else
	    snprintf(str,sizeof(str),"%d",(int) points);
	} else if(type == SCORESIGN_TIME) {
	  if(points >= 0)
	    snprintf(str,sizeof(str),"+%ds",(int) points);
	  else
	    snprintf(str,sizeof(str),"%ds",(int) points);
	}
	mkTexture(str);
  }
  if(Game::current) {
    switch(type) {
    case SCORESIGN_SCORE: Game::current->player1->score += p; break;
    case SCORESIGN_TIME: Game::current->player1->timeLeft += p; break;
    }
  }

  Sign::tick(t);
}
