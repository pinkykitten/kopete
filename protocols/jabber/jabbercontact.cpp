/***************************************************************************
                          jabbercontact.cpp  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    copyright            : (C) 2002 by Daniel Stone, Till Gerken,
                           The Kopete Development Team
    email                : dstone@kde.org, till@tantalo.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qcursor.h>
#include <qfont.h>
#include <qptrlist.h>
#include <qstrlist.h>

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>

#include <psi/tasks.h>
#include <psi/types.h>

#include "dlgjabbervcard.h"
#include "dlgjabberrename.h"
#include "jabbercontact.h"
#include "jabberprotocol.h"
#include "jabberresource.h"
#include "jabbermessage.h"
#include "kopete.h"
#include "kopetestdaction.h"
#include "kopetewindow.h"
#include "kopetemessage.h"
#include "kopetemessagemanager.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemetacontact.h"
#include "ui/kopetehistorydialog.h"

/**
 * JabberContact constructor
 */
JabberContact::JabberContact(QString userId, QString nickname, QStringList groups, JabberProtocol *p, KopeteMetaContact *mc, QString identity) : KopeteContact(p->id(), mc)
{

	// save parent protocol object
	protocol = p;
	
	parentMetaContact = mc;
	
	historyDialog = 0L;
	mMsgManagerKCW = 0L;
	mMsgManagerKEW = 0L;

	resourceOverride = false;

	mIdentityId = identity;

	rosterItem.setJid(Jabber::Jid(userId));
	rosterItem.setName(nickname);
	rosterItem.setGroups(groups);

	initActions();

	// create a default (empty) resource for the contact
	JabberResource *defaultResource = new JabberResource(QString::null, -1, QDateTime::currentDateTime(), JabberProtocol::STATUS_OFFLINE, "");
	resources.append(defaultResource);

	activeResource = defaultResource;
	
	// update the displayed name
	setDisplayName(rosterItem.name());

	// specifically cause this instance to update this contact as offline
	slotUpdatePresence(JabberProtocol::STATUS_OFFLINE, "");

}

/**
 * JabberContact destructor
 */
JabberContact::~JabberContact()
{

	delete actionMessage;
	delete actionChat;
	delete actionHistory;
	delete actionRename;
	delete actionSelectResource;
	delete actionRetrieveVCard;
	delete actionSendAuth;
	delete actionStatusAway;
	delete actionStatusChat;
	delete actionStatusXA;
	delete actionStatusDND;

}

/**
 * Factory for a Kopete Email Window
 */
KopeteMessageManager* JabberContact::msgManagerKEW()
{
	QPtrList<KopeteContact>contacts;

	contacts.append(this);

	if (!mMsgManagerKEW)
	{
		// get new instance
		mMsgManagerKEW = kopeteapp->sessionFactory()->create(protocol->myself(), contacts, protocol, "jabber_logs/" + userId() +".log", KopeteMessageManager::Email);

		connect(mMsgManagerKEW, SIGNAL(messageSent(const KopeteMessage&, KopeteMessageManager*)),
				this, SLOT(slotSendMsgKEW(const KopeteMessage&)));
	}

	return mMsgManagerKEW;

}

/**
 * Factory for a Kopete Chat Window
 */
KopeteMessageManager* JabberContact::msgManagerKCW()
{
	QPtrList<KopeteContact>contacts;

	contacts.append(this);

	if (!mMsgManagerKCW)
	{
		// get new instance
		mMsgManagerKCW = kopeteapp->sessionFactory()->create(protocol->myself(), contacts, protocol, "jabber_logs/" + userId() +".log", KopeteMessageManager::ChatWindow);

		connect(mMsgManagerKCW, SIGNAL(messageSent(const KopeteMessage&, KopeteMessageManager*)),
				this, SLOT(slotSendMsgKCW(const KopeteMessage&)));
	}

	return mMsgManagerKCW;

}

/**
 * Create actions
 */
