/*
    Kopete Yahoo Protocol
    saveyabtask.h - Handles the Yahoo Address Book

    Copyright (c) 2006 André Duffeck <andre.duffeck@kdemail.net>
    Kopete (c) 2002-2006 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "saveyabtask.h"
#include "yabtask.h"
#include "transfer.h"
#include "ymsgtransfer.h"
#include "yahootypes.h"
#include "client.h"
#include <qstring.h>
#include <qdatastream.h>
#include <qdom.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kbufferedsocket.h>

using namespace KNetwork;
SaveYABTask::SaveYABTask(Task* parent) : Task(parent)
{
	kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
}

SaveYABTask::~SaveYABTask()
{
}

void SaveYABTask::onGo()
{
	kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	KBufferedSocket* yahooSocket = new KBufferedSocket( "address.yahoo.com", QString::number(80) );
	connect( yahooSocket, SIGNAL( connected( const KResolverEntry& ) ), this, SLOT( connectSucceeded() ) );
	connect( yahooSocket, SIGNAL( gotError(int) ), this, SLOT( connectFailed(int) ) );

	yahooSocket->connect();
}

void SaveYABTask::setEntry( const YABEntry &entry )
{
	QDomDocument doc("");
	QDomElement root = doc.createElement( "ab" );
	root.setAttribute( "k", client()->userId() );
	root.setAttribute( "cc", "1" );
	doc.appendChild( root );
	
	QDomElement contact = doc.createElement( "ct" );
	entry.fillQDomElement( contact );
	contact.setAttribute( "e", "1" );
	root.appendChild( contact );

	entry.dump();
	m_postData = doc.toString().utf8();
}



void SaveYABTask::connectFailed( int i)
{
	kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << i << ": " << dynamic_cast<const KBufferedSocket*>( sender() )->errorString() << endl;
}

void SaveYABTask::connectSucceeded()
{
	kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << endl;
	KBufferedSocket* socket = const_cast<KBufferedSocket*>( dynamic_cast<const KBufferedSocket*>( sender() ) );

	QString header = QString::fromLatin1("POST /yab/us?v=XM&prog=ymsgr&.intl=us&sync=1&tags=short&noclear=1& HTTP/1.1\r\n"
			"Cookie: Y=%1; T=%2; C=%3 ;B=fckeert1kk1nl&b=2\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.5)\r\n"
			"Host: address.yahoo.com\r\n"
			"Content-length: %4\r\n"
			"Cache-Control: no-cache\r\n\r\n")
			.arg(client()->yCookie()).arg(client()->tCookie())
			.arg(client()->cCookie()).arg(m_postData.local8Bit().size());

	QByteArray buffer;
	QByteArray paket;
	QDataStream stream( buffer, IO_WriteOnly );
	stream.writeRawBytes( header.local8Bit(), header.length() );
	stream.writeRawBytes( m_postData.local8Bit(), m_postData.local8Bit().size() );
	
	if( socket->writeBlock( buffer, buffer.size() ) )
		kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << "Upload Successful. Waiting for confirmation..." << endl;
	else
		kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << "Upload Failed." << endl;
	
	connect( socket, SIGNAL( readyRead() ), this, SLOT( slotRead() ) );
}

void SaveYABTask::slotRead()
{
	KBufferedSocket* socket = const_cast<KBufferedSocket*>( dynamic_cast<const KBufferedSocket*>( sender() ) );
	QByteArray ar( socket->bytesAvailable() );
	socket->readBlock ( ar.data (), ar.size () );
	QString data( ar );
	data = data.right( data.length() - data.find("<?xml") );


	QDomDocument doc;
	QDomNodeList list;
	QDomElement e;
	uint it = 0;
	
	doc.setContent( data );

	list = doc.elementsByTagName( "ct" );			// Get records
	for( it = 0; it < list.count(); it++ )	{
		kdDebug(YAHOO_RAW_DEBUG) << k_funcinfo << "Parsing entry..." << endl;
		if( !list.item( it ).isElement() )
			continue;
		e = list.item( it ).toElement();
		
		YABEntry *entry = new YABEntry;
		entry->fromQDomElement( e );

		if( !e.attribute( "es" ).isEmpty() && e.attribute( "es" ) != "0" )		// Check for errors
		{
			emit error( entry, i18n("The Yahoo Addressbook entry could not be saved:\n%1 - %2").arg( e.attribute("es") ).arg( e.attribute("ee") ) );
		}
		else
		{
			emit gotEntry( entry );
		}
	}


	setSuccess( true );
}
#include "saveyabtask.moc"
