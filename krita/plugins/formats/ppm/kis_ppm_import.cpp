/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_ppm_import.h"

#include <QApplication>
#include <QFile>

#include <KGenericFactory>

#include <kio/netaccess.h>
#include <kio/deletejob.h>

#include <KoFilterChain.h>

#include <kis_debug.h>
#include <kis_doc2.h>

typedef KGenericFactory<KisPPMImport> PPMImportFactory;
K_EXPORT_COMPONENT_FACTORY(libkritappmimport, PPMImportFactory("kofficefilters"))

KisPPMImport::KisPPMImport(QObject* parent, const QStringList&) : KoFilter(parent)
{
}

KisPPMImport::~KisPPMImport()
{
}

KoFilter::ConversionStatus KisPPMImport::convert(const QByteArray& from, const QByteArray& to)
{
    dbgFile << "Importing using PPMImport!";

    if (to != "application/x-krita")
        return KoFilter::BadMimeType;

    KisDoc2 * doc = dynamic_cast<KisDoc2*>(m_chain -> outputDocument());

    if (!doc)
        return KoFilter::CreationError;

    QString filename = m_chain -> inputFile();
    doc -> prepareForImport();

    if (filename.isEmpty()) {
        return KoFilter::FileNotFound;
    }

    KUrl url;
    url.setPath(filename);


    dbgFile << "Import: " << url;
    if (url.isEmpty())
        return KoFilter::FileNotFound;

    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, qApp -> activeWindow())) {
        dbgFile << "Inexistant file";
        return KoFilter::FileNotFound;
    }

    // We're not set up to handle asynchronous loading at the moment.
    QString tmpFile;
    KoFilter::ConversionStatus result;
    if (KIO::NetAccess::download(url, tmpFile, QApplication::activeWindow())) {
        KUrl uriTF;
        uriTF.setPath(tmpFile);

        // open the file
        QFile *fp = new QFile(uriTF.path());
        if (fp->exists()) {
            result = loadFromDevice(fp);
        } else {
            result = KoFilter::CreationError;
        }

        KIO::NetAccess::removeTempFile(tmpFile);
        return result;
    }
    dbgFile << "Download failed";
    return KoFilter::CreationError;
}

KoFilter::ConversionStatus KisPPMImport::loadFromDevice(QIODevice* device)
{
    dbgFile << "Start decoding file";
    device->open(QIODevice::ReadOnly);
    if(!device->isOpen())
    {
      return KoFilter::CreationError;
    }

    QByteArray array = device->read(2);

    if (array.size() < 2) return KoFilter::CreationError;

    // Read the type of the ppm file
    enum { Puk, P1, P2, P3, P4, P5, P6 } fileType = Puk; // Puk => unknown

    if (array == "P1") {
        fileType = P1;
    } else if (array == "P2") {
        fileType = P2;
    } else if (array == "P3") {
        fileType = P3;
    } else if (array == "P4") {
        fileType = P4;
    } else if (array == "P5") {
        fileType = P5;
    } else if (array == "P6") {
        fileType = P6;
    }

    if (fileType != P6) {
        dbgFile << "Only P6 is implemented for now";
        return KoFilter::CreationError;
    }

    exit(-1);
}
