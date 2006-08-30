/* This file is part of KDevelop
Copyright (C) 2001-2002 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
Copyright (C) 2001-2002 Bernd Gehrmann <bernd@kdevelop.org>
Copyright (C) 2001 Sandy Meier <smeier@kdevelop.org>
Copyright (C) 2002 Daniel Engelschalt <daniel.engelschalt@gmx.net>
Copyright (C) 2002 Simon Hausmann <hausmann@kde.org>
Copyright (C) 2002-2003 Roberto Raggi <roberto@kdevelop.org>
Copyright (C) 2003 Mario Scalas <mario.scalas@libero.it>
Copyright (C) 2003 Harald Fernengel <harry@kdevelop.org>
Copyright (C) 2003,2006 Hamish Rodda <rodda@kde.org>
Copyright (C) 2004 Alexander Dymo <adymo@kdevelop.org>
Copyright (C) 2006 Adam Treat <treat@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#include "kdevcore.h"

#include <QPointer>
#include <kstaticdeleter.h>

#include "kdevproject.h"
#include "kdevmainwindow.h"
#include "kdevenvironment.h"
#include "kdevpartcontroller.h"
#include "kdevlanguagesupport.h"
#include "kdevplugincontroller.h"
#include "kdevprojectcontroller.h"
#include "kdevdocumentcontroller.h"
#include "kdevlanguagecontroller.h"
#include "kdevbackgroundparser.h"

class KDevCorePrivate;
static KStaticDeleter<KDevCorePrivate> s_deleter;

class KDevCorePrivate
{
    friend class KStaticDeleter<KDevCorePrivate>;
public:
    static KDevCorePrivate *self()
    {
        if ( !s_private )
            s_deleter.setObject( s_private, new KDevCorePrivate );
        return s_private;
    }

    static KDevCorePrivate *s_private;
    /* We hold these as QPointers in case some errant plugin
       decides to delete one of us.  This way we'll know.
     */
    QPointer<KDevEnvironment> environment;
    QPointer<KDevMainWindow> mainWindow;
    QPointer<KDevPartController> partController;
    QPointer<KDevProjectController> projectController;
    QPointer<KDevLanguageController> languageController;
    QPointer<KDevPluginController> pluginController;
    QPointer<KDevDocumentController> documentController;
    QPointer<KDevBackgroundParser> backgroundParser;

private:
    KDevCorePrivate()
    {}

    ~KDevCorePrivate()
    {
        /* Check to make sure these objects haven't been deleted already.
           Some errant plugin might delete it or they might be deleted by
           Qt in the case of KDevMainWindow.
        */

        /* WARNING!!! Any object that is deleted here should be cleaned up
           in KDevMainWindow::queryClose()!  All data or objects that require
           one of these classes must be deleted or cleaned up there first.
        */
        if ( environment )
            delete environment;
        if ( pluginController )
            delete pluginController;
        if ( projectController )
            delete projectController;
        if ( documentController )
            delete documentController;
        if ( languageController )
            delete languageController;
        if ( backgroundParser )
            delete backgroundParser;
        if ( partController )
            delete partController;
        if ( mainWindow )
            delete mainWindow; //should go last
    }
};

KDevCorePrivate *KDevCorePrivate::s_private = 0;

#define d (KDevCorePrivate::self())

KDevEnvironment *KDevCore::environment()
{
    return d->environment;
}

void KDevCore::setEnvironment( KDevEnvironment *environment )
{
    d->environment = environment;
}

KDevProjectController *KDevCore::projectController()
{
    return d->projectController;
}

void KDevCore::setProjectController( KDevProjectController *projectController )
{
    d->projectController = projectController;
}

KDevProject* KDevCore::activeProject()
{
    Q_ASSERT( d->projectController );
    return d->projectController->activeProject();
}

KDevMainWindow *KDevCore::mainWindow()
{
    return d->mainWindow;
}

void KDevCore::setMainWindow( KDevMainWindow *mainWindow )
{
    d->mainWindow = mainWindow;
}

KDevPluginController* KDevCore::pluginController()
{
    return d->pluginController;
}

void KDevCore::setPluginController( KDevPluginController* pluginController )
{
    d->pluginController = pluginController;
}

