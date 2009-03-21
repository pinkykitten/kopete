/*
    Kopete Contactlist Model

    Copyright (c) 2007      by Aleix Pol              <aleixpol@gmail.com>
    Copyright (c) 2008      by Matt Rogers            <mattr@kde.org>
    Copyright (c) 2009      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    Copyright     2009      by Roman Jarosz           <kedgedev@gmail.com>

    Kopete    (c) 2002-2009 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "contactlistmodel.h"

#include <QMimeData>
#include <QFile>
#include <QTextDocument>
#include <QDomDocument>

#include <KDebug>
#include <KEmoticonsTheme>

#include "kopeteaccount.h"
#include "kopetepicture.h"
#include "kopetemetacontact.h"
#include "kopeteprotocol.h"
#include "kopetecontact.h"
#include "kopetecontactlist.h"
#include "kopeteitembase.h"
#include "kopeteappearancesettings.h"
#include "kopeteemoticons.h"

namespace Kopete {

namespace UI {

ContactListModel::ContactListModel( QObject* parent )
 : QAbstractItemModel( parent )
{
	AppearanceSettings* as = AppearanceSettings::self();
	m_manualGroupSorting = (as->contactListGroupSorting() == AppearanceSettings::EnumContactListGroupSorting::Manual);
	m_manualMetaContactSorting = (as->contactListMetaContactSorting() == AppearanceSettings::EnumContactListMetaContactSorting::Manual);
	connect ( AppearanceSettings::self(), SIGNAL(configChanged()), this, SLOT(appearanceConfigChanged()) );
}

// Can't be in constructor because we can't call virtual method loadContactList from constructor
void ContactListModel::init()
{
	Kopete::ContactList* kcl = Kopete::ContactList::self();

	// Wait till whole contact list is loaded so we can apply manual sort.
	if ( !kcl->loaded() )
		connect( kcl, SIGNAL(contactListLoaded()), this, SLOT(loadContactList()) );
	else
		loadContactList();
}

int ContactListModel::columnCount ( const QModelIndex& ) const
{
	return 1;
}

Qt::DropActions ContactListModel::supportedDropActions() const
{
	return QAbstractItemModel::supportedDropActions();
	return Qt::MoveAction;
}

QMimeData* ContactListModel::mimeData(const QModelIndexList &indexes) const
{
	// the mimeData encoded by QAbstractItemView is required for the
	// drop indicators to work on QTreeView
	QMimeData *mdata = QAbstractItemModel::mimeData(indexes);
	QByteArray encodedData;

	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	enum DragType { DragNone = 0x0, DragGroup = 0x1, DragMetaContact = 0x2 };
	int dragType = DragNone;
	foreach (QModelIndex index, indexes)
	{
		if ( !index.isValid() )
			continue;

		switch ( data(index, Kopete::Items::TypeRole).toInt() )
		{
		case Kopete::Items::MetaContact:
			{
				dragType |= DragMetaContact;
				// each metacontact entry will be encoded as group/uuid to
				// make sure that when moving a metacontact from one group
				// to another it will handle the right group
				
				// so get the group id
				QString text = data(index.parent(), Kopete::Items::IdRole).toString();
				
				// and the metacontactid
				text += "/" + data(index, Kopete::Items::UuidRole).toString();
				stream << text;
				break;
			}
		case Kopete::Items::Group:
			{
				dragType |= DragGroup;
				// so get the group id
				QString text = data(index, Kopete::Items::IdRole).toString();
				stream << text;
				break;
			}
		}
	}

	if ( (dragType & (DragGroup | DragMetaContact)) == (DragGroup | DragMetaContact) )
		return 0;

	if ( (dragType & DragGroup) == DragGroup )
		mdata->setData("application/kopete.group", encodedData);
	else if ( (dragType & DragMetaContact) == DragMetaContact )
		mdata->setData("application/kopete.metacontacts.list", encodedData);
	else
		return 0;

	return mdata;
}

bool ContactListModel::loadModelSettings( const QString& modelType )
{
	QDomDocument doc;

	QString fileName = KStandardDirs::locateLocal( "appdata", QLatin1String( "contactlistmodel.xml" ) );
	if ( QFile::exists( fileName ) )
	{
		QFile file( fileName );
		if( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
		{
			kDebug() << "error opening/parsing file " << fileName;
			QDomElement dummyElement;
			loadModelSettingsImpl( dummyElement );
			return false;
		}
	}

	QDomElement rootElement = doc.firstChildElement( "Models" );
	if ( !rootElement.isNull() )
	{
		QDomElement modelRootElement;
		QDomNodeList modelRootList = rootElement.elementsByTagName("Model");
		for ( int index = 0; index < modelRootList.size(); ++index )
		{
			QDomElement element = modelRootList.item( index ).toElement();
			if ( !element.isNull() && element.attribute( "type" ) == modelType )
			{
				modelRootElement = element;
				break;
			}
		}

		if ( !modelRootElement.isNull() )
		{
			loadModelSettingsImpl( modelRootElement );
			return true;
		}
	}

	QDomElement dummyElement;
	loadModelSettingsImpl( dummyElement );
	return false;
}

bool ContactListModel::saveModelSettings( const QString& modelType )
{
	QDomDocument doc;

	QString fileName = KStandardDirs::locateLocal( "appdata", QLatin1String( "contactlistmodel.xml" ) );
	if ( QFile::exists( fileName ) )
	{
		QFile file( fileName );
		if( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
			kDebug() << "error opening/parsing file " << fileName;

		file.close();
	}

	QDomElement rootElement = doc.firstChildElement( "Models" );
	if ( rootElement.isNull() )
	{
		rootElement = doc.createElement( "Models" );
		doc.appendChild( rootElement );
	}

	QDomElement modelRootElement;
	QDomNodeList modelRootList = rootElement.elementsByTagName("Model");
	for ( int index = 0; index < modelRootList.size(); ++index )
	{
		QDomElement element = modelRootList.item( index ).toElement();
		if ( !element.isNull() && element.attribute( "type" ) == modelType )
		{
			modelRootElement = element;
			break;
		}
	}

	if ( modelRootElement.isNull() )
	{
		modelRootElement = doc.createElement( "Model" );
		rootElement.appendChild( modelRootElement );
		modelRootElement.setAttribute( "type", modelType );
	}

	saveModelSettingsImpl( doc, modelRootElement );

	QFile file( fileName );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		kDebug() << "error saving file " << fileName;
		return false;
	}

	QTextStream out( &file );
	out << doc.toString();
	file.close();
	return true;
}

void ContactListModel::addMetaContact( Kopete::MetaContact* contact )
{
	connect( contact, SIGNAL(onlineStatusChanged(Kopete::MetaContact*, Kopete::OnlineStatus::StatusType)),
	         this, SLOT(handleContactDataChange(Kopete::MetaContact*)) );
	connect( contact, SIGNAL(statusMessageChanged(Kopete::MetaContact*)),
	         this, SLOT(handleContactDataChange(Kopete::MetaContact*)) );
}

void ContactListModel::removeMetaContact( Kopete::MetaContact* contact )
{
	disconnect( contact, SIGNAL(onlineStatusChanged(Kopete::MetaContact*, Kopete::OnlineStatus::StatusType)),
	            this, SLOT(handleContactDataChange(Kopete::MetaContact*)));
	disconnect( contact, SIGNAL(statusMessageChanged(Kopete::MetaContact*)),
	            this, SLOT(handleContactDataChange(Kopete::MetaContact*)) );
}

void ContactListModel::addGroup( Kopete::Group* group )
{
	Q_UNUSED( group );
}

void ContactListModel::removeGroup( Kopete::Group* group )
{
	Q_UNUSED( group );
}

void ContactListModel::addMetaContactToGroup( Kopete::MetaContact *mc, Kopete::Group *group )
{
	Q_UNUSED( mc );
	Q_UNUSED( group );
}

void ContactListModel::removeMetaContactFromGroup( Kopete::MetaContact *mc, Kopete::Group *group )
{
	Q_UNUSED( mc );
	Q_UNUSED( group );
}

void ContactListModel::moveMetaContactToGroup( Kopete::MetaContact *mc, Kopete::Group *from, Kopete::Group *to)
{
	removeMetaContactFromGroup(mc, from);
	addMetaContactToGroup(mc, to);
}

void ContactListModel::loadContactList()
{
	Kopete::ContactList* kcl = Kopete::ContactList::self();
	disconnect( kcl, SIGNAL(contactListLoaded()), this, SLOT(loadContactList()) );

	// MetaContact related
	connect( kcl, SIGNAL( metaContactAdded( Kopete::MetaContact* ) ),
	         this, SLOT( addMetaContact( Kopete::MetaContact* ) ) );
	connect( kcl, SIGNAL( metaContactRemoved( Kopete::MetaContact* ) ),
	         this, SLOT( removeMetaContact( Kopete::MetaContact* ) ) );

	// Group related
	connect( kcl, SIGNAL( groupAdded( Kopete::Group* ) ),
	         this, SLOT( addGroup( Kopete::Group* ) ) );
	connect( kcl, SIGNAL( groupRemoved( Kopete::Group* ) ),
	         this, SLOT( removeGroup( Kopete::Group* ) ) );

	// MetaContact and Group related
	connect( kcl, SIGNAL(metaContactAddedToGroup(Kopete::MetaContact*, Kopete::Group*)),
	         this, SLOT(addMetaContactToGroup(Kopete::MetaContact*, Kopete::Group*)) );
	connect( kcl, SIGNAL(metaContactRemovedFromGroup(Kopete::MetaContact*, Kopete::Group*)),
	         this, SLOT(removeMetaContactFromGroup(Kopete::MetaContact*, Kopete::Group*)) );
	connect( kcl, SIGNAL(metaContactMovedToGroup(Kopete::MetaContact*, Kopete::Group*, Kopete::Group*)),
	         this, SLOT(moveMetaContactToGroup(Kopete::MetaContact*, Kopete::Group*, Kopete::Group*)));
}

QVariant ContactListModel::metaContactData( const Kopete::MetaContact* mc, int role ) const
{
	switch ( role )
	{
	case Qt::DisplayRole:
		return mc->displayName();
		break;
	case Kopete::Items::MetaContactImageRole:
		return metaContactImage( mc );
		break;
	case Qt::ToolTipRole:
		return metaContactTooltip( mc );
		break;
	case Kopete::Items::TypeRole:
		return Kopete::Items::MetaContact;
		break;
	case Kopete::Items::ObjectRole:
		return qVariantFromValue( (QObject*)mc );
		break;
	case Kopete::Items::UuidRole:
		return mc->metaContactId().toString();
		break;
	case Kopete::Items::OnlineStatusRole:
		return mc->status();
		break;
	case Kopete::Items::StatusMessageRole:
		return mc->statusMessage().message();
		break;
	case Kopete::Items::StatusTitleRole:
		return mc->statusMessage().title();
		break;
	case Kopete::Items::AccountIconsRole:
		QList<QVariant> accountIconList;
		foreach ( Kopete::Contact *contact, mc->contacts() )
			accountIconList << qVariantFromValue( contact->onlineStatus().iconFor( contact ) );

		return accountIconList;
	}

	return QVariant();
}

QVariant ContactListModel::metaContactImage( const Kopete::MetaContact* mc ) const
{
	using namespace Kopete;

	int iconMode = AppearanceSettings::self()->contactListIconMode();
	if ( iconMode == AppearanceSettings::EnumContactListIconMode::IconPhoto )
	{
		QImage img = mc->picture().image();
		if ( !img.isNull() && img.width() > 0 && img.height() > 0 )
			return img;
	}

	switch( mc->status() )
	{
	case OnlineStatus::Online:
		if( mc->useCustomIcon() )
			return mc->icon( ContactListElement::Online );
		else
			return QString::fromUtf8( "user-online" );
		break;
	case OnlineStatus::Away:
		if( mc->useCustomIcon() )
			return mc->icon( ContactListElement::Away );
		else
			return QString::fromUtf8( "user-away" );
		break;
	case OnlineStatus::Unknown:
		if( mc->useCustomIcon() )
			return mc->icon( ContactListElement::Unknown );
		if ( mc->contacts().isEmpty() )
			return QString::fromUtf8( "metacontact_unknown" );
		else
			return QString::fromUtf8( "user-offline" );
		break;
	case OnlineStatus::Offline:
	default:
		if( mc->useCustomIcon() )
			return mc->icon( ContactListElement::Offline );
		else
			return QString::fromUtf8( "user-offline" );
		break;
	}

	return QVariant();
}

QString ContactListModel::metaContactTooltip( const Kopete::MetaContact* metaContact ) const
{
	// We begin with the meta contact display name at the top of the tooltip
	QString toolTip = QLatin1String("<qt><table cellpadding=\"0\" cellspacing=\"1\">");
	toolTip += QLatin1String("<tr><td>");
	
	if ( !metaContact->picture().isNull() )
	{
#ifdef __GNUC__
#warning Currently using metaContact->picture().path() but should use replacement of KopeteMimeSourceFactory
#endif
#if 0
			QString photoName = QLatin1String("kopete-metacontact-photo:%1").arg( KUrl::encode_string( metaContact->metaContactId() ));
			//QMimeSourceFactory::defaultFactory()->setImage( "contactimg", metaContact->photo() );
			toolTip += QString::fromLatin1("<img src=\"%1\">").arg( photoName );
#endif
		toolTip += QString::fromLatin1("<img src=\"%1\">").arg( metaContact->picture().path() );
	}

	toolTip += QLatin1String("</td><td>");

	QString displayName;
	QList<KEmoticonsTheme::Token> t = Kopete::Emoticons::tokenize( metaContact->displayName());
	QList<KEmoticonsTheme::Token>::iterator it;
	for( it = t.begin(); it != t.end(); ++it )
	{
		if( (*it).type == KEmoticonsTheme::Image )
			displayName += (*it).picHTMLCode;
		else if( (*it).type == KEmoticonsTheme::Text )
			displayName += Qt::escape( (*it).text );
	}

	toolTip += QString::fromLatin1("<b><font size=\"+1\">%1</font></b><br><br>").arg( displayName );

	QList<Contact*> contacts = metaContact->contacts();
	if ( contacts.count() == 1 )
		return toolTip + contacts.first()->toolTip() + QLatin1String("</td></tr></table></qt>");

	toolTip += QLatin1String("<table>");

	// We are over a metacontact with > 1 child contacts, and not over a specific contact
	// Iterate through children and display a summary tooltip
	foreach ( Contact* c, contacts )
	{
		QString iconName = QString::fromLatin1("kopete-contact-icon:%1:%2:%3")
			.arg( QString(QUrl::toPercentEncoding( c->protocol()->pluginId() )),
			      QString(QUrl::toPercentEncoding( c->account()->accountId() )),
			      QString(QUrl::toPercentEncoding( c->contactId() ) )
			    );

		toolTip += i18nc("<tr><td>STATUS ICON <b>PROTOCOL NAME</b> (ACCOUNT NAME)</td><td>STATUS DESCRIPTION</td></tr>",
		                 "<tr><td><img src=\"%1\">&nbsp;<nobr><b>%2</b></nobr>&nbsp;<nobr>(%3)</nobr></td><td align=\"right\"><nobr>%4</nobr></td></tr>",
		                 iconName, Kopete::Emoticons::parseEmoticons(c->property(Kopete::Global::Properties::self()->nickName()).value().toString()) , c->contactId(), c->onlineStatus().description() );
	}

	return toolTip + QLatin1String("</table></td></tr></table></qt>");
}

}

}

#include "contactlistmodel.moc"
//kate: tab-width 4
