#ifndef SMSSERVICE_H
#define SMSSERVICE_H

#include "kopetemessage.h"

#include <qstring.h>
#include <qwidget.h>
#include <qobject.h>

class SMSContact;
class KopeteAccount;
class QGridLayout;
class QWidget;

class SMSService : public QObject
{
	Q_OBJECT
public:
	SMSService(KopeteAccount* account = 0);
	virtual ~SMSService();

	/**
	 * Reimplement to do extra stuff when the account is dynamically changed
	 * (other than just changing m_account).
	 *
	 * Don't forget to call SMSService::setAccount(...) after you've finished.
	 */
	virtual void setAccount(KopeteAccount* account);

	/**
	 * Called when the settings widget has a place to be. @param parent is the
	 * settings widget's parent and @param layout is the 2xn grid layout it may
	 * use.
	 */
	virtual void setWidgetContainer(QWidget* parent, QGridLayout* layout) = 0;

	virtual void send(const KopeteMessage& msg) = 0;
	virtual int maxSize() = 0;
	virtual const QString& description() = 0;

public slots:
	virtual void savePreferences() = 0;

signals:
	void messageSent(const KopeteMessage &);
	void messageNotSent(const KopeteMessage &, const QString &);

protected:
	KopeteAccount* m_account;
	QGridLayout* m_layout;
	QWidget* m_parent;
};

#endif //SMSSERVICE_H
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

