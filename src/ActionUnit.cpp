/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                *
 *   KoehnHeiko@googlemail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include "Host.h"
#include "TLuaInterpreter.h"
#include <QDebug>
#include "ActionUnit.h"
#include "mudlet.h"

using namespace std;

void ActionUnit::processDataStream( QString & data )
{
    TLuaInterpreter * Lua = mpHost->getLuaInterpreter();
    QString lua_command_string = "command";
    Lua->set_lua_string( lua_command_string, data );
    typedef list<TAction *>::const_iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        TAction * pChild = *it;
        pChild->match( data );
    }
    //data = Lua->get_lua_string( lua_command_string );
}


void ActionUnit::addActionRootNode( TAction * pT )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );    
    }
    mActionRootNodeList.push_back( pT );
        
    mActionMap.insert( pT->getID(), pT );
}

void ActionUnit::reParentAction( int childID, int oldParentID, int newParentID )
{
    QMutexLocker locker(& mActionUnitLock);
    
    TAction * pOldParent = getActionPrivate( oldParentID );
    TAction * pNewParent = getActionPrivate( newParentID );
    TAction * pChild = getActionPrivate( childID );
    if( ! pChild )
    {
        return;
    }
    if( pOldParent )
    {
        pOldParent->popChild( pChild );
    }
    if( ! pOldParent )
    {
        removeActionRootNode( pChild );  
    }
    if( pNewParent ) 
    {
        pNewParent->addChild( pChild );
        if( pChild ) pChild->Tree<TAction>::setParent( pNewParent );
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    if( ! pNewParent )
    {
        addActionRootNode( pChild );
    }
}

void ActionUnit::removeActionRootNode( TAction * pT )
{
    if( ! pT ) return;
    mActionRootNodeList.remove( pT );
}

TAction * ActionUnit::getAction( int id )
{ 
    if( mActionMap.contains( id ) )
    {
        return mActionMap.value( id );
    }
    else
    {
        return 0;
    }
}

TAction * ActionUnit::getActionPrivate( int id )
{ 
    if( mActionMap.find( id ) != mActionMap.end() )
    {
        return mActionMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool ActionUnit::registerAction( TAction * pT )
{
    if( ! pT ) return false;
    
    if( pT->getParent() )
    {
        addAction( pT );
        return true;
    }
    else
    {
        addActionRootNode( pT );    
        return true;
    }
}

void ActionUnit::unregisterAction( TAction * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeAction( pT );
        return;
    }
    else
    {
        removeActionRootNode( pT );    
        return;
    }
}


void ActionUnit::addAction( TAction * pT )
{
    if( ! pT ) return;
    
    QMutexLocker locker(& mActionUnitLock); 
    
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }
    
    mActionMap.insert(pT->getID(), pT);
}

void ActionUnit::removeAction( TAction * pT )
{
    if( ! pT ) return;
    
    mActionMap.remove( pT->getID() );    
}


qint64 ActionUnit::getNewID()
{
    return ++mMaxID;
}

std::list<TToolBar *> ActionUnit::getToolBarList()
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        if( (*it)->mLocation != 4 ) continue;
        bool found = false;
        TToolBar * pTB;
        typedef list<TToolBar *>::iterator I2;
        for( I2 it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( *it2 == (*it)->mpToolBar )
            {
                found = true;
                pTB = *it2;
            }
        }
        if( ! found )
        {
            pTB = new TToolBar( *it, (*it)->getName(), mudlet::self() );
            mToolBarList.push_back( pTB );
        }
        if( (*it)->mOrientation == 1 )
        {
            pTB->setVerticalOrientation();
        }
        else
        {
            pTB->setHorizontalOrientation();
        }
        constructToolbar( *it, mudlet::self(), pTB );
        (*it)->mpToolBar = pTB;
        pTB->setStyleSheet( pTB->mpTAction->css );
    }    
    
    return mToolBarList;
}

std::list<TEasyButtonBar *> ActionUnit::getEasyButtonBarList()
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        bool found = false;
        TEasyButtonBar * pTB;
        typedef list<TEasyButtonBar *>::iterator I2;
        for( I2 it2 = mEasyButtonBarList.begin(); it2!=mEasyButtonBarList.end(); it2++ )
        {
            if( *it2 == (*it)->mpEasyButtonBar )
            {
                found = true;
                pTB = *it2;
            }
        }
        if( ! found )
        {
            pTB = new TEasyButtonBar( *it, (*it)->getName(), mpHost->mpConsole->mpTopToolBar );
            mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB );
            mEasyButtonBarList.push_back( pTB );
        }
        if( (*it)->mOrientation == 1 )
        {
            pTB->setVerticalOrientation();
        }
        else
        {
            pTB->setHorizontalOrientation();
        }
        constructToolbar( *it, mudlet::self(), pTB );
        (*it)->mpEasyButtonBar = pTB;
        pTB->setStyleSheet( pTB->mpTAction->css );
    }

    return mEasyButtonBarList;
}

