 /*
    icqaddcontactpage.cpp  -  ICQ Protocol Plugin

    Copyright (c) 2002 by Stefan Gehn <metz AT gehn.net>
    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "icqaddcontactpage.h"

#include <ctype.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kpushbutton.h>

#include "icqadd.h"
#include "icqaccount.h"
#include "icqprotocol.h"
#include "icqsearchdialog.h"


ICQAddContactPage::ICQAddContactPage(ICQAccount *owner, QWidget *parent, const char *name)
	: AddContactPage(parent,name)
{
	kdDebug(14153) << k_funcinfo << "called" << endl;
	mAccount = owner;
	m_searchDialog = 0L;

	(new QVBoxLayout(this))->setAutoAdd(true);
	addUI = new icqAddUI(this);
	connect( addUI->searchButton, SIGNAL( clicked() ), this, SLOT( showSearchDialog() ) );
	
	if ( !mAccount->isConnected() )
	{
		addUI->searchButton->setEnabled( false );
		addUI->uinEdit->setReadOnly( true );
		addUI->uinEdit->setText( i18n( "%1 is offline. Unable to add contacts" ).arg( mAccount->accountLabel() ) );
	}
}

ICQAddContactPage::~ICQAddContactPage()
{
}

void ICQAddContactPage::setUINFromSearch( const QString& uin )
{
	addUI->uinEdit->setText( uin );
}

void ICQAddContactPage::showEvent(QShowEvent *e)
{
//	slotSelectionChanged();
	AddContactPage::showEvent(e);
}

bool ICQAddContactPage::apply(Kopete::Account* , Kopete::MetaContact *parentContact  )
{
	kdDebug(14153) << k_funcinfo << "called; adding contact..." << endl;

	QString contactId = addUI->uinEdit->text();
	kdDebug(14153) << k_funcinfo << "uin=" << contactId << endl;
	return mAccount->addContact(contactId, parentContact, Kopete::Account::ChangeKABC );

}

bool ICQAddContactPage::validateData()
{
	if(!mAccount->isConnected())
		return false;
	
	Q_ULONG uin = addUI->uinEdit->text().toULong();
	if ( uin < 1000 )
		return false;
	else
		return true;
}

void ICQAddContactPage::showSearchDialog()
{
	if ( m_searchDialog )
		m_searchDialog->raise();
	else
	{
		m_searchDialog = new ICQSearchDialog( mAccount, this, "icqSearchDialog" );
		m_searchDialog->show();
		connect( m_searchDialog, SIGNAL( destroyed() ), this, SLOT( searchDialogDestroyed() ) );
	}
}

void ICQAddContactPage::searchDialogDestroyed()
{
	m_searchDialog = 0L;
}


#include "icqaddcontactpage.moc"
// vim: set noet ts=4 sts=4 sw=4:
// kate: indent-mode csands; space-indent off; replace-tabs off;