void JabberContact::initActions()
{

	actionChat = KopeteStdAction::sendMessage(this, SLOT(slotChatUser()), this, "actionChat");
	actionMessage = new KAction(i18n("Send Email Message"), "mail_generic", 0, this, SLOT(slotEmailUser()), this, "actionMessage");
	actionRemove = KopeteStdAction::deleteContact(this, SLOT(slotRemoveThisUser()), this, "actionDelete");
	actionHistory = KopeteStdAction::viewHistory(this, SLOT(slotViewHistory()), this, "actionHistory");
	actionRename = new KAction(i18n("Rename Contact"), "editrename", 0, this, SLOT(slotRenameContact()), this, "actionRename");
	actionSelectResource = new KSelectAction(i18n("Select Resource"), "selectresource", 0, this, SLOT(slotSelectResource()), this, "actionSelectResource");
	actionRetrieveVCard = new KAction(i18n("Get vCard"), "identity", 0, this, SLOT(slotRetrieveVCard()), this, "actionRetrieveVCard");
	actionSendAuth = new KAction(i18n("(Re)send authorization to"), "", 0, this, SLOT(slotSendAuth()), this, "actionSendAuth");
	actionStatusAway = new KAction(i18n("Away"), "jabber_away", 0, this,SLOT(slotStatusAway()), this,  "actionAway");
	actionStatusChat = new KAction(i18n("Free to chat"), "jabber_online", 0, this, SLOT(slotStatusChat()), this, "actionChat");
	actionStatusXA = new KAction(i18n("Extended away"), "jabber_away", 0, this, SLOT(slotStatusXA()),this, "actionXA");
	actionStatusDND = new KAction(i18n("Do not Disturb"), "jabber_na", 0, this, SLOT(slotStatusDND()), this, "actionDND");

}

/**
 * Popup the context menu
 */
void JabberContact::showContextMenu(const QPoint& point, const QString&)
{

	KPopupMenu *popup = new KPopupMenu();
	popup->insertTitle(userId() + " (" + activeResource->resource() + ")");

	KGlobal::config()->setGroup("Jabber");
	
	// depending on the window type preference,
	// chat window or email window goes first
	if (KGlobal::config()->readBoolEntry("EmailDefault", false))
	{
		actionMessage->plug(popup);
		actionChat->plug(popup);
	}
	else
	{
		actionChat->plug(popup);
		actionMessage->plug(popup);
	}

	// if the contact is online,
	// display the resources we have for it
	if (status() != Offline)
	{
		QStringList items;
		int activeItem = 0;
		JabberResource *tmpBestResource = bestResource();
		
		// put best resource first
		items.append("Automatic (best resource)");

		if(tmpBestResource->resource() != QString::null)
			items.append(tmpBestResource->resource());
		
		// iterate through available resources
		int i = 1;
		for (JabberResource *tmpResource = resources.first(); tmpResource; tmpResource = resources.next(), i++)
		{
			// skip the default (empty) resource
			if(tmpResource->resource() == QString::null)
			{
				i--;
				continue;
			}

			// only add the item if it is not the best resource
			// (which we already added above)
			if (tmpResource != tmpBestResource)
				items.append(tmpResource->resource());
				
			// mark the currently active resource
			if (tmpResource->resource() == activeResource->resource() && resourceOverride)
			{
				kdDebug() << "[JabberContact] Activating item " << i << " as active resource." << endl;
				activeItem = i;
			}
		}
		
		// attach list to the menu action
		actionSelectResource->setItems(items);
		
		// make sure the active item is selected
		actionSelectResource->setCurrentItem(activeItem);
		
		// plug it to the menu
		actionSelectResource->plug(popup);
	}
	
	popup->insertSeparator();

	actionRetrieveVCard->plug(popup);
	actionHistory->plug(popup);
	popup->insertSeparator();
	actionRename->plug(popup);
	actionSendAuth->plug(popup);

	// Availability popup menu
	KPopupMenu *popup_status = new KPopupMenu();
	popup->insertItem("Set availabilty", popup_status);

	actionStatusChat->plug(popup_status);
	actionStatusAway->plug(popup_status);
	actionStatusXA->plug(popup_status);
	actionStatusDND->plug(popup_status);
	// Remove
	popup->insertSeparator();
	actionRemove->plug(popup);

	popup->exec( point );

	delete popup;
	delete popup_status;

}