TAction * ActionUnit::getHeadAction( TToolBar * pT )
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        bool found = false;
        typedef list<TToolBar *>::iterator I2;
        for( I2 it2 = mToolBarList.begin(); it2!=mToolBarList.end(); it2++ )
        {
            if( pT == (*it)->mpToolBar )
            {
                found = true;
                return *it;
            }
        }
    }
    return 0;
}

void ActionUnit::showToolBar( QString & name )
{
    typedef list<TEasyButtonBar *>::iterator IT;
    for( IT it = mEasyButtonBarList.begin(); it!=mEasyButtonBarList.end(); it++ )
    {
        if( (*it)->mpTAction->mName == name )
        {
            (*it)->show();
        }
    }
}

void ActionUnit::hideToolBar( QString & name )
{
    typedef list<TEasyButtonBar *>::iterator IT;
    for( IT it = mEasyButtonBarList.begin(); it!=mEasyButtonBarList.end(); it++ )
    {
        if( (*it)->mpTAction->mName == name )
        {
            (*it)->hide();
        }
    }
}
    
void ActionUnit::constructToolbar( TAction * pA, mudlet * pMainWindow, TToolBar * pTB )
{
    if( pA->mLocation != 4 ) return;
    pTB->clear();
    if( pA->mLocation == 4 )
    {
        pA->expandToolbar( pMainWindow, pTB, 0 );
        pTB->setTitleBarWidget( 0 );
    }
    else
    {
        pA->expandToolbar( pMainWindow, pTB, 0 );
        QWidget * test = new QWidget;
        pTB->setTitleBarWidget( test );
    }
    
    pTB->finalize();
      
    if( pA->mOrientation == 0 )
        pTB->setHorizontalOrientation();
    else
        pTB->setVerticalOrientation();
    
    pTB->setTitleBarWidget( 0 );
    pTB->setFeatures( QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable );
    if( pA->mLocation == 4 )
    {
        mudlet::self()->addDockWidget( Qt::LeftDockWidgetArea, pTB ); //float toolbar
        pTB->setFloating( true );
        QPoint pos = QPoint( pA->mPosX, pA->mPosY );
        pTB->show();
        pTB->move( pos );
        pTB->mpTAction = pA;
        pTB->recordMove();
    }
    else
        pTB->show();
    pTB->setStyleSheet( pTB->mpTAction->css );
}

TAction * ActionUnit::getHeadAction( TEasyButtonBar * pT )
{
    typedef list<TAction *>::iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        bool found = false;
        typedef list<TEasyButtonBar *>::iterator I2;
        for( I2 it2 = mEasyButtonBarList.begin(); it2!=mEasyButtonBarList.end(); it2++ )
        {
            if( pT == (*it)->mpEasyButtonBar )
            {
                found = true;
                return *it;
            }
        }
    }
    return 0;
}

void ActionUnit::constructToolbar( TAction * pA, mudlet * pMainWindow, TEasyButtonBar * pTB )
{
    if( pA->mLocation == 4 ) return;
    pTB->clear();
    pA->expandToolbar( pMainWindow, pTB, 0 );
    pTB->finalize();
    if( pA->mOrientation == 0 )
        pTB->setHorizontalOrientation();
    else
        pTB->setVerticalOrientation();
    switch( pA->mLocation )
    {
        case 0: mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB ); break;
        //case 1: mpHost->mpConsole->mpTopToolBar->layout()->addWidget( pTB ); break;
        case 2: mpHost->mpConsole->mpLeftToolBar->layout()->addWidget( pTB ); break;
        case 3: mpHost->mpConsole->mpRightToolBar->layout()->addWidget( pTB ); break;
    }
    pTB->show();
    pTB->setStyleSheet( pTB->mpTAction->css );
}


void ActionUnit::updateToolbar()
{
    getToolBarList();
    getEasyButtonBarList();
}

bool ActionUnit::serialize( QDataStream & ofs )
{
    bool ret = true;
    ofs << (qint64)mMaxID;
    ofs << (qint64)mActionRootNodeList.size();
    typedef list<TAction *>::const_iterator I;
    for( I it = mActionRootNodeList.begin(); it != mActionRootNodeList.end(); it++)
    {
        TAction * pChild = *it;
        ret = pChild->serialize( ofs );
    }
    return ret;
    
}

bool ActionUnit::restore( QDataStream & ifs, bool initMode )
{
    ifs >> mMaxID;
    qint64 children;
    ifs >> children;
    
    bool ret1 = false;
    bool ret2 = true;
    
    if( ifs.status() == QDataStream::Ok )
        ret1 = true;
    
    mMaxID = 0;
    for( qint64 i=0; i<children; i++ )
    {
        TAction * pChild = new TAction( 0, mpHost );
        ret2 = pChild->restore( ifs, initMode );
        
        if( ! initMode ) 
        {
            delete pChild;
        }
        else 
            registerAction( pChild );
    }
    
    return ret1 && ret2;
}
