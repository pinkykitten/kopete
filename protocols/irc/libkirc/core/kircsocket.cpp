/*
    kircsocket.cpp - IRC socket.

    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003-2007 by Michel Hermier <michel.hermier@gmail.com>
    Copyright (c) 2006      by Tommi Rantala <tommi.rantala@cs.helsinki.fi>

    Kopete    (c) 2002-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <config-kopete.h>

#include "kircsocket.moc"

#include <qtextcodec.h>
#include <qtimer.h>
#include <QtCore/QUrl>

#include <QtNetwork/QTcpSocket>

class KIrc::Socket::Private
{
public:
	Private()
		: socket(0)
		, useSSL(false)
		, state(Idle)
		, defaultCodec(KIrc::UTF8)
		, commandHandler(0)
		, entityManager(0)
		, owner(new Entity())
	{ }

	QTcpSocket *socket;
	QUrl url;
	bool useSSL;
	KIrc::Socket::ConnectionState state;

	QTextCodec *defaultCodec;

	KIrc::CommandHandler *commandHandler;
	KIrc::EntityManager *entityManager;
	KIrc::Entity::Ptr owner;
};

using namespace KIrc;

Socket::Socket(QObject *parent)
	: QObject(parent),
	  d( new Private )
{
}

Socket::~Socket()
{
	delete d;
}

Socket::ConnectionState Socket::connectionState() const
{
	return d->state;
}

QTcpSocket *Socket::socket()
{
	return d->socket;
}

QTextCodec *Socket::defaultCodec() const
{
	return d->defaultCodec;
}

void Socket::setDefaultCodec(QTextCodec *codec)
{
	codec = d->defaultCodec;
}

CommandHandler *Socket::commandHandler() const
{
	return d->commandHandler;
}

void Socket::setCommandHandler(CommandHandler *commandHandler)
{
	d->commandHandler = commandHandler;
}

EntityManager *Socket::entityManager() const
{
	return d->entityManager;
}

void Socket::setEntityManager(EntityManager *entityManager)
{
	d->entityManager = entityManager;
}

Entity::Ptr Socket::owner() const
{
	return d->owner;
}

void Socket::setOwner(const Entity::Ptr &newOwner)
{
	d->owner = newOwner;
}

const QUrl &Socket::url() const
{
	return d->url;
}

void Socket::connectToServer(const QUrl &url)
{
	close();
	d->url = "";

	bool useSSL = false;
	if (url.scheme() == "irc")
		useSSL = false;
	else if (url.scheme() == "ircs")
		useSSL = true;
	else
	{
//		#warning FIXME: send an event here to reflect the error
		return;
	}

	QString host = url.host();
	if(host.isEmpty())
		host = "localhost";

	int port = url.port();

	if (port == -1)
	{
		// Make the port being guessed by the socket (look into /etc/services)
		//port = url.scheme();
		;
	}

//	the given url is now validated
	d->url = url;

#ifdef KIRC_SSL_SUPPORT
	if( d->useSSL ) {
		d->socket = new KSSLSocket(this);
		d->socket->setSocketFlags( KExtendedSocket::inetSocket );
	}
#else
	if( d->useSSL ) {
//		kWarning(14121) << "You tried to use SSL, but this version of Kopete was"
//			" not compiled with IRC SSL support. A normal IRC connection will be attempted." << endl;
	}
#endif
	if (!d->socket) {
		d->socket = new QTcpSocket(this);
//		d->socket->setSocketFlags( KExtendedSocket::inputBufferedSocket | KExtendedSocket::inetSocket );
	}

	connect(d->socket, SIGNAL(readyRead()), SLOT(onReadyRead()));
	connect(d->socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	                   SLOT(socketStateChanged(QAbstractSocket::SocketState)));
	connect(d->socket, SIGNAL(error(QAbstractSocket::SocketError)),
	                   SLOT(socketGotError(QAbstractSocket::SocketError)));

#ifdef __GNUC__
	#warning FIXME: send an event here to reflect connection state
#endif

	d->socket->connectToHost(host, port);
}

void Socket::close()
{
	delete d->socket;
	d->socket = 0;
}

void Socket::writeMessage(const Message &msg)
{
#ifdef __GNUC__
	#warning Check message validity before sending it
#endif

	if (!d->socket || d->socket->state() != QAbstractSocket::ConnectedState)
	{
//		postErrorEvent(i18n("Attempting to send while not connected: %1", msg.data()));
		return;
	}

	qint64 wrote = d->socket->write(msg.rawLine() + "\n\r");

//	if (wrote == -1)
//		kDebug(14121) << k_funcinfo << "Socket write failed!" << endl;

//	kDebug(14121) << QString::fromLatin1("(%1 bytes) >> %2").arg(wrote).arg(rawMsg) << endl;
}

void Socket::showInfoDialog()
{
/*
	if( d->useSSL )
	{
		static_cast<KSSLSocket*>( d->socket )->showInfoDialog();
	}
*/
}
#if 0
void Socket::postEvent(Event::MessageType messageType, const QString &message)
{
#ifdef __GNUC__
#warning implement me
#endif
}
#endif
void Socket::setConnectionState(ConnectionState newstate)
{
	if (d->state != newstate)
	{
		d->state = newstate;
		emit connectionStateChanged(newstate);
	}
}

