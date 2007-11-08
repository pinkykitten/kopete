/*
 * xmpp_features.h
 * Copyright (C) 2003  Justin Karneges
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef XMPP_FEATURES_H
#define XMPP_FEATURES_H

#include <QStringList>


#include <iris_export.h>

namespace XMPP
{
	class IRIS_EXPORT Features
	{
	public:
		class FeatureName;
		Features();
		Features(const QStringList &);
		Features(const QString &);
		~Features();

		QStringList list() const; // actual featurelist
		void setList(const QStringList &);
		void addFeature(const QString&);

		// features
		bool canRegister() const;
		bool canSearch() const;
		bool canMulticast() const;
		bool canGroupchat() const;
		bool canVoice() const;
		bool canDisco() const;
		bool canChatState() const;
		bool canCommand() const;
		bool canXHTML() const;
		bool isGateway() const;
		bool haveVCard() const;

		enum FeatureID {
			FID_Invalid = -1,
			FID_None,
			FID_Register,
			FID_Search,
			FID_Groupchat,
			FID_Disco,
			FID_Gateway,
			FID_VCard,
			FID_AHCommand,
 			FID_Xhtml,

			// private Psi actions
			FID_Add
		};

		// useful functions
		bool test(const QStringList &) const;

		QString name() const;
		static QString name(long id);
		static QString name(const QString &feature);

		long id() const;
		static long id(const QString &feature);
		static QString feature(long id);

		private:
		QStringList _list;
	};
}

#endif
