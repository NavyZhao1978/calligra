/*
 *  Copyright (c) 2013 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KOVIEWMANAGERFACTORY_H
#define KOVIEWMANAGERFACTORY_H

#include "komvc_export.h"
#include <QObject>

/**
 * KoViewManagerFactory is the base class for KoViewManagerBase
 * plugins.
 */
class KOMVC_EXPORT KoViewManagerFactory : public QObject
{
    Q_OBJECT
public:
    explicit KoViewManagerFactory(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif 