void Socket::onReadyRead()
{
	// The caller can also be self so lets check the socket still exist

	if (d->socket && d->socket->canReadLine())
	{
		QByteArray rawMsg = d->socket->readLine();

		Message msg;
		msg.setLine(rawMsg);
		msg.setDirection(Message::InGoing);

		if (msg.isValid())
			emit receivedMessage(msg);
		else
			//postErrorEvent(i18n("Parse error while parsing: %1").arg(msg.rawLine()));

		QTimer::singleShot( 0, this, SLOT( onReadyRead() ) );
	}

//	if(d->socket->socketStatus() != KExtendedSocket::connected)
//		error();
}

void Socket::socketStateChanged(QAbstractSocket::SocketState newstate)
{
	switch (newstate) {
	case QAbstractSocket::UnconnectedState:
		setConnectionState(Idle);
		break;
	case QAbstractSocket::HostLookupState:
		setConnectionState(HostLookup);
		break;
	case QAbstractSocket::ConnectingState:
		setConnectionState(Connecting);
		break;
	case QAbstractSocket::ConnectedState:
		setConnectionState(Open);
		break;
	case QAbstractSocket::ClosingState:
		setConnectionState(Closing);
		break;
	default:
//		postErrorEvent(i18n("Unknown SocketState value:%1", newstate));
		close();
	}
}

void Socket::socketGotError(QAbstractSocket::SocketError)
{
	/*
	KBufferedSocket::SocketError err = d->socket->error();

	// Ignore spurious error
	if (err == KBufferedSocket::NoError)
		return;

	QString errStr = d->socket->errorString();
	kDebug(14121) << k_funcinfo << "Socket error: " << errStr << endl;
//	postErrorEvent(errStr);

	// ignore non-fatal error
	if (!KBufferedSocket::isFatalError(err))
		return;

	close();
	*/
}
/*
QByteArray Socket::encode(const QString &str, bool *success, QTextCodec *codec) const
{
	kDebug(14121) << k_funcinfo << endl;
	*success = false;

	if (!codec || !codec->canEncode(str))
	{
		if (!d->defaultCodec || !d->defaultCodec->canEncode(str))
		{
			if (!UTF8 || !UTF8->canEncode(str))
			{
//				postErrorEvent(i18n("Codec Error: .").arg(codec->name()));
				return QByteArray();
			}
			else
			{
//				postErrorEvent(i18n("Codec Warning: Using codec %1 for:\n%2.")
//					.arg(UTF8->name(), str));
				codec = UTF8;
			}
		}
		else
			codec = d->defaultCodec;
	}

	*success = true;
	return codec->fromUnicode(str);
}
*/
