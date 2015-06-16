/*****************************************************************************\
 *                             GAME STATE
\*****************************************************************************/
/*! @file 
 *
 *  @brief
 *  general game managing controller
 *
 *  controlls all input events and deligates to the managers.
 *  Since this is the central controller try to keep it clean and deligate as much as possible. 
 *
 *  
 */


//---------------------------------------------------------------------------
//  Includes
//---------------------------------------------------------------------------

#include "GameState.h"



/**
 * init is called right at the beginning after the ogre system was created. 
 * at this point none of the voxels is created!
 */
bool GameState::init()
{
    Config::getInstance().init();
    mPlayer.init();

	OgreFramework::getSingletonPtr()->m_pSceneMgr->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

	Ogre::Light* spotLight = OgreFramework::getSingletonPtr()->m_pSceneMgr->createLight("SpotLight");
	spotLight->setDiffuseColour(1, 1, 1.0);
	spotLight->setSpecularColour(1, 1, 1.0);
	spotLight->setType(Ogre::Light::LT_SPOTLIGHT);
	spotLight->setDirection(-1, -1, 0);
	spotLight->setPosition(Ogre::Vector3(200, 200, 0));

	Ogre::String mName = "iso";
	Ogre::SceneNode *mNode = OgreFramework::getSingletonPtr()->m_pSceneMgr->getRootSceneNode()->createChildSceneNode("mynode");

	std::vector<SkeletonNode*> skeleton;
	SphereNode* node = new SphereNode(Ogre::Vector3(0,0,0), 10);
	skeleton.push_back(node);

	/*SphereNode* node2 = new SphereNode(Ogre::Vector3(5, 5, 0), 3);
	skeleton.push_back(node2);
	
	SphereNode* node3 = new SphereNode(Ogre::Vector3(-5, -5, 0), 3);
	skeleton.push_back(node3);

	SphereNode* node4 = new SphereNode(Ogre::Vector3(-5, 5, 0), 3);
	skeleton.push_back(node4);

	SphereNode* node5 = new SphereNode(Ogre::Vector3(5, -5, 0), 3);
	skeleton.push_back(node5);*/

	
	iso.calculate(skeleton, 0.2, SPORE, 0.8);
	
	Ogre::ManualObject* meshManualObject = OgreFramework::getSingletonPtr()->m_pSceneMgr->createManualObject("TESTOBJECT");
	mNode->attachObject(meshManualObject);
		
	meshManualObject->begin("Terrain/White", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	meshManualObject->estimateVertexCount(iso.mVertices.size());

	for (int i = 0; i < iso.mVertices.size(); i++)
	{
		meshManualObject->position(iso.mVertices.at(i));
		meshManualObject->normal(iso.mNormals.at(i));
	}

	for (int i = 0; i < iso.mTriangles.size(); i++)
	{
		int* tri = iso.mTriangles.at(i).i;
		meshManualObject->triangle(tri[0], tri[1], tri[2]);
	}
	meshManualObject->end();

	std::cout << "Vertices: " << iso.mVertices.size() << std::endl;
	return true;
}

/**
 * Main loop hook which is called after all render targets have had their rendering commands issued,
 * but before render windows have been asked to flip their buffers over.
 *
 * @warning	Avoid 
 *
 * @return	True to continue rendering, false to drop out of the rendering loop, exit code 0 to shutdown.
 */
 bool GameState::frameStarted(const Ogre::FrameEvent& evt)
 {
	 //main loop
	 if (evt.timeSinceLastFrame > 0)
	 {
		mPlayer.update(evt.timeSinceLastFrame);
	 }	
	 return true;
 }


 /******************************* mouse and keyboard input callbacks  ************************************/

 /**
 * Handles key press events.
 *
 * @return	Success
 */
bool GameState::keyPressed(const OIS::KeyEvent &arg)
{
    switch (arg.key)
	{
	case OIS::KC_F1: //shows framestats
		OgreFramework::getSingletonPtr()->m_pTrayMgr->toggleAdvancedFrameStats();
		break;
	case OIS::KC_SYSRQ: //Takes a screenshot and saves it in the bin folder 
		OgreFramework::getSingletonPtr()->m_pRenderWnd->writeContentsToTimestampedFile("screenshot", ".jpg");
		break;
	case OIS::KC_ESCAPE: // Shutdown the application
		//mSiccomManager.shutdown();
		OgreFramework::getSingletonPtr()->shutdown();
		break;
	case OIS::KC_R: //shows framestats
		std::vector<SkeletonNode*> skeleton;
		SphereNode* node = new SphereNode(Ogre::Vector3(0, 0, 0), 10);
		skeleton.push_back(node);

		//SphereNode* node2 = new SphereNode(Ogre::Vector3(3, 3, 0), 10);
		//skeleton.push_back(node2);
		
		iso.update(skeleton);
		break;
	}
	
    return true;
}

/**
 * Handles key release events.
 *
 * @return	Success
 */
bool GameState::keyReleased(const OIS::KeyEvent &arg)
{
	    return true;
}

/**
 * Handles mouse press events.
 *
 * @return	Success
 */
bool GameState::mousePressed( const OIS::MouseEvent &evt, OIS::MouseButtonID id )
{
	if(OgreFramework::getSingletonPtr()->m_pTrayMgr->injectPointerDown(evt, id)) return true;
    return true;
}

/**
 * Handles mouse release events.
 *
 * @return	Success
 */
bool GameState::mouseReleased( const OIS::MouseEvent &evt, OIS::MouseButtonID id )
{
	if(OgreFramework::getSingletonPtr()->m_pTrayMgr->injectPointerUp(evt, id)) return true;
    return true;
}

/**
 * Handles mouse move events.
 *
 * @return	Success
 */
bool GameState::mouseMoved(const OIS::MouseEvent &evt)
{
	if(OgreFramework::getSingletonPtr()->m_pTrayMgr->injectPointerMove(evt)) return true;

	OIS::MouseState state = evt.state;
	if ( state.buttonDown(OIS::MB_Right) )
	{
		mPlayer.mouseMoved(evt);
	}
    return true;
}