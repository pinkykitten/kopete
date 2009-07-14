/*  This file is part of the KDE project
    Copyright (C) 2005 Michal Vaner <michal.vaner@kdemail.net>
    Copyright (C) 2008-2009 Pali Rohár <pali.rohar@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

*/

#include "skypedetails.h"
#include "skypeaccount.h"

#include <kdebug.h>
#include <klocale.h>
#include <qlineedit.h>
#include <qcombobox.h>

SkypeDetails::SkypeDetails() : KDialog() {
	kDebug() << k_funcinfo << endl;

	setButtons( KDialog::Close ); //add only close button
	setDefaultButton( KDialog::Close );

	QWidget* w = new QWidget( this );
	dialog = new Ui::SkypeDetailsBase();//create the insides
	dialog->setupUi( w );
	setMainWidget( w );

	connect(dialog->authorCombo, SIGNAL(activated(int)), this, SLOT(changeAuthor(int)));
}


SkypeDetails::~SkypeDetails() {
	kDebug() << k_funcinfo << endl;
	delete dialog;
}

void SkypeDetails::closeEvent(QCloseEvent *) {
	kDebug() << k_funcinfo << endl;
	deleteLater();
}

void SkypeDetails::changeAuthor(int item) {
	kDebug() << k_funcinfo << endl;
	switch (item) {
		case 0:
			account->authorizeUser(dialog->idEdit->text());
			break;
		case 1:
			account->disAuthorUser(dialog->idEdit->text());
			break;
		case 2:
			account->blockUser(dialog->idEdit->text());
			break;
	}
}

SkypeDetails &SkypeDetails::setNames(const QString &id, const QString &nick, const QString &name) {
	kDebug() << k_funcinfo << endl;
	setCaption(i18n("Details for User %1", id));
	dialog->idEdit->setText(id);
	dialog->nickEdit->setText(nick);
	dialog->nameEdit->setText(name);
	return *this;
}

SkypeDetails &SkypeDetails::setPhones(const QString &priv, const QString &mobile, const QString &work) {
	kDebug() << k_funcinfo << endl;
	dialog->privatePhoneEdit->setText(priv);
	dialog->mobilePhoneEdit->setText(mobile);
	dialog->workPhoneEdit->setText(work);
	return *this;
}

SkypeDetails &SkypeDetails::setHomepage(const QString &homepage) {
	kDebug() << k_funcinfo << endl;
	dialog->homepageEdit->setText(homepage);
	return *this;
}

SkypeDetails &SkypeDetails::setAuthor(int author, SkypeAccount *account) {
	kDebug() << k_funcinfo << endl;
	dialog->authorCombo->setCurrentIndex(author);
	this->account = account;
	return *this;
}

SkypeDetails &SkypeDetails::setSex(const QString &sex) {
	kDebug() << k_funcinfo << endl;
	dialog->sexEdit->setText(sex);
	return *this;
}

#include "skypedetails.moc"