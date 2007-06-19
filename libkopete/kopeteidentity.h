/*
    kopeteidentity.h - Kopete Identity

    Copyright (c) 2007      by Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>

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

#ifndef KOPETEIDENTITY_H
#define KOPETEIDENTITY_H

#include <kdemacros.h>
#include "kopeteglobal.h"
#include "kopeteinfopage.h"
#include "kopetepropertycontainer.h"
#include "kopeteonlinestatus.h"
#include "kopete_export.h"

class KConfigGroup;

namespace Kopete
{

class Account;

/**
 * @author Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * An identity that might contain one or more accounts associated to it 
 */
class KOPETE_EXPORT Identity : public PropertyContainer
{
	Q_OBJECT
public:	
	typedef QList<Identity*> List;
	/**
	 * @brief A container for properties. 
	 *
	 * This class provides an interface for reading and writing properties.
	 */
	Identity(const QString &id);

	~Identity();

	QString identityId() const;

	/**
	 * @brief Returns a list of user info widgets
	 *
	 * This function should be implemented in derived classes if you want to get custom 
	 * info pages to be displayed in the contact info dialog
	 */
	virtual Kopete::UI::InfoPage::List customInfoPages() const;

	/**
	 * This identity should be connected when connect all is called?
	 */
	bool excludeConnect() const;

	/**
	 * @brief Sets the online status for this identity
	 *
	 * FIXME: describe a bit more
	 */
	void setOnlineStatus( uint category, const QString &awayMessage );

	/**
	 * @brief Get the online status of the identity
	 * @return the online status of the identity
	 */
	OnlineStatus onlineStatus() const;
	
	/**
	 * \brief Get the tooltip for this identity
	 * \return an RTF tooltip depending on Kopete::AppearanceSettings settings
	 **/
	QString toolTip() const;

	/**
	 * \brief Return the icon for this identity
	 */
	QString customIcon() const;

	/**
	 * @brief Return the menu for this identity
	 *
	 * @see OnlineStatusManager::registerOnlineStatus
	 */
	KActionMenu* actionMenu();
	/**
	 * @brief Adds an account to the identity
	 *
	 * @param account the account to be added
	 */
	void addAccount( Kopete::Account *account );

	/**
	 * @brief Removes an account from the identity
	 *
	 * @param account the account to be removed
	 */
	void removeAccount( Kopete::Account *account );

	/**
	 * Returns the @ref KConfigGroup that should be used to read/write settings 
	 * of this identity
	 */
	KConfigGroup *configGroup() const;

	/**
	 * Load the identity information
	 */
	void load();

	/**
	 * Save the identity information
	 */
	void save();

public slots:
	void updateOnlineStatus();

signals:
	void onlineStatusChanged(Kopete::Identity*,Kopete::OnlineStatus,Kopete::OnlineStatus);
	void identityDestroyed( const Kopete::Identity *identity );

private:
	class Private;
	Private *d;

};

} //END namespace Kopete

#endif

