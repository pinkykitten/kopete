/*
    kopeteplugin.cpp - Kopete Plugin API

    Copyright (c) 2001-2002 by Duncan Mac-Vicar P. <duncan@kde.org>
    Copyright (c) 2002-2004 by Olivier Goffart  <ogoffart @tiscalinet.be>

    Copyright (c) 2002-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopeteplugin.h"
#include "kopetepluginmanager.h"

#include <kplugininfo.h>
#include <ksettings/dispatcher.h>

namespace Kopete {


Plugin::Plugin( KInstance *instance, QObject *parent, const char *name )
: QObject( parent, name ), KXMLGUIClient()
{
	setInstance( instance );
	KSettings::Dispatcher::self()->registerInstance( instance, this, SIGNAL( settingsChanged() ) );
}

Plugin::~Plugin()
{
}

QString Plugin::pluginId() const
{
	return QString::fromLatin1( className() );
}

QString Plugin::displayName() const
{
	return pluginInfo()->name();
}

QString Plugin::pluginIcon() const
{
	return pluginInfo()->icon();
}

KPluginInfo *Plugin::pluginInfo() const 
{
	return PluginManager::self()->pluginInfo( this );
}

void Plugin::aboutToUnload()
{
	// Just make the unload synchronous by default
	emit readyForUnload();
}

#if 0
void Plugin::deserialize( MetaContact * /* metaContact */,
	const QMap<QString, QString> & /* stream */ )
{
	// Do nothing in default implementation
}
#endif

void Plugin::virtual_hook( uint, void * ) { }

} //END namespace Kopete

#include "kopeteplugin.moc"


