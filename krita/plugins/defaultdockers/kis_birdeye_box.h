/*
 *  Copyright (c) 2004 Kivio Team
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_BIRDEYE_BOX_H
#define KIS_BIRDEYE_BOX_H

#include "qwidget.h"

class KoBirdEyePanel;
class KisDoubleWidget;
class KisView;


class KisBirdEyeBox : public QWidget
{ 
    Q_OBJECT

public:

    KisBirdEyeBox(KisView * view, QWidget * parent = 0, const char* name=0);
    ~KisBirdEyeBox();

protected slots:

    void exposureValueChanged(double exposure);
    void exposureSliderPressed();
    void exposureSliderReleased();

signals:
    void exposureChanged(float exposure);

private:
    KoBirdEyePanel * m_birdEyePanel;
    KisDoubleWidget * m_exposureDoubleWidget;
    
    bool m_draggingExposureSlider;
};

#endif // KIS_BIRDEYE_BOX_H
