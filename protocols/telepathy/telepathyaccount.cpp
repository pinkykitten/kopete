/*
 * telepathyaccount.cpp
 *
 * Copyright (c) 2009 by Dariusz Mikulski <dariusz.mikulski@gmail.com>
 *
 * Kopete    (c) 2002-2006 by the Kopete developers  <kopete-devel@kde.org>
 *
 *************************************************************************
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 *************************************************************************
 */

#include "telepathyaccount.h"

// KDE includes
#include <kaction.h>
#include <kactionmenu.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenu.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kicon.h>

// Local includes
#include "telepathyprotocol.h"
#include "telepathycontact.h"
#include "telepathycontactmanager.h"
#include "telepathychatsession.h"
#include "common.h"

// Kopete includes
#include <kopetemetacontact.h>
#include <kopeteonlinestatus.h>
#include <kopetecontactlist.h>
#include <kopetechatsessionmanager.h>
#include <kopeteuiglobal.h>
#include <avatardialog.h>

#include <QtTapioca/TextChannel>

#include <TelepathyQt4/Client/Connection>
#include <TelepathyQt4/Client/AccountManager>
#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/PendingReadyConnection>
#include <TelepathyQt4/Client/PendingOperation>
#include <TelepathyQt4/Client/PendingConnection>
#include <TelepathyQt4/Client/PendingReadyAccount>
#include <TelepathyQt4/Client/PendingReadyConnectionManager>
#include <TelepathyQt4/Client/PendingReadyAccountManager>

#define SHOW_MESSAGEBOX_ERRORS

TelepathyAccount::TelepathyAccount(TelepathyProtocol *protocol, const QString &accountId)
    : Kopete::Account(protocol, accountId.toLower()),
    currentConnectionManager(0), currentAccountManager(0), account(0), existingAccountsCount(0),
    existingAccountCounter(0)
{
    kDebug(TELEPATHY_DEBUG_AREA);
    Telepathy::registerTypes();
    kDebug(TELEPATHY_DEBUG_AREA);
    setMyself( new TelepathyContact(this, accountId, Kopete::ContactList::self()->myself()) );
    connectAfterInit = false;
    setStatusAfterInit = false;
}

TelepathyAccount::~TelepathyAccount()
{
    kDebug(TELEPATHY_DEBUG_AREA);
}

bool TelepathyAccount::isOperationError(Telepathy::Client::PendingOperation* operation)
{
    if(operation->isError())
    {
        kDebug(TELEPATHY_DEBUG_AREA) << operation->errorName() << operation->errorMessage();
#ifdef SHOW_MESSAGEBOX_ERRORS
        KMessageBox::information(0, i18n("Error: %1\n%2", operation->errorName() , operation->errorMessage()));
#endif
        return true;
    }

    return false;
}

void TelepathyAccount::connect (const Kopete::OnlineStatus &initialStatus)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(!account || !account->isReady())
    {
        // \brief: initialize account if we dont have any
        initTelepathyAccount();
        connectAfterInit = true;
        statusInit = initialStatus;
        return;
    }

    kDebug(TELEPATHY_DEBUG_AREA) << account->parameters();

    // \brief: set connection early
    Telepathy::Client::PendingConnection *pc =
        currentConnectionManager->requestConnection(account->protocol(), account->parameters());

    QObject::connect(pc, SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onConnectionCreated(Telepathy::Client::PendingOperation*))
    );
}

void TelepathyAccount::onConnectionCreated(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    Telepathy::Client::PendingConnection *pc = dynamic_cast<Telepathy::Client::PendingConnection*>(operation);
    if(!pc)
    {
        kDebug(TELEPATHY_DEBUG_AREA) << "Error with connection.";
        return;
    }

    m_connection = pc->connection();

    // \brief: request connect
    QObject::connect(m_connection->requestConnect(),
        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onRequestConnectReady(Telepathy::Client::PendingOperation*))
    );
}

void TelepathyAccount::onRequestConnectReady(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;
    
    QObject::connect(m_connection->becomeReady(),
        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onConnectionReady(Telepathy::Client::PendingOperation*))
    );
}

