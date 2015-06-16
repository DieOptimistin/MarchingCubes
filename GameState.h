//---------------------------------------------------------------------------
//                              General game handler
//---------------------------------------------------------------------------
//  Copyright (C): CGVR Bremen (Anthea Sander)
//---------------------------------------------------------------------------


#ifndef __GameState_h_
#define __GameState_h_


//---------------------------------------------------------------------------
//  Includes
//---------------------------------------------------------------------------

#include "OgreFramework.h"
#include "Player.h"
#include "Isosurface.h"


//---------------------------------------------------------------------------
//  The Class
//---------------------------------------------------------------------------

class GameState: public Ogre::FrameListener, public OIS::KeyListener, public OIS::MouseListener, public OgreBites::SdkTrayListener
{

/***************************************************************************/
public:

/*------------------ constructors & destructors ---------------------------*/
	GameState(){};
	~GameState(){};

	//init is called after the ogre system was created
	bool init();

/*-------------------------- callbacks  -----------------------------------*/

	//main loop - will be called for every frame
	virtual bool frameStarted(const Ogre::FrameEvent& evt);
	
	//event handling for keys 
	virtual bool keyPressed(const OIS::KeyEvent &arg);
    virtual bool keyReleased(const OIS::KeyEvent &arg);

	//event handling for mouse
    virtual bool mouseMoved(const OIS::MouseEvent &arg);
	virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

/***************************************************************************/
private:

	Player mPlayer;

	Ogre::RaySceneQuery* m_pRSQ;
	Isosurface iso;
};

#endif // #ifndef __GameState_h_