/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004  Alexander Dymo <cloudtemple@mskat.net>
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KPROPERTY_PIXMAPEDIT_H
#define KPROPERTY_PIXMAPEDIT_H

#include "widget.h"
#include <qpixmap.h>
#include <qvariant.h>

class QLabel;
class QPushButton;

namespace KoProperty {

class KOPROPERTY_EXPORT PixmapEdit : public Widget
{
	Q_OBJECT

	public:
		PixmapEdit(Property *property, QWidget *parent=0, const char *name=0);
		~PixmapEdit();

		virtual QVariant value() const;
		virtual void setValue(const QVariant &value, bool emitChange=true);
		virtual void drawViewer(QPainter *p, const QColorGroup &cg, const QRect &r, const QVariant &value);

		void resizeEvent(QResizeEvent *ev);
		bool eventFilter(QObject *o, QEvent *ev);

	protected slots:
		/*! Selects a new pixmap using "open" file dialog. 
		 Selected path will be stored in "lastVisitedImagePath" config entry within "Recent Dirs" 
		 config group of application's settings. This entry can be later reused when file dialogs 
		 are opened for selecting image files. */
		virtual void selectPixmap();

	private:
		QLabel *m_edit;
		QLabel *m_popup;
		QPushButton *m_button;
		QVariant m_recentlyPainted;
		QPixmap m_pixmap, m_scaledPixmap, m_previewPixmap;
};

}

#endif