void JabberContact::slotUpdatePresence(const JabberProtocol::Presence newStatus, const QString &reason)
{
	
	QString dbgString = "[JabberContact] Updating contact " + userId() + "  to ";
	switch(newStatus)
	{
		case JabberProtocol::STATUS_ONLINE:
			dbgString += "STATUS_ONLINE";
			break;
		case JabberProtocol::STATUS_AWAY:
			 dbgString += "STATUS_AWAY";
			break;
		case JabberProtocol::STATUS_XA:
			dbgString += "STATUS_XA";
			break;
		case JabberProtocol::STATUS_DND:
			dbgString += "STATUS_DND";
			break;
		case JabberProtocol::STATUS_INVISIBLE:
			dbgString += "STATUS_INVISIBLE";
			break;
		case JabberProtocol::STATUS_OFFLINE:
			dbgString += "STATUS_OFFLINE";
			break;
	}
	kdDebug() << dbgString << " (Reason: " << reason << ")" << endl;

	awayReason = reason;
	presence = newStatus;

	emit statusChanged(this, status());

}

void JabberContact::slotUpdateContact(const Jabber::RosterItem &item)
{

	rosterItem = item;

	// only update the nickname if its not empty
	if(!item.name().isEmpty() && !item.name().isNull())
		setDisplayName(item.name());

	emit statusChanged(this, status());

}

void JabberContact::slotRenameContact()
{

	kdDebug() << "[JabberContact] Renaming contact." << endl;

	dlgJabberRename *renameDialog = new dlgJabberRename;

	renameDialog->setUserId(userId());
	renameDialog->setNickname(displayName());
	
	connect(renameDialog, SIGNAL(rename(const QString &)), this, SLOT(slotDoRenameContact(const QString &)));

	renameDialog->show();

}

void JabberContact::slotDoRenameContact(const QString &nickname)
{
	QString name = nickname;

	// if the name has been deleted, revert
	// to using the user ID instead
	if (name == QString(""))
		name = userId();

	rosterItem.setName(name);

	// send rename request to protocol backend
	protocol->updateContact(rosterItem);

	// update display (we cannot use setDisplayName()
	// as parameter here as the above call is asynch
	// and thus our changes did not make it to the server
	// yet)
	setDisplayName(name);

}

void JabberContact::slotDeleteMySelf(bool)
{
    
	delete this;

}

JabberContact::ContactStatus JabberContact::status() const
{
	JabberContact::ContactStatus retval;

	if(!protocol->isConnected())
		return Offline;

	switch(presence)
	{
		case JabberProtocol::STATUS_ONLINE:
					retval = Online;
					break;

		case JabberProtocol::STATUS_AWAY:
		case JabberProtocol::STATUS_XA:
		case JabberProtocol::STATUS_DND:
					retval = Away;
					break;

		default:
					retval = Offline;
					break;
	}
	
	return retval;
	
}

QString JabberContact::statusText() const
{
	QString txt;

	switch (presence)
	{
		case JabberProtocol::STATUS_ONLINE:
			txt = i18n("Online");
			break;

		case JabberProtocol::STATUS_AWAY:
			txt = i18n("Away");
			break;

		case JabberProtocol::STATUS_XA:
			txt = i18n("Extended Away");
			break;

		case JabberProtocol::STATUS_DND:
			txt = i18n("Do Not Disturb");
			break;

		default:
			txt = i18n("Offline");
			break;
	}

	// append away reason if there is one
	if ( !awayReason.isNull() && !awayReason.isEmpty() )
		txt += " (" + awayReason + ")";

	return txt;

}

QString JabberContact::statusIcon() const
{
	QString icon;
	
	switch(presence)
	{
		case JabberProtocol::STATUS_ONLINE:
					icon = "jabber_online";
					break;

		case JabberProtocol::STATUS_AWAY:
		case JabberProtocol::STATUS_XA:
					icon = "jabber_away";
					break;

		case JabberProtocol::STATUS_DND:
					icon = "jabber_na";
					break;

		default:
					icon = "jabber_offline";
					break;
	}
	
	return icon;
	
}

void JabberContact::slotRemoveThisUser()
{

	kdDebug() << "[JabberContact] Removing user " << userId() << endl;

	// unsubscribe
	protocol->removeContact(rosterItem);

	delete this;

}