void TelepathyAccount::onConnectionReady(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    Telepathy::SimplePresence simplePresence;
    simplePresence.type = TelepathyProtocol::protocol()->kopeteStatusToTelepathy(statusInit);
    kDebug(TELEPATHY_DEBUG_AREA) << "Requested Presence status: " << simplePresence.type;
    simplePresence.statusMessage = reasonInit.message();
    Telepathy::Client::PendingOperation *op = account->setRequestedPresence(simplePresence);
    QObject::connect(op, SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onRequestedPresence(Telepathy::Client::PendingOperation*))
    );

    // \todo: Add here connect(connection, statusChanged...)
}

void TelepathyAccount::onRequestedPresence(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    QSharedPointer<Telepathy::Client::Connection> connection = account->connection();

    Telepathy::Client::PendingOperation *op = connection->requestConnect();
    QObject::connect(op, SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onConnectionConnected(Telepathy::Client::PendingOperation*))
    );
}

void TelepathyAccount::onConnectionConnected(Telepathy::Client::PendingOperation*)
{
    kDebug(TELEPATHY_DEBUG_AREA);
}

void TelepathyAccount::disconnect ()
{
    kDebug(TELEPATHY_DEBUG_AREA);
}

void TelepathyAccount::setOnlineStatus (const Kopete::OnlineStatus &status, const Kopete::StatusMessage &reason, const OnlineStatusOptions& options)
{
    Q_UNUSED(options);
    
    kDebug(TELEPATHY_DEBUG_AREA);

    if(!account || !account->isReady())
    {
        initTelepathyAccount();
        setStatusAfterInit = true;
        statusInit = status;
        reasonInit = reason;
        return;
    }
}

void TelepathyAccount::setStatusMessage (const Kopete::StatusMessage &statusMessage)
{
    Q_UNUSED(statusMessage);
    
    kDebug(TELEPATHY_DEBUG_AREA);
}

bool TelepathyAccount::createContact (const QString &contactId, Kopete::MetaContact *parentContact)
{
    Q_UNUSED(contactId);
    Q_UNUSED(parentContact);
    
    kDebug(TELEPATHY_DEBUG_AREA);
    return false;
}

QtTapioca::TextChannel *TelepathyAccount::createTextChannel(QtTapioca::Contact *internalContact)
{
    Q_UNUSED(internalContact);
    
    kDebug(TELEPATHY_DEBUG_AREA);
    return NULL;
}

QString TelepathyAccount::connectionManager()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    return QString();
}

QString TelepathyAccount::connectionProtocol()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    return QString();
}

bool TelepathyAccount::readConfig()
{
    kDebug(TELEPATHY_DEBUG_AREA);

    // Restore config not related to ConnectionManager parameters first
    // so that the UI for the protocol parameters will be generated
    KConfigGroup *accountConfig = configGroup();
    connectionManagerName = accountConfig->readEntry( QLatin1String("ConnectionManager"), QString() );
    connectionProtocolName = accountConfig->readEntry( QLatin1String("SelectedProtocol"), QString() );

    // Clear current connection parameters
    m_allConnectionParameters.clear();
    connectionParameters.clear();

    // Get the preferences from the connection manager to get the right types
    Telepathy::Client::ProtocolInfo* protocolInfo = getProtocolInfo(getConnectionManager(), connectionProtocolName);
    if(!protocolInfo)
    {
        kDebug(TELEPATHY_DEBUG_AREA) << "Error: could not get protocol info" << connectionProtocolName;
        return false;
    }

    Telepathy::Client::ProtocolParameterList tempParameters = protocolInfo->parameters();

    // Now update the preferences
    KSharedConfig::Ptr telepathyConfig = KGlobal::config();
    QMap<QString,QString> allEntries = telepathyConfig->entryMap( TelepathyProtocol::protocol()->formatTelepathyConfigGroup(connectionManagerName, connectionProtocolName, accountId()));
    QMap<QString,QString>::ConstIterator it, itEnd = allEntries.constEnd();
    for(it = allEntries.constBegin(); it != itEnd; ++it)
    {
        foreach(Telepathy::Client::ProtocolParameter *parameter, tempParameters)
        {
            if( parameter->name() == it.key() )
            {
                kDebug(TELEPATHY_DEBUG_AREA) << parameter->defaultValue().toString() << it.value();
                if( parameter->defaultValue().toString() != it.value() )
                {
                    QVariant oldValue = parameter->defaultValue();
                    QVariant newValue(oldValue.type());
                    if ( oldValue.type() == QVariant::String )
                        newValue = QVariant(it.value());
                    else if( oldValue.type() == QVariant::Int )
                        newValue = QVariant(it.value()).toInt();
                    else if( oldValue.type() == QVariant::UInt )
                        newValue = QVariant(it.value()).toUInt();
                    else if( oldValue.type() == QVariant::Double )
                        newValue = QVariant(it.value()).toDouble();
                    else if( oldValue.type() == QVariant::Bool)
                    {
                        if( it.value().toLower() == "true")
                            newValue = true;
                        else
                            newValue = false;
                    }
                    else
                        newValue = QVariant(it.value());

                    kDebug(TELEPATHY_DEBUG_AREA) << "Name: " << parameter->name() << " Value: " << newValue << "Type: " << parameter->defaultValue().typeName();
                    connectionParameters.append(
                        new Telepathy::Client::ProtocolParameter(
                            parameter->name(),
                            parameter->dbusSignature(),
                            newValue,
                            Telepathy::ConnMgrParamFlagHasDefault
                        ));
                }
                else
                {
                    kDebug(TELEPATHY_DEBUG_AREA) << parameter->name() << parameter->defaultValue();

                    // \todo: is this right to add in list default values ???
                    connectionParameters.append(
                        new Telepathy::Client::ProtocolParameter(
                            parameter->name(),
                            parameter->dbusSignature(),
                            parameter->defaultValue(),
                            Telepathy::ConnMgrParamFlagHasDefault
                        ));
                }
            }
        }
    }

    kDebug(TELEPATHY_DEBUG_AREA) << connectionManagerName << connectionProtocolName << connectionParameters;
    if( !connectionManagerName.isEmpty() &&
        !connectionProtocolName.isEmpty() &&
        !connectionParameters.isEmpty() )
            return true;
    else
        return false;
}

