//
// C++ Implementation: eventtransfer
//
// Description: 
//
//
// Author: Kopete Developers <kopete-devel@kde.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "eventtransfer.h"

EventTransfer::EventTransfer( const Q_UINT32 eventType, const QString & source, QDateTime timeStamp )
 : Transfer(), m_eventType( eventType ), m_source( source ), m_timeStamp( timeStamp )
{
	m_contentFlags |= ( EventType | Source | TimeStamp );
}


EventTransfer::~EventTransfer()
{
}

// query contents

bool EventTransfer::hasEventType()
{
	return ( m_contentFlags & EventType );
}

bool EventTransfer::hasSource()
{
	return ( m_contentFlags & Source );
}

bool EventTransfer::hasTimeStamp()
{
	return ( m_contentFlags & TimeStamp );
}

bool EventTransfer::hasGuid()
{
	return ( m_contentFlags & Guid );
}

bool EventTransfer::hasFlags()
{
	return ( m_contentFlags & Flags );
}

bool EventTransfer::hasMessage()
{
	return ( m_contentFlags & Message );
}

bool EventTransfer::hasStatus()
{
	return ( m_contentFlags & Status );
}

bool EventTransfer::hasStatusText()
{
	return ( m_contentFlags & StatusText );
}

// accessors
	
int EventTransfer::eventType()
{ 
	return m_eventType;
}

QString EventTransfer::source()
{
	return m_source;
}

QDateTime EventTransfer::timeStamp()
{
	return m_timeStamp;
}

QString EventTransfer::guid()
{
	return m_guid;
}

Q_UINT32 EventTransfer::flags()
{
	return m_flags;
}

QString EventTransfer::message()
{
	return m_message;
}

Q_UINT16 EventTransfer::status()
{
	return m_status;
}

QString EventTransfer::statusText()
{
	return m_statusText;
}

// mutators
void EventTransfer::setGuid( const QString & guid )
{
	m_contentFlags |= Guid;
	m_guid = guid;
}

void EventTransfer::setFlags( const Q_UINT32 flags )
{
	m_contentFlags |= Flags;
	m_flags = flags;
}

void EventTransfer::setMessage( const QString & message )
{
	m_contentFlags |= Message;
	m_message = message;
}

void EventTransfer::setStatus( const Q_UINT16 inStatus )
{
	m_contentFlags |= Status;
	m_status = inStatus;
}

void EventTransfer::setStatusText( const QString & statusText )
{
	m_contentFlags |= StatusText;
	m_statusText = statusText;
}

