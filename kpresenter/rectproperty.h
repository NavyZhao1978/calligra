// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C) 2005 Thorsten Zachmann <zachmann@kde.org>

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

#ifndef RECTPROPERTY_H
#define RECTPROPERTY_H

#include <qwidget.h>

#include "kprcommand.h"

class RectPropertyUI;

class RectProperty : public QWidget
{
    Q_OBJECT
public:
    RectProperty( QWidget *parent, const char *name, RectValueCmd::RectValues &rectValue );
    ~RectProperty();

    int getRectPropertyChange() const;
    RectValueCmd::RectValues getRectValues() const;

    void apply();

protected:
    int getXRnd() const;
    int getYRnd() const;

    RectPropertyUI *m_ui;

    RectValueCmd::RectValues m_rectValue;

protected slots:
    void slotReset();
    void slotRndChanged();
};

#endif /* RECTPROPERTY_H */
