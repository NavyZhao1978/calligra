/*
 *  Copyright (c) 2010 Adam Celarek <kdedev at xibo dot at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#ifndef KIS_COLOR_SELECTOR_BASE_H
#define KIS_COLOR_SELECTOR_BASE_H

#include <QWidget>

class QColor;
class QTimer;

class KisColorSelectorBase : public QWidget
{
Q_OBJECT
public:
    explicit KisColorSelectorBase(QWidget *parent = 0);
    ~KisColorSelectorBase();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    virtual KisColorSelectorBase* createPopup() = 0;
    virtual QColor pickColorAt(int x, int y)=0;

protected slots:
    void hidePopup();

private:
    KisColorSelectorBase* m_popup;
    int m_hideDistance;
    QTimer* m_timer;

};

#endif // KIS_COLOR_SELECTOR_BASE_H
