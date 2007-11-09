/*
    kopeteidentity.cpp - Kopete Identity

    Copyright (c) 2007      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
              (c) 2007         Will Stephenson <wstephenson@kde.org>

    Kopete    (c) 2002-2007 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "kopetepropertycontainer.h"
#include "kopeteidentity.h"
#include "kopeteaccount.h"
#include "kopetecontact.h"
#include "kopeteprotocol.h"

#include <QStringList>
#include <KDebug>
#include <KConfigGroup>
#include <KGlobal>
#include <KSharedConfigPtr>
#include <KLocale>
#include <KRandom>
#include <KMenu>

#include <kdeversion.h>

namespace Kopete 
{

class Identity::Private
{
public:
	Private(const QString &i, const QString &l) : onlineStatus( OnlineStatus::Unknown )
	{
		id = i;
		label = l;
		configGroup = new KConfigGroup(KGlobal::config(), QString::fromLatin1( "Identity_%1" ).arg( id ));
	}
	QList<Kopete::Account*> accounts;
	QString id;
	QString label;
	KConfigGroup *configGroup;
	OnlineStatus::StatusType onlineStatus;
	QString statusMessage;
};

Identity::Identity( const QString &id, const QString &label )
	: d( new Private(id, label) )
{
	load();
	connect(this, SIGNAL(propertyChanged(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)),
	        this, SLOT(slotSaveProperty(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)));
}

Identity::Identity(const QString &label)
: d( new Private(KRandom::randomString(10), label) )
{
	connect(this, SIGNAL(propertyChanged(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)),
	        this, SLOT(slotSaveProperty(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)));
}

Identity * Identity::clone() const
{
	Identity * id = new Identity( label() );
	QMap<QString,QString> props;
	serializeProperties( props );
	id->deserializeProperties( props );

	connect(id, SIGNAL(propertyChanged(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)),
			id, SLOT(slotSaveProperty(Kopete::PropertyContainer*, const QString&, const QVariant &, const QVariant &)));
	return id;
}

Identity::~Identity()
{
	emit identityDestroyed(this);

	delete d->configGroup;
	delete d;
}

QString Identity::id() const
{
	return d->id;
}

QString Identity::label() const
{
	return d->label;
}

void Identity::setLabel(const QString& label)
{
	d->label = label;
}

bool Identity::excludeConnect() const
{
	//TODO implement
	return false;
}

void Identity::setOnlineStatus( uint category, const QString &awayMessage )
{
	OnlineStatusManager::Categories katgor=(OnlineStatusManager::Categories)category;

	d->statusMessage = awayMessage;
	foreach( Account *account ,  d->accounts )
	{
		Kopete::OnlineStatus status = OnlineStatusManager::self()->onlineStatus(account->protocol() , katgor);
		if ( !account->excludeConnect() )
			account->setOnlineStatus( status , awayMessage );
	}
}

OnlineStatus::StatusType Identity::onlineStatus() const
{
	return d->onlineStatus;
}

QString Identity::statusMessage() const
{
	return d->statusMessage;
}

QString Identity::toolTip() const
{

	QString tt = QLatin1String("<qt>");

	QString nick;
	if ( hasProperty(Kopete::Global::Properties::self()->nickName().key()) )
		nick = property(Kopete::Global::Properties::self()->nickName()).value().toString();
	else
		nick = d->label;

	tt+= i18nc( "Identity tooltip information: <nobr>ICON <b>NAME</b></nobr><br /><br />",
				"<nobr><img src=\"kopete-identity-icon:%1\"> <b>%2</b></nobr><br /><br />",
				QString(QUrl::toPercentEncoding( d->label )), nick );

	foreach(Account *a, d->accounts)
	{
		Kopete::Contact *self = a->myself();
		QString onlineStatus = self ? self->onlineStatus().description() : i18n("Offline");
		tt += i18nc( "Account tooltip information: <nobr>ICON <b>PROTOCOL:</b> NAME (<i>STATUS</i>)</nobr><br />",
					 "<nobr><img src=\"kopete-account-icon:%3:%4\"> <b>%1:</b> %2 (<i>%5</i>)</nobr><br />",
					 a->protocol()->displayName(), a->accountLabel(), QString(QUrl::toPercentEncoding( a->protocol()->pluginId() )),
					 QString(QUrl::toPercentEncoding( a->accountId() )), onlineStatus );
	}
	tt += QLatin1String("</qt>");
	return tt;
}

QString Identity::customIcon() const
{
	//TODO implement
	return "user";
}


QList<Account*> Identity::accounts() const
{
	return d->accounts;
}

void Identity::addAccount( Kopete::Account *account )
{
	// check if we already have this account
	if ( d->accounts.indexOf( account ) != -1 )
		return;

	d->accounts.append( account );
	connect(account, SIGNAL(accountDestroyed(const Kopete::Account*)),
			this, SLOT(removeAccount(const Kopete::Account*)));
	//TODO implement the signals for status changes and so
	
	emit identityChanged( this );
}

void Identity::removeAccount( const Kopete::Account *account )
{
	Kopete::Account *a = const_cast<Kopete::Account*>(account);
	if ( !d->accounts.contains( a ) )
		return;

	//TODO disconnect signals and so on
	d->accounts.removeAll( a );
	
	emit identityChanged( this );
}

KConfigGroup *Identity::configGroup() const
{
	return d->configGroup;
}

void Identity::load()
{
	QMap<QString,QString>::iterator it;

	QMap<QString,QString> props = d->configGroup->entryMap();
	deserializeProperties(props);
}

void Identity::save()
{
	// FIXME: using kconfig for now, but I guess this is going to change
	QMap<QString,QString> props;
	serializeProperties(props);
	QMap<QString,QString>::iterator it;
	for (it = props.begin(); it != props.end();  ++it)
	{
		d->configGroup->writeEntry(it.key(), it.value());
	}
}

void Identity::updateOnlineStatus()
{
	Kopete::OnlineStatus mostSignificantStatus;
	Kopete::OnlineStatus::StatusType newStatus = Kopete::OnlineStatus::Unknown; 

	foreach(Account *account, d->accounts)
	{
		// find most significant status
		if ( account->myself() && 
			 account->myself()->onlineStatus() > mostSignificantStatus )
			mostSignificantStatus = account->myself()->onlineStatus();
	}

	newStatus = mostSignificantStatus.status();
	if( newStatus != d->onlineStatus )
	{
		emit onlineStatusChanged( this, d->onlineStatus, newStatus );
		d->onlineStatus = newStatus;
	}
}

void Identity::slotSaveProperty( Kopete::PropertyContainer *container, const QString &key,
		                const QVariant &oldValue, const QVariant &newValue )
{
	save();
}

} //END namespace Kopete

#include "kopeteidentity.moc"

// vim: set noet ts=4 sts=4 sw=4:

