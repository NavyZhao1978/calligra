/* 
 * Copyright (C) 2015 by Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * Based on the Digikam CIE Tongue widget
 * Copyright (C) 2006-2013 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * Any source code are inspired from lprof project and
 * Copyright (C) 1998-2001 Marti Maria
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 **/
 
#ifndef KIS_CIETONGUEWIDGET_H
#define KIS_CIETONGUEWIDGET_H
 
// Qt includes
 
#include <QtGui/QWidget>
#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
 
// KDE includes

#include <KoColor.h>
#include <KoColorSpace.h>
  
#include <krita_export.h>
 
class KRITAUI_EXPORT KisCIETongueWidget : public QWidget
{
    Q_OBJECT
 
public:
 
    KisCIETongueWidget(QWidget *parent=0);
    ~KisCIETongueWidget();
 
    void setProfileData(QVector <double> p, QVector <double> w, bool profileData = false);
 
    void loadingStarted();
    void loadingFailed();
    void uncalibratedColor();
 
protected:
 
    int  grids(double val) const;
 
    void outlineTongue();
    void fillTongue();
    void drawTongueAxis();
    void drawTongueGrid();
    void drawLabels();
 
    QRgb colorByCoord(double x, double y);
    void drawSmallElipse(QPointF xy, int r, int g, int b, int sz);
 
    void resizeEvent(QResizeEvent* event);
    void paintEvent(QPaintEvent*);
 
private:
 
    void drawColorantTriangle();
    void drawWhitePoint();
    void drawPatches();
    void updatePixmap();
 
    void mapPoint(int& icx, int& icy, QPointF xy);
    void biasedLine(int x1, int y1, int x2, int y2);
    void biasedText(int x, int y, const QString& txt);
 
private Q_SLOTS:
 
    void slotProgressTimerDone();
 
private :
 
    class Private;
    Private* const d;
};
 
#endif /* KISCIETONGUEWIDGET_H */