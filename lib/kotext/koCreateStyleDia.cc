/* This file is part of the KDE project
   Copyright (C)  2002 Montel Laurent <lmontel@mandrakesoft.com>

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

#include <klocale.h>

#include <qvbox.h>
#include <qlineedit.h>
#include "koCreateStyleDia.h"
#include <kmessagebox.h>

KoCreateStyleDia::KoCreateStyleDia( const QStringList & _list, QWidget *parent, const char *name )
    : KDialogBase( parent, name , true, "", Ok|Cancel, Ok, true )
{
    styleList=_list;
    setCaption( i18n("Create New Style") );
    QVBox *page = makeVBoxMainWidget();
    m_styleName = new QLineEdit( page );
    m_styleName->setFocus();
}

void KoCreateStyleDia::slotOk()
{
    if ( styleList.findIndex(m_styleName->text() ) != -1 )
    {
        KMessageBox::error(this, i18n("Name already exists! Please choose another name"));
    }
    else
        KDialogBase::slotOk();
}

QString KoCreateStyleDia::nameOfNewStyle()const
{
    return m_styleName->text();
}


#include "koCreateStyleDia.moc"