KDevDocumentController* KDevCore::documentController()
{
    return d->documentController;
}

void KDevCore::setDocumentController( KDevDocumentController* documentController )
{
    d->documentController = documentController;
}

KDevPartController* KDevCore::partController()
{
    return d->partController;
}

void KDevCore::setPartController( KDevPartController* partController )
{
    d->partController = partController;
}

KDevLanguageController* KDevCore::languageController()
{
    return d->languageController;
}

void KDevCore::setLanguageController( KDevLanguageController * langController )
{
    d->languageController = langController;
}

KDevLanguageSupport *KDevCore::activeLanguage()
{
    Q_ASSERT( d->languageController );
    return d->languageController->activeLanguage();
}

KDevBackgroundParser* KDevCore::backgroundParser()
{
    return d->backgroundParser;
}

void KDevCore::setBackgroundParser( KDevBackgroundParser* backgroundParser )
{
    d->backgroundParser = backgroundParser;
}

void KDevCore::initialize()
{
    //All KDevCore API objects can utilize all resources which
    //depend upon one another.  Can not do this in the constructor
    //as they might depend upon one another.

    //WARNING! the order is important

    Q_ASSERT( d->environment );
    Q_ASSERT( d->partController );
    Q_ASSERT( d->languageController );
    Q_ASSERT( d->documentController );
    Q_ASSERT( d->projectController );
    Q_ASSERT( d->mainWindow );
    Q_ASSERT( d->backgroundParser );
    Q_ASSERT( d->pluginController );

    d->environment->initialize();
    d->partController->initialize();
    d->languageController->initialize();
    d->documentController->initialize();
    d->projectController->initialize();

    d->mainWindow->initialize();

    d->backgroundParser->initialize();
    d->pluginController->initialize();

    d->mainWindow->setVisible( true ); //Done initializing
}

void KDevCore::cleanup()
{
    Q_ASSERT( d->environment );
    Q_ASSERT( d->partController );
    Q_ASSERT( d->languageController );
    Q_ASSERT( d->documentController );
    Q_ASSERT( d->projectController );
    Q_ASSERT( d->mainWindow );
    Q_ASSERT( d->backgroundParser );
    Q_ASSERT( d->pluginController );

    d->environment->cleanup();
    d->partController->cleanup();
    d->languageController->cleanup();
    d->documentController->cleanup();
    d->projectController->cleanup();

    d->mainWindow->cleanup();

    d->backgroundParser->cleanup();
    d->pluginController->cleanup();
}

void KDevCore::loadSettings()
{
    Q_ASSERT( d->environment );
    Q_ASSERT( d->partController );
    Q_ASSERT( d->languageController );
    Q_ASSERT( d->documentController );
    Q_ASSERT( d->projectController );
    Q_ASSERT( d->mainWindow );
    Q_ASSERT( d->backgroundParser );
    Q_ASSERT( d->pluginController );

    d->environment->loadSettings();
    d->partController->loadSettings();
    d->languageController->loadSettings();
    d->documentController->loadSettings();
    d->projectController->loadSettings();

    d->mainWindow->loadSettings();

    d->backgroundParser->loadSettings();
    d->pluginController->loadSettings();
}

void KDevCore::saveSettings()
{
    Q_ASSERT( d->environment );
    Q_ASSERT( d->partController );
    Q_ASSERT( d->languageController );
    Q_ASSERT( d->documentController );
    Q_ASSERT( d->projectController );
    Q_ASSERT( d->mainWindow );
    Q_ASSERT( d->backgroundParser );
    Q_ASSERT( d->pluginController );

//     d->environment->saveSettings();
//     d->partController->saveSettings();
//     d->languageController->saveSettings();
//     d->documentController->saveSettings();
//     d->projectController->saveSettings();
// 
//     d->mainWindow->saveSettings();
// 
//     d->backgroundParser->saveSettings();
//     d->pluginController->saveSettings();
}

//FIXME
namespace MainWindowUtils
{
QString beautifyToolTip( const QString& text )
{
    QString temp = text;
    temp.replace( QRegExp( "&" ), "" );
    temp.replace( QRegExp( "\\.\\.\\." ), "" );
    return temp;
}
}
