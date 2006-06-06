/*
    yahoochatsession.cpp - Yahoo! Message Manager

    Copyright (c) 2005 by André Duffeck        <andre@duffeck.de>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "yahoochatsession.h"

#include <qlabel.h>
#include <qimage.h>
#include <qtooltip.h>
#include <qfile.h>
#include <qicon.h>
//Added by qt3to4:
#include <QPixmap>
#include <QList>

#include <k3widgetaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <ktempfile.h>
#include <kmainwindow.h>
#include <ktoolbar.h>
#include <krun.h>
#include <kiconloader.h>

#include "kopetecontactaction.h"
#include "kopetemetacontact.h"
#include "kopetecontactlist.h"
#include "kopetechatsessionmanager.h"
#include "kopeteuiglobal.h"
#include "kopeteglobal.h"
#include "kopeteview.h"

#include "yahoocontact.h"
#include "yahooaccount.h"

YahooChatSession::YahooChatSession( Kopete::Protocol *protocol, const Kopete::Contact *user,
	Kopete::ContactPtrList others )
: Kopete::ChatSession( user, others, protocol )
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	Kopete::ChatSessionManager::self()->registerChatSession( this );
	setInstance(protocol->instance());

	// Add Actions
	KAction *buzzAction = new KAction( KIcon("bell"), i18n( "Buzz Contact" ), actionCollection(), "yahooBuzz" ) ;
	buzzAction->setShortcut( KShortcut("Ctrl+G") );
	connect( buzzAction, SIGNAL( triggered(bool) ), this, SLOT( slotBuzzContact() ) );
	new KAction( i18n( "Send File" ), QIconSet(BarIcon("attach")), 0, this, SLOT( slotSendFile() ), actionCollection(), "yahooSendFile" );

	KAction *userInfoAction = new KAction( KIcon("idea"), i18n( "Show User Info" ), actionCollection(), "yahooShowInfo" ) ;
	connect( userInfoAction, SIGNAL( triggered(bool) ), this, SLOT( slotUserInfo() ) );

	KAction *receiveWebcamAction = new KAction( KIcon("webcamreceive"), i18n( "Request Webcam" ), actionCollection(), "yahooRequestWebcam" ) ;
	connect( receiveWebcamAction, SIGNAL( triggered(bool) ), this, SLOT( slotRequestWebcam() ) );

	KAction *sendWebcamAction = new KAction( KIcon("webcamsend"), i18n( "Invite to view your Webcam" ), actionCollection(), "yahooSendWebcam" ) ;
	connect( sendWebcamAction, SIGNAL( triggered(bool) ), this, SLOT( slotInviteWebcam() ) );

	YahooContact *c = static_cast<YahooContact*>( others.first() );
	connect( c, SIGNAL( displayPictureChanged() ), this, SLOT( slotDisplayPictureChanged() ) );
	m_image = new QLabel( 0L );
	m_image->setObjectName( QLatin1String("kde toolbar widget") );
	new K3WidgetAction( m_image, i18n( "Yahoo Display Picture" ), 0, this, SLOT( slotDisplayPictureChanged() ), actionCollection(), "yahooDisplayPicture" );
	if(c->hasProperty(Kopete::Global::Properties::self()->photo().key())  )
	{
		connect( Kopete::ChatSessionManager::self() , SIGNAL(viewActivated(KopeteView* )) , this, SLOT(slotDisplayPictureChanged()) );
	}
	else
	{
		m_image = 0L;
	}

	setXMLFile("yahoochatui.rc");
}

YahooChatSession::~YahooChatSession()
{
	delete m_image;
}

void YahooChatSession::slotBuzzContact()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QList<Kopete::Contact*>contacts = members();
	static_cast<YahooContact *>(contacts.first())->buzzContact();
}

void YahooChatSession::slotUserInfo()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QList<Kopete::Contact*>contacts = members();
	static_cast<YahooContact *>(contacts.first())->slotUserInfo();
}

void YahooChatSession::slotRequestWebcam()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QList<Kopete::Contact*>contacts = members();
	static_cast<YahooContact *>(contacts.first())->requestWebcam();
}

void YahooChatSession::slotInviteWebcam()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QList<Kopete::Contact*>contacts = members();
	static_cast<YahooContact *>(contacts.first())->inviteWebcam();
}

void YahooChatSession::slotSendFile()
{
	kdDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QPtrList<Kopete::Contact>contacts = members();
	static_cast<YahooContact *>(contacts.first())->sendFile();
}

void YahooChatSession::slotDisplayPictureChanged()
{
	kDebug(YAHOO_GEN_DEBUG) << k_funcinfo << endl;
	QList<Kopete::Contact*> mb=members();
	YahooContact *c = static_cast<YahooContact *>( mb.first() );
	if ( c && m_image )
	{
		if(c->hasProperty(Kopete::Global::Properties::self()->photo().key()))
		{
#warning Port or remove this KToolBar hack
#if 0
			int sz=22;
			// get the size of the toolbar were the aciton is plugged.
			//  if you know a better way to get the toolbar, let me know
			KMainWindow *w= view(false) ? dynamic_cast<KMainWindow*>( view(false)->mainWidget()->topLevelWidget() ) : 0L;
			if(w)
			{
				//We connected that in the constructor.  we don't need to keep this slot active.
				disconnect( Kopete::ChatSessionManager::self() , SIGNAL(viewActivated(KopeteView* )) , this, SLOT(slotDisplayPictureChanged()) );

				KAction *imgAction=actionCollection()->action("yahooDisplayPicture");
				if(imgAction)
				{
					QList<KToolBar*> toolbarList = w->toolBarList();
					QList<KToolBar*>::Iterator it, itEnd = toolbarList.end();
					for(it = toolbarList.begin(); it != itEnd; ++it)
					{
						KToolBar *tb=*it;
						if(imgAction->isPlugged(tb))
						{
							sz=tb->iconSize();
							//ipdate if the size of the toolbar change.
							disconnect(tb, SIGNAL(modechange()), this, SLOT(slotDisplayPictureChanged()));
							connect(tb, SIGNAL(modechange()), this, SLOT(slotDisplayPictureChanged()));
							break;
						}
						++it;
					}
				}
			}

			QString imgURL=c->property(Kopete::Global::Properties::self()->photo()).value().toString();
			QImage scaledImg = QPixmap( imgURL ).toImage().smoothScale( sz, sz );
			if(!scaledImg.isNull())
				m_image->setPixmap( QPixmap(scaledImg) );
			else
			{ //the image has maybe not been transfered correctly..  force to download again
				c->removeProperty(Kopete::Global::Properties::self()->photo());
				//slotDisplayPictureChanged(); //don't do that or we might end in a infinite loop
			}
			QToolTip::add( m_image, "<qt><img src=\"" + imgURL + "\"></qt>" );
#endif
		}
	}
}

#include "yahoochatsession.moc"
