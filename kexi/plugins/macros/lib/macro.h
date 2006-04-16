/***************************************************************************
 * This file is part of the KDE project
 * copyright (C) 2005 by Sebastian Sauer (mail@dipe.org)
 * copyright (C) 2005 by Tobi Krebs (tobi.krebs@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#ifndef KOMACRO_MACRO_H
#define KOMACRO_MACRO_H

#include <qobject.h>

#include "action.h"

// Forward declarations.
class QDomElement;

namespace KoMacro {

	// Forward declarations.
	class Manager;
	class MacroItem;
	class XMLHandler;

	/**
	* The Macro class implements all the action-handling.
	*/
	class KOMACRO_EXPORT Macro : public Action
	{
			Q_OBJECT

		public:

			/**
			* Shared pointer to implement reference-counting.
			*/
			typedef KSharedPtr<Macro> Ptr;

			/**
			* A QMap of @a Macro instances accessible by there unique name. Each
			* class should use this typemap rather then the QMap direct. That
			* way we are more flexible on future changes.
			*/
			typedef QMap<QString, Macro::Ptr> Map;

			/**
			* Constructor.
			*/
			explicit Macro(const QString& name);

			/**
			* Destructor.
			*/
			virtual ~Macro();

			/**
			* @return a string-representation of the macro.
			*/
			virtual const QString toString() const;

			/**
			* @return the @a MacroItem identified with @p name
			* which is a children of this @a Macro . If there
			* exists no such @a MacroItem with that name, NULL
			* is returned.
			*/
			MacroItem* item(const QString& name) const;

			/**
			* @return a list of @a MacroItem instances which
			* are children of this @a Macro .
			*/
			QValueList<MacroItem*> items() const;

			/**
			* Add a new @a MacroItem to
			*/
			MacroItem* addItem(const QString& name);



#if 0
			/**
			* Add an @a Action instance to the list of children this
			* Macro has.
			* 
			* @param action The @a Action instance which should be added as
			* child of this @a Macro .
			*/
			void addChild(Action::Ptr action);

			/**
			* @return Returns true if there is at least one @a Action
			* instance that is a child of this @a Macro instance. If
			* that is not the case false got returned.
			*/
			bool hasChildren() const;

			/**
			 * @return the @a Action defined with \p name or NULL if
			 * this @a macro has no child with that name.
			 */
			Action::Ptr child(const QString& name) const;

			/**
			* @return Returns a list of @a Action children this Macro
			* has.
			*/
			QValueList<Action::Ptr> children() const;
#endif

			/**
			* Connect the Qt signal @p signal of the QObject @p sender
			* with this @a Macro . If the signal got emitted this
			* @a Macro instance will be activated and the in the
			* signal passed arguments are transfered into the
			* activation @a Context .
			*/
			void connectSignal(const QObject* sender, const char* signal);

			/**
			* \return the \a XMLHandler instance for this \a Macro
			* instance. This method always returns a valid
			* \a XMLHandler instance. So, it's not needed to
			* check for a NULL return-value.
			*/
			XMLHandler* xmlHandler();

		public slots:

			/**
			* Called if the @a Macro should be executed.
			*/
			virtual void activate();

			/**
			* Called if the @a Macro should be executed.
			* 
			* @param context The @a Context this @a Macro should
			* be executed in.
			*/
			virtual void activate(Context::Ptr context);

		private:
			/// @internal d-pointer class.
			class Private;
			/// @internal d-pointer instance.
			Private* const d;
	};

}

#endif