Telepathy::Client::ProtocolParameterList TelepathyAccount::allConnectionParameters()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    
    if( m_allConnectionParameters.isEmpty() )
    {
        if( connectionProtocolName.isEmpty() )
            readConfig();

        Telepathy::Client::ProtocolInfo *protocolInfo = getProtocolInfo(getConnectionManager(), connectionProtocolName);
        Telepathy::Client::ProtocolParameterList allParameters = protocolInfo->parameters();
        foreach(Telepathy::Client::ProtocolParameter *parameter, allParameters)
        {
            Telepathy::Client::ProtocolParameter *newParameter = parameter;
            foreach(Telepathy::Client::ProtocolParameter *connectionParameter, connectionParameters)
            {
                // Use value from the saved connection parameter
                if( parameter->name() == connectionParameter->name() )
                {
                    newParameter = new Telepathy::Client::ProtocolParameter(
                    parameter->name(), parameter->dbusSignature(), connectionParameter->defaultValue(), Telepathy::ConnMgrParamFlagHasDefault);
                    break;
                }
             }
            m_allConnectionParameters.append( newParameter );
         }
    }

    return m_allConnectionParameters;
}

Telepathy::Client::ConnectionManager *TelepathyAccount::getConnectionManager()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    if(!currentConnectionManager)
        currentConnectionManager = new Telepathy::Client::ConnectionManager(connectionManagerName);

    return currentConnectionManager;
}

Telepathy::Client::AccountManager *TelepathyAccount::getAccountManager()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    if(!currentAccountManager)
        currentAccountManager = new Telepathy::Client::AccountManager(QDBusConnection::sessionBus());

    return currentAccountManager;
}

void TelepathyAccount::initTelepathyAccount()
{
    kDebug(TELEPATHY_DEBUG_AREA);

    // Restore config not related to ConnectionManager parameters first
    // so that the UI for the protocol parameters will be generated
    KConfigGroup *accountConfig = configGroup();
    connectionManagerName = accountConfig->readEntry( QLatin1String("ConnectionManager"), QString() );
    connectionProtocolName = accountConfig->readEntry( QLatin1String("SelectedProtocol"), QString() );

    // \brief: init managers early
    Telepathy::Client::ConnectionManager *cm = getConnectionManager();
    QObject::connect(cm->becomeReady(),
        SIGNAL(finished(Telepathy::Client::PendingOperation*)),
        this,
        SLOT(onConnectionManagerReady(Telepathy::Client::PendingOperation*))
    );
}

