/*
    ircaccount.cpp - IRC Account

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>

    Kopete    (c) 2002      by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include "ircaccount.h"
#include "ircprotocol.h"
#include "ircusercontact.h"
#include "ircchannelcontact.h"

#include "kopetecontactlist.h"
#include "kopetemessagemanager.h"
#include "kopetemetacontact.h"
#include "kopeteonlinestatus.h"
#include "kopetemessage.h"
#include "kdebug.h"
#include "ksparser.h"
#include "kirc.h"

IRCAccount::IRCAccount(const QString &accountId, const IRCProtocol *protocol) : KopeteAccount( (KopeteProtocol*)protocol, accountId )
{
	mManager = 0L;
	mProtocol = protocol;

	mNickName = accountId.section('@',0,0);
	QString serverInfo = accountId.section('@',1);
	mServer = serverInfo.section(':',0,0);
	mPort = serverInfo.section(':',1).toUInt();

	actionOnline = new KAction ( i18n("Online"), "", 0, this, SLOT(connect()), this );
	actionOffline =  new KAction ( i18n("Offline"), "", 0, this, SLOT(disconnect()), this);

	mEngine = new KIRC( mServer, mPort );
	QString version=QString::fromLatin1("Kopete IRC Plugin [http://kopete.kde.org]");
	mEngine->setVersionString( version  );
	if( rememberPassword() )
		mEngine->setPassword( getPassword() );

	QObject::connect(mEngine, SIGNAL(successfullyChangedNick(const QString &, const QString &)), this, SLOT(successfullyChangedNick(const QString &, const QString &)));
	QObject::connect(mEngine, SIGNAL(incomingPrivMessage(const QString &, const QString &, const QString &)), this, SLOT(slotNewPrivMessage(const QString &, const QString &, const QString &)));
	QObject::connect(mEngine, SIGNAL(connectedToServer()), this, SLOT(slotConnectedToServer()));
	QObject::connect(mEngine, SIGNAL(connectionClosed()), this, SLOT(slotConnectionClosed()));

	mMySelf = findUser( mNickName );
}

IRCAccount::~IRCAccount()
{
	kdDebug(14120) << k_funcinfo << endl;
	if ( engine()->state() != QSocket::Idle )
		engine()->quitIRC( i18n("Plugin Unloaded") );

	delete engine();
}

KActionMenu *IRCAccount::actionMenu()
{
	QString menuTitle = QString::fromLatin1( " %1 <%2> " ).arg( accountId() ).arg( mMySelf->onlineStatus().description() );

	KActionMenu *mActionMenu = new KActionMenu( accountId(), this );
	mActionMenu->popupMenu()->insertTitle( mMySelf->onlineStatus().genericIcon(), menuTitle, 1 );
	mActionMenu->setIconSet( QIconSet ( mMySelf->onlineStatus().genericIcon() ) );

	mActionMenu->insert( actionOnline );
	mActionMenu->insert( actionOffline );

	return mActionMenu;
}

void IRCAccount::slotNewPrivMessage(const QString &originating, const QString &, const QString &message)
{
	//kdDebug(14120) << k_funcinfo << "o:" << originating << "; t:" << target << endl;
	KopeteContactPtrList others;
	others.append( myself() );
	IRCUserContact *c = findUser(  originating.section('!',0,0) );
	KopeteMessage msg( (KopeteContact*)c, others, message, KopeteMessage::Inbound, KopeteMessage::PlainText, KopeteMessage::Chat );
	msg.setBody( mProtocol->parser()->parse( msg.escapedBody() ), KopeteMessage::RichText );
	c->manager()->appendMessage(msg);
}

void IRCAccount::connect()
{
	engine()->connectToServer( mMySelf->nickName() );
}

void IRCAccount::disconnect()
{
 	engine()->quitIRC("Kopete IRC 2.0. http://kopete.kde.org");
}

void IRCAccount::setAway(bool)
{

}

bool IRCAccount::isConnected()
{
	return (mMySelf->onlineStatus().status() == KopeteOnlineStatus::Online);
}

bool IRCAccount::addContact( const QString &contact, const QString &displayName, KopeteMetaContact *m )
{
	IRCContact *c;

	if( !m )
	{
		m = new KopeteMetaContact();
		KopeteContactList::contactList()->addMetaContact(m);
		m->setDisplayName( displayName );
	}

	if ( contact.startsWith( QString::fromLatin1("#") ) )
		c = static_cast<IRCContact*>( findChannel(contact, m) );
	else
	{
		engine()->addToNotifyList( contact );
		c = static_cast<IRCContact*>( findUser(contact, m) );
	}

	if( c->metaContact() != m )
	{
		KopeteMetaContact *old = c->metaContact();
		c->setMetaContact( m );
		KopeteContactPtrList children = old->contacts();
		if( children.isEmpty() )
			KopeteContactList::contactList()->removeMetaContact( old );
	}
	else if( c->metaContact()->isTemporary() )
		m->setTemporary(false);

	return true;
}

IRCChannelContact *IRCAccount::findChannel(const QString &name, KopeteMetaContact *m  )
{
	if( !m )
	{
		m = new KopeteMetaContact();
		m->setTemporary( true );
	}

	QString lowerName = name.lower();
	IRCChannelContact *channel = 0L;
	if ( !mChannels.contains( lowerName ) )
	{
		channel = new IRCChannelContact(this, name, m);
		mChannels.insert( lowerName, channel );
		if( mEngine->isLoggedIn() )
			channel->setOnlineStatus( IRCProtocol::IRCChannelOnline() );
		QObject::connect(channel, SIGNAL(contactDestroyed(KopeteContact *)), this,
			SLOT(slotContactDestroyed(KopeteContact *)));
	}
	else
	{
		channel = mChannels[ lowerName ];
		kdDebug(14120) << k_funcinfo << lowerName << " conversations:" << channel->conversations() << endl;
	}

	return channel;
}

void IRCAccount::unregisterChannel( const QString &name )
{
	QString lowerName = name.lower();
	if( mChannels.contains( lowerName ) )
	{
		IRCChannelContact *channel = mChannels[lowerName];
		if( channel->conversations() == 0 && channel->metaContact()->isTemporary() )
		{
			kdDebug(14120) << k_funcinfo << name << endl;
			delete channel->metaContact();
		}
	}
}

IRCUserContact *IRCAccount::findUser(const QString &name, KopeteMetaContact *m)
{
	if( !m )
	{
		m = new KopeteMetaContact();
		m->setTemporary( true );
	}

	QString lowerName = name.lower();
	IRCUserContact *user = 0L;
	if ( !mUsers.contains( lowerName ) )
	{
		user = new IRCUserContact(this, name, m);
		mUsers.insert( lowerName, user );
		QObject::connect(user, SIGNAL(contactDestroyed(KopeteContact *)), this,
			SLOT(slotContactDestroyed(KopeteContact *)));
	}
	else
	{
		user = mUsers[ lowerName ];
		kdDebug(14120) << k_funcinfo << lowerName << " conversations:" << user->conversations() << endl;
	}

	return user;
}

void IRCAccount::unregisterUser( const QString &name )
{
	QString lowerName = name.lower();
	if( lowerName != mNickName.lower() && mUsers.contains( lowerName ) )
	{
		IRCUserContact *user = mUsers[lowerName];
		if( user->conversations() == 0 && user->metaContact()->isTemporary() )
		{
			kdDebug(14120) << k_funcinfo << name << endl;
			delete user->metaContact();
		}
	}
}

void IRCAccount::slotContactDestroyed(KopeteContact *contact)
{
	kdDebug(14120) << k_funcinfo << endl;
	const QString nickname = static_cast<IRCContact*>( contact )->nickName().lower();

	if ( nickname.startsWith( QString::fromLatin1("#") ) )
		mChannels.remove( nickname );
	else
	{
		mUsers.remove(nickname);
		engine()->removeFromNotifyList( nickname );
	}
}

void IRCAccount::slotConnectedToServer()
{
	kdDebug(14120) << k_funcinfo << endl;
	mMySelf->setOnlineStatus( IRCProtocol::IRCUserOnline() );
}

void IRCAccount::slotConnectionClosed()
{
	kdDebug(14120) << k_funcinfo << endl;
	mMySelf->setOnlineStatus( IRCProtocol::IRCUserOffline() );
}

void IRCAccount::successfullyChangedNick(const QString &/*oldnick*/, const QString &newnick)
{
	kdDebug(14120) << k_funcinfo << "Changing nick to " << newnick << endl;
	mMySelf->manager()->setDisplayName( mMySelf->caption() );

	if( isConnected() )
		engine()->changeNickname( newnick );
}

bool IRCAccount::addContactToMetaContact( const QString &contactId, const QString &displayName,
	 KopeteMetaContact *parentContact )
{
	//TODO:
	kdDebug(14120) << k_funcinfo << "TODO" << endl;
	return false;
}


#include "ircaccount.moc"
