/*
   contact.cpp - Information for a Windows Live Messenger contact.

   Copyright (c) 2006 by Michaël Larouche <larouche@kde.org>

   *************************************************************************
   *                                                                       *
   * This library is free software; you can redistribute it and/or         *
   * modify it under the terms of the GNU Lesser General Public            *
   * License as published by the Free Software Foundation; either          *
   * version 2 of the License, or (at your option) any later version.      *
   *                                                                       *
   *************************************************************************
*/
#include "Papillon/Contact"

// Qt includes
#include <QtCore/QStringList>

namespace Papillon
{

class Contact::Private
{
public:
	Private()
	 : clientFeatures(0)
	{}

	QString contactId;
	QString passportId;
	
	Papillon::ClientInfo::Features clientFeatures;
	
	QString displayName;
};

Contact::Contact()
 : d(new Private)
{}

Contact::~Contact()
{
	delete d;
}

bool Contact::isValid() const
{
	return !d->passportId.isEmpty();
}

QString Contact::contactId() const
{
	return d->contactId;
}

void Contact::setContactId(const QString &contactId)
{
	d->contactId = contactId;
}

QString Contact::passportId() const
{
	return d->passportId;
}

void Contact::setPassportId(const QString &passportId)
{
	d->passportId = passportId;
}

ClientInfo::Features Contact::clientFeatures() const
{
	return d->clientFeatures;
}

void Contact::setClientFeatures(const ClientInfo::Features &features)
{
	d->clientFeatures = features;
}

QString Contact::displayName() const
{
	return d->displayName;
}

void Contact::setDisplayName(const QString &displayName)
{
	d->displayName = displayName;
}


}
