/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
 * Copyright (C) Sven Langkamp <sven.langkamp@gmail.com>, (C) 2009
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
#include "kis_paintop_preset.h"

#include <QFile>
#include <QSize>
#include <QImage>
#include <QDomDocument>
#include <QBuffer>

#include <KoColorSpaceRegistry.h>
#include <KoInputDevice.h>

#include "kis_types.h"
#include "kis_paintop_settings.h"
#include "kis_paintop_registry.h"
#include "kis_painter.h"
#include "kis_paint_information.h"
#include "kis_paint_device.h"
#include "kis_image.h"

struct KisPaintOpPreset::Private {
    KisPaintOpSettingsSP settings;
    QImage image;
};


KisPaintOpPreset::KisPaintOpPreset()
        : KoResource(QString())
        , m_d(new Private)
{
    m_d->settings = 0;
}

KisPaintOpPreset::KisPaintOpPreset(const QString & fileName)
        : KoResource(fileName)
        , m_d(new Private)
{
}

KisPaintOpPreset::~KisPaintOpPreset()
{
    delete m_d;
}

KisPaintOpPreset* KisPaintOpPreset::clone() const
{
    KisPaintOpPreset * preset = new KisPaintOpPreset();
    if (settings()) {
        preset->setSettings(settings()->clone());
    }
    preset->setPaintOp(paintOp());
    preset->setName(name());
    preset->setValid(valid());

    return preset;
}

void KisPaintOpPreset::setPaintOp(const KoID & paintOp)
{
    m_d->settings->setProperty("paintop", paintOp.id());
}

KoID KisPaintOpPreset::paintOp() const
{
    return KoID(m_d->settings->getString("paintop"), name());
}

void KisPaintOpPreset::setSettings(KisPaintOpSettingsSP settings)
{
    Q_ASSERT(!settings->getString("paintop", "").isEmpty());
    m_d->settings = settings->clone();
    setValid(true);
}

KisPaintOpSettingsSP KisPaintOpPreset::settings() const
{
    Q_ASSERT(!m_d->settings->getString("paintop", "").isEmpty());

    return m_d->settings;
}

bool KisPaintOpPreset::load()
{
    if (filename().isEmpty())
        return false;

    QFile f(filename());

    QDomDocument doc;
    if (!f.open(QIODevice::ReadOnly))
        return false;
    if (!doc.setContent(&f)) {
        f.close();
        return false;
    }
    f.close();

    QDomElement element = doc.documentElement();
    fromXML(element);
    
        
    QDomElement preview = element.firstChildElement("PreviewImage");
    if(!preview.isNull()) {
         QByteArray data = QByteArray::fromHex(preview.text().toAscii());
         m_d->image = QImage::fromData(data, "PPM");
    }
    
    if(!m_d->settings)
        return false;
        
    return true;
}

bool KisPaintOpPreset::save()
{
    if (filename().isEmpty())
        return false;

    QFile f(filename());
    f.open(QIODevice::WriteOnly);
    
    QString paintopid = m_d->settings->getString("paintop", "");
    if (paintopid.isEmpty())
        return false;

    QDomDocument doc;
    QDomElement root = doc.createElement("Preset");
    toXML(doc, root);
    
    if(!m_d->image.isNull()) {
        QDomElement preview = doc.createElement("PreviewImage");
        preview.setAttribute("width", m_d->image.width());
        preview.setAttribute("height", m_d->image.height());
        
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        m_d->image.save(&buffer, "PPM");
        
        preview.appendChild(doc.createCDATASection(ba.toHex()));
        root.appendChild(preview);
    }
    
    doc.appendChild(root);
    
    QTextStream textStream(&f);
    doc.save(textStream, 4);
    f.close();
    return true;
}

void KisPaintOpPreset::toXML(QDomDocument& doc, QDomElement& elt) const
{
    QString paintopid = m_d->settings->getString("paintop", "");

    elt.setAttribute("paintopid", paintopid);
    elt.setAttribute("name", name());

    m_d->settings->toXML(doc, elt);
}

void KisPaintOpPreset::fromXML(const QDomElement& presetElt)
{
    setName(presetElt.attribute("name"));
    QString paintopid = presetElt.attribute("paintopid");

    if (paintopid.isEmpty())
    {
        dbgImage << "No paintopid attribute";
        return;
    }

    KoID id(paintopid, "");

    KisPaintOpSettingsSP settings = KisPaintOpRegistry::instance()->settings(id, 0);
    if (!settings)
    {
        dbgImage << "No settings for " << paintopid;
        return;
    }
    settings->fromXML(presetElt);
    setSettings(settings);
}

QImage KisPaintOpPreset::image() const
{
    return m_d->image;
}

void KisPaintOpPreset::setImage(QImage image)
{
    m_d->image = image;
}

