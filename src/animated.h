/** \file animated.h
	The base class for all Animated (ie. drawable) objects.
*/
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

#ifndef ANIMATED_H
#define ANIMATED_H

#ifndef GAMEHOOK_H
#include "gameHook.h"
#endif

#define DIE_CRASH  0
#define DIE_FF     1
#define DIE_ACID   2
#define DIE_TIMEOUT 3
#define DIE_OTHER  4

class Animated : public GameHook {
 public:
  Animated();

  virtual ~Animated();
  /** First drawing pass of object. Only opaque objects. */ 
  virtual void draw();                 
  /** Draws the second pass (eg. alpha) of object. */
  virtual void draw2();                
  /** Recomputes the bounding box of the object. Needed after changes in size */
  virtual void computeBoundingBox();   
  void onRemove();

  virtual void tick(Real dt);
  virtual void die(int how);

  /** General purpose field. Semantics defined by children */
  int flags;            

  /** Tries to make the current position reasonable. Eg. set
      z-position to height of map */
  virtual void has_moved(); 

  /** Computed by game::draw if the object appears roughly on the screen or not. */
  int onScreen;                        
  /** Center position of object */
  Coord3d position;                    
  /** Lower/higher coordinate of boundingbox relative to position */
  Coord3d boundingBox[2];              

  GLfloat primaryColor[4],secondaryColor[4],specularColor[4];
  GLuint texture;

  /** Number of points the player is awarded when this object dies */
  double scoreOnDeath;
  /** Time modification player is awarded when this object dies */
  double timeOnDeath;
};



#endif
