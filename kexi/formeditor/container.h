/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef FORMEDITORCONTAINER_H
#define FORMEDITORCONTAINER_H

#include <qobject.h>

class QEvent;
class QWidget;

namespace KFormDesigner {
/**
 *
 * Lucijan Busch
 **/
class Container;
class WidgetLibrary;
class ObjectTreeItem;
class Form;

/**
 * this class makes a container out
 * of any QWidget
 */

class KFORMEDITOR_EXPORT Container : public QObject
{
	Q_OBJECT

	public:
		/**
		 * simply add a widget which you want to have as container
		 * this factory will care about the rest...
		 * (e.g. inserting widgets, painting the grid, ...)
		 */

		Container(Container *toplevel, QWidget *container, QObject *parent=0, const char *name=0);
		~Container();

		/**
		 * clears the dots around selected widgets and emits the new selection.
		 * you won't need to call that in most cases.<br>
		 * note: use this for a toplevel container only!
		 */
		void		setSelectionChanged(QWidget *selected);

		/**
		 * @returns a pointer to the toplevel
		 */
		Container	*toplevel();

		/**
		 * sets the object tree
		 * NOTE: this is needed if we are toplevel
		 */
		void		setObjectTree(ObjectTreeItem *t) { m_tree = t; }

		/**
		 * @returns the treenode assosiated with current container
		 */
		ObjectTreeItem	*tree();


		/**
		 * registers a sub-container and adds it to the widget tree
		 */
		void		registerChild(Container *t);

	signals:
		/**
		 * this siganl gets emmited when the mode changes<br>
		 * if e = true one can edit the form<br>
		 * if e = false one can use the form
		 */
		void		modeChanged(bool e);

	public slots:
		/**
		 * use this function to toggle between editing and viewing mode.<br>
		 * if e = true one can edit the form<br>
		 * if e = false one can use the form
		 */
		void		setEditingMode(bool e);

		/**
		 * @returns the watched widget
		 */
		QWidget		*widget() { return m_container; }

		void		setForm(Form *form);

		/**
		 * @returns the form this container belongs to
		 */
		Form		*form();

		void		deleteItem();

	protected slots:
		/**
		 * this slot uselets widgets
		 */
		void		setSelectedWidget(QWidget *selected);

		void		widgetDeleted();
		//void		updateBackground();

	signals:
		void		insertStop();

	protected:
		virtual bool	eventFilter(QObject *o, QEvent *e);

	private:
		// the watched container and it's toplevel one...
		QWidget		*m_container;
		Container 	*m_toplevel;

		// selection
		QWidget		*m_selected;

		// moving etc.
		QPoint		m_grab;
		QWidget		*m_moving;

		//inserting
		QPoint		m_insertBegin;
		QRect		m_insertRect;
		ObjectTreeItem	*m_tree;

		Form		*m_form;
};

}
#endif