void TelepathyAccount::onConnectionManagerReady(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);
    
    if(isOperationError(operation))
        return;

    if(readConfig())
    {
        Telepathy::Client::AccountManager *accountManager = getAccountManager();
            QObject::connect(accountManager->becomeReady(),
            SIGNAL(finished(Telepathy::Client::PendingOperation*)),
            this,
            SLOT(onAccountManagerReady(Telepathy::Client::PendingOperation*))
        );
    }
    else
    {
        // \brief: problem with config? So here we can die.
        kDebug(TELEPATHY_DEBUG_AREA) << "Init connection manager failed!";
        delete currentConnectionManager;
        currentConnectionManager = 0;
    }
}

void TelepathyAccount::onAccountManagerReady(Telepathy::Client::PendingOperation* operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    /*
     * get a list of all the accounts that
     * are all ready there
     */
    QStringList pathList = currentAccountManager->allAccountPaths();
    kDebug(TELEPATHY_DEBUG_AREA) << "accounts: " << pathList.size() << pathList;
    existingAccountsCount = pathList.size();
    if(existingAccountsCount != 0)
        existingAccountCounter++;
    else
    {
        createNewAccount();
    }

    /*
     * check if account already exist
     */
    foreach(const QString &path, pathList)
    {
        QSharedPointer<Telepathy::Client::Account> a = currentAccountManager->accountForPath(path);
        QObject::connect(a->becomeReady(), SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this, SLOT(onExistingAccountReady(Telepathy::Client::PendingOperation *)));
    }
}

void TelepathyAccount::onExistingAccountReady(Telepathy::Client::PendingOperation *operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    Telepathy::Client::PendingReadyAccount *pa = dynamic_cast<Telepathy::Client::PendingReadyAccount *>(operation);
    if(!pa)
        return;

    Telepathy::Client::Account *a = pa->account();

    if( !account && ((a->displayName() == accountId()) && (a->protocol() == connectionProtocolName)) )
    {
        kDebug(TELEPATHY_DEBUG_AREA) << "Account already exist " << a->cmName() << connectionManagerName << a->displayName() << accountId() << a->protocol() << connectionProtocolName << existingAccountCounter;
        account = QSharedPointer<Telepathy::Client::Account>(a);

        QObject::connect(account->becomeReady(), SIGNAL(finished(Telepathy::Client::PendingOperation *)),
            this, SLOT(onAccountReady(Telepathy::Client::PendingOperation *)));
        
        return;
    }

    if( !account && (existingAccountCounter == existingAccountsCount) )
    {
        createNewAccount();
    }
    existingAccountCounter++;
}

void TelepathyAccount::createNewAccount()
{
    kDebug(TELEPATHY_DEBUG_AREA);
    
    QVariantMap parameters;
    foreach(Telepathy::Client::ProtocolParameter *parameter, connectionParameters)
    {
        kDebug(TELEPATHY_DEBUG_AREA) << parameter->name() << parameter->defaultValue().toString();
        parameters[parameter->name()] = parameter->defaultValue();
    }

    kDebug(TELEPATHY_DEBUG_AREA) << "Creating account: " << connectionManagerName << connectionProtocolName << accountId() << parameters;
    paccount = currentAccountManager->createAccount(connectionManagerName, connectionProtocolName, accountId(), parameters);

    QObject::connect(paccount, SIGNAL(finished(Telepathy::Client::PendingOperation *)),
        this, SLOT(newTelepathyAccountCreated(Telepathy::Client::PendingOperation *)));
}

void TelepathyAccount::newTelepathyAccountCreated(Telepathy::Client::PendingOperation *operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    // \brief: zeroing counter
    existingAccountCounter = 0;
    
    if(isOperationError(operation))
        return;

    account = paccount->account();

    QObject::connect(account->becomeReady(), SIGNAL(finished(Telepathy::Client::PendingOperation *)),
        this, SLOT(onAccountReady(Telepathy::Client::PendingOperation *)));
}

void TelepathyAccount::onAccountReady(Telepathy::Client::PendingOperation *operation)
{
    kDebug(TELEPATHY_DEBUG_AREA);

    if(isOperationError(operation))
        return;

    kDebug(TELEPATHY_DEBUG_AREA) << "New account: " << account->cmName() << account->protocol() << account->displayName();

    if(connectAfterInit)
    {
        connectAfterInit = false;
        connect(statusInit);
    }
    else if(setStatusAfterInit)
    {
        setStatusAfterInit = false;
        setOnlineStatus(statusInit, reasonInit);
    }
}





