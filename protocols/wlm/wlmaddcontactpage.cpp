/*
    wlmaddcontactpage.cpp - Kopete Wlm Protocol

    Copyright (c) 2008      by Tiago Salem Herrmann <tiagosh@gmail.com>
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "wlmaddcontactpage.h"

#include <QLayout>
#include <QRadioButton>
#include <QLineEdit>

#include <kdebug.h>
#include <kmessagebox.h>

#include "kopeteuiglobal.h"
#include "kopeteaccount.h"
#include "kopetemetacontact.h"

#include "wlmprotocol.h"
#include "ui_wlmaddui.h"

WlmAddContactPage::WlmAddContactPage (QWidget * parent):
AddContactPage (parent)
{
    m_wlmAddUI = new Ui::WlmAddUI ();
    m_wlmAddUI->setupUi (this);
}

WlmAddContactPage::~WlmAddContactPage ()
{
    delete m_wlmAddUI;
}

bool
WlmAddContactPage::apply (Kopete::Account *account, Kopete::MetaContact * metaContact)
{
   QString contactId = m_wlmAddUI->m_uniqueName->text();
   return account->addContact( contactId, metaContact, Kopete::Account::ChangeKABC );
}

bool
WlmAddContactPage::validateData ()
{
    QString contactId = m_wlmAddUI->m_uniqueName->text();
    if (WlmProtocol::validContactId(contactId))
        return true;

    KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
                                   i18n( "<qt>You must enter a valid WLM passport.</qt>" ), i18n( "MSN Plugin" )  );

    return false;
}


#include "wlmaddcontactpage.moc"
