/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KIS_PAINTOP_OPTIONS_WIDGET_H
#define KIS_PAINTOP_OPTIONS_WIDGET_H

#include <krita_export.h>
#include <kis_paintop_settings_widget.h>

class KisPaintOpOption;
class KisPropertiesConfiguration;
class KisPaintOpSettingsWidget;

/**
 * A common widget for enabling/disabling and determining
 * the effect of tablet pressure, tilt and rotation and
 * other paintop settings.
 */
class KRITAUI_EXPORT KisPaintOpOptionsWidget : public KisPaintOpSettingsWidget
{
    Q_OBJECT

public:

    KisPaintOpOptionsWidget(QWidget * parent = 0);

    ~KisPaintOpOptionsWidget();

    void addPaintOpOption(KisPaintOpOption * option);

    /// Reimplemented
    virtual void setConfiguration(const KisPropertiesConfiguration * config);

    /// Reimplemented
    virtual void writeConfiguration(KisPropertiesConfiguration *config) const;

    ///Reimplemented, sets image on option widgets
    virtual void setImage(KisImageWSP image);

    ///Reimplemented, sets node on option widgets
    virtual void setNode(KisNodeWSP node);



private Q_SLOTS:

    void changePage(const QModelIndex&);
    void lockProperties(const QModelIndex& index);
    void slotLockPropertiesDrop();
    void slotLockPropertiesSave();
    void slotEntryChecked(const QModelIndex &index);

private:
    
    struct Private;
    Private* const m_d;
    bool m_saveLockedOption;

};

#endif