void JabberContact::slotSendAuth()
{

	kdDebug() << "[JabberContact] (Re)sendAuth -> recall subscribe() " << userId() << endl;

//	protocol->addContact(userId());

}

void JabberContact::addToGroup(const QString &group)
{

	kdDebug() << "[JabberContact] Adding user " << userId() << " to group " << group << endl;

	QStringList groups = rosterItem.groups();
	groups.append(group);
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

void JabberContact::moveToGroup(const QString &from, const QString &to)
{

	kdDebug() << "[JabberContact] Moving user " << userId() << " from group " << from << " to group " << to << endl;

	QStringList groups = rosterItem.groups();
	groups.append(to);
	groups.remove(from);
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

void JabberContact::removeFromGroup(const QString &group)
{

	kdDebug() << "[JabberContact] Removing user " << userId() << " from group " << group << endl;

	QStringList groups = rosterItem.groups();
	groups.remove(group);
	rosterItem.setGroups(groups);

	protocol->updateContact(rosterItem);

}

int JabberContact::importance() const
{
	int value;
	
	switch(status())
	{
		case JabberProtocol::STATUS_ONLINE:
					value = 20;
					break;
		case JabberProtocol::STATUS_AWAY:
					value = 15;
					break;
		case JabberProtocol::STATUS_XA:
					value = 12;
					break;
		case JabberProtocol::STATUS_DND:
					value = 10;
					break;
		default:
					value = 0;
					break;
	}
	
	return value;
	
}

void JabberContact::slotChatUser()
{

	kdDebug() << "[JabberContact] Opening chat with user " << userId() << endl;
	
	msgManagerKCW()->readMessages();

}

void JabberContact::slotEmailUser()
{
	
	kdDebug() << "[JabberContact] Opening message with user " << userId() << endl;
	
	msgManagerKEW()->readMessages();
	msgManagerKEW()->slotSendEnabled(true);

}

void JabberContact::execute()
{

	KGlobal::config()->setGroup("Jabber");
	
	if (KGlobal::config()->readBoolEntry("EmailDefault", false))
		// user selected email window as preference
		slotEmailUser();
	else
		// user selected chat window as preference
		slotChatUser();

}

void JabberContact::slotNewMessage(const Jabber::Message &message)
{
	KopeteContactPtrList contactList;

	contactList.append(protocol->myself());

	KopeteMessage newMessage(this, contactList, message.body(), message.subject(), KopeteMessage::Inbound);

	// depending on the incoming message type,
	// append it to the correct widget
	if (message.type() == "email")
	{
		msgManagerKEW()->appendMessage(newMessage);
		msgManagerKEW()->slotSendEnabled(false);
	}
	else
	{
		msgManagerKCW()->appendMessage(newMessage);
	}

}

void JabberContact::slotViewHistory()
{

	if (historyDialog == 0L)
	{
		historyDialog = new KopeteHistoryDialog(QString("jabber_logs/%1.log").arg(userId()), displayName(), true, 50, 0, "JabberHistoryDialog");
		connect(historyDialog, SIGNAL(closing()), this, SLOT(slotCloseHistoryDialog()));
	}
	
	historyDialog->show();
	historyDialog->raise();

}

void JabberContact::slotCloseHistoryDialog()
{
    
	delete historyDialog;
	historyDialog = 0L;

}

void JabberContact::km2jm(const KopeteMessage &km, Jabber::Message &jm)
{
	JabberContact *to = dynamic_cast<JabberContact *>(km.to().first());
	const JabberContact *from = dynamic_cast<const JabberContact *>(km.from());

	// ugly hack, Jabber::Message does not have a setFrom() method
	Jabber::Message jabMessage(Jabber::Jid(from->userId()));

	// if a resource has been selected for this contact,
	// send to this special resource - otherwise,
	// just send to the server and let the server decide
	if (activeResource->resource() != QString::null)
		jabMessage.setTo(Jabber::Jid(QString("%1/%2").arg(to->userId(), 1).arg(activeResource->resource(), 2)));
	else
		jabMessage.setTo(Jabber::Jid(to->userId()));

	//jabMessage.setFrom(from->userId();
	jabMessage.setBody(km.body());
	jabMessage.setType("chat");
	jabMessage.setSubject(km.subject());

	jm = jabMessage;

}

void JabberContact::slotSendMsgKEW(const KopeteMessage& message)
{
	Jabber::Message jabMessage;

	kdDebug() << "[JabberContact] slotSendMsgKEW called: " << message.body() << "." << endl;

	km2jm(message, jabMessage);

	jabMessage.setType("normal");

	// pass message on to protocol backend
	protocol->slotSendMessage(jabMessage);

	// append message to window
	msgManagerKEW()->appendMessage(message);

}

void JabberContact::slotSendMsgKCW(const KopeteMessage& message)
{
	Jabber::Message jabMessage;

	kdDebug() << "[JabberContact] slotSendMsgKCW called: " << message.body() << "." << endl;

    km2jm(message, jabMessage);

	jabMessage.setType("chat");

	// pass message on to protocol backend
	protocol->slotSendMessage(jabMessage);

	// append message to window
	msgManagerKCW()->appendMessage(message);

}

/**
 * Determine the currently best resource for the contact
 */
JabberResource *JabberContact::bestResource()
{
	JabberResource *resource, *tmpResource;

	// iterate through all available resources
	for (resource = tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		kdDebug() << "[JabberContact] Processing resource " << tmpResource->resource() << endl;

		if (tmpResource->priority() > resource->priority())
		{
			kdDebug() << "[JabberContact] Got better resource " << tmpResource->resource() << " through better priority." << endl;
			resource = tmpResource;
		}
		else
		{
			if (tmpResource->priority() == resource->priority())
			{
				if (tmpResource->timestamp() >= resource->timestamp())
				{
					kdDebug() << "[JabberContact] Got better resource " << tmpResource->resource() << " through newer timestamp." << endl;
					resource = tmpResource;
				}
				else
				{
					kdDebug() << "[JabberContact] Discarding resource " << tmpResource->resource() << " with older timestamp." << endl;
				}
			}
			else
			{
				kdDebug() << "[JabberContact] Discarding resource " << tmpResource->resource() << " with worse priority." << endl;
			}
		}
	}

	return resource;

}

void JabberContact::slotResourceAvailable(const Jabber::Jid &jid, const Jabber::Resource &resource)
{
	QString theirJID = QString("%1@%2").arg(jid.user(), 1).arg(jid.host(), 2);

	// safety check: don't process resources of other users
	if (theirJID != userId())
		return;

	kdDebug() << "[JabberContact] Adding new resource '" << resource.name() << "' for " << userId() << endl;

	/*
	 * if the new resource already exists, remove the current
	 * instance of it (Psi will emit resourceAvailable()
	 * whenever a certain resource changes status)
	 * removing the current instance will prevent double entries
	 * of the same resource.
	 * updated resource will be re-added below
	 * FIXME: this should be done using the QPtrList methods! (needs checking of pointer values)
	 */
	for (JabberResource *tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		// FIXME: shouldn't this be resource instead of jid.resource?
		if (tmpResource->resource() == resource.name())
		{
			kdDebug() << "[JabberContact] Resource " << tmpResource->resource() << " already added, removing instance with older timestamp" << endl;
			resources.remove();
		}
	}

	JabberProtocol::Presence status = JabberProtocol::STATUS_ONLINE;
	if(resource.status().show() == "away")
	{
		status = JabberProtocol::STATUS_AWAY;
	}
	else
	if(resource.status().show() == "xa")
	{
		status = JabberProtocol::STATUS_XA;
	}
	else
	if(resource.status().show() == "dnd")
	{
		status = JabberProtocol::STATUS_DND;
	}
	
	JabberResource *newResource = new JabberResource(resource.name(), resource.priority(), resource.status().timeStamp(), status, resource.status().status());
	resources.append(newResource);

	JabberResource *tmpBestResource = bestResource();

	kdDebug() << "[JabberContact] Best resource is now " << tmpBestResource->resource() << "." << endl;

	slotUpdatePresence(tmpBestResource->status(), tmpBestResource->reason());

	// switch active resource if override is not in effect
	if(!resourceOverride)
		activeResource = tmpBestResource;

}

void JabberContact::slotResourceUnavailable(const Jabber::Jid &jid, const Jabber::Resource &resource)
{
	JabberResource *tmpResource;
	QString theirJID = QString("%1@%2").arg(jid.user(), 1).arg(jid.host(), 2);
	
	// safety check: don't process resources of other users
	if (theirJID != userId())
		return;

	kdDebug() << "[JabberContact] Removing resource '" << jid.resource() << "' for " << userId() << endl;

	for (tmpResource = resources.first(); tmpResource; tmpResource = resources.next())
	{
		if (tmpResource->resource() == resource.name())
		{
			kdDebug() << "[JabberContact] Got a match in " << tmpResource->resource() << ", removing." << endl;
			
			if (resources.remove())
				kdDebug() << "[JabberContact] Successfully removed, there are now " << resources.count() << " resources!" << endl;
			else
				kdDebug() << "[JabberContact] Ack! Couldn't remove the resource. Bugger!" << endl;

			break;
		}
	}
	
	JabberResource *newResource = bestResource();
	
	kdDebug() << "[JabberContact] Best resource is now " << newResource->resource() << "." << endl;
	slotUpdatePresence(newResource->status(), newResource->reason());

	// if override was in effect or we just deleted the current
	// resource, switch to new resource
	if(resourceOverride || (activeResource->resource() == resource.name()))
	{
		resourceOverride = false;
		activeResource = newResource;
	}

}

void JabberContact::slotSelectResource()
{
	
	if (actionSelectResource->currentItem() == 0)
	{
		kdDebug() << "[JabberContact] Removing active resource, trusting bestResource()." << endl;
		
		resourceOverride = false;
		activeResource = bestResource();
	}
	else
	{
		QString selectedResource = actionSelectResource->currentText();

		kdDebug() << "[JabberContact] Moving to resource " << selectedResource << endl;

		resourceOverride = true;
		
		for (JabberResource *resource = resources.first(); resource; resource = resources.next())
		{
			if (resource->resource() == selectedResource)
			{
				kdDebug() << "[JabberContact] New active resource is " << resource->resource() << endl;

				activeResource = resource;

				break;
			}
		}
	}
	
}


void JabberContact::slotRetrieveVCard()
{

	// just pass the request on to the protocol backend
	protocol->slotRetrieveVCard(userId());

}

void JabberContact::slotGotVCard(Jabber::JT_VCard *vCard)
{

	kdDebug() << "[JabberContact] Got vCard for user " << vCard->jid().userHost() << ", displaying." << endl;
	
	dlgVCard = new dlgJabberVCard(kopeteapp->mainWindow(), "dlgJabberVCard", vCard);

/*	if (mEditingVCard) {
		connect(dlgVCard, SIGNAL(saveAsXML(QDomElement &)), this, SLOT(slotSaveVCard(QDomElement &)));
		dlgVCard->setReadOnly(false);
	}
	else
*/		connect(dlgVCard, SIGNAL(updateNickname(const QString &)), this, SLOT(slotDoRenameContact(const QString &)));
	
	dlgVCard->show();
	dlgVCard->raise();

}

/*
void JabberContact::slotEditVCard() {

	mEditingVCard = true;
	slotSnarfVCard();

}

void JabberContact::slotSaveVCard(QDomElement &vCardXML) {

	mProtocol->slotSaveVCard(vCardXML);
	mEditingVCard = false;

}
*/

QString JabberContact::id() const
{

	return userId();

}

QString JabberContact::data() const
{

	return userId();

}

void JabberContact::slotStatusChat()
{

	protocol->sendPresenceToNode(JabberProtocol::STATUS_ONLINE, userId());

}

void JabberContact::slotStatusAway()
{

	protocol->sendPresenceToNode(JabberProtocol::STATUS_AWAY, userId());

}

void JabberContact::slotStatusXA()
{

	protocol->sendPresenceToNode(JabberProtocol::STATUS_XA, userId());

}

void JabberContact::slotStatusDND()
{

	protocol->sendPresenceToNode(JabberProtocol::STATUS_DND, userId());

}

#include "jabbercontact.moc"

// vim: noet ts=4 sts=4 sw=4:
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

