//Added by qt3to4:
#include <QTextStream>
/* This file is part of the KDE project
   Copyright (C) 2004 Nicolas GOUTTE <goutte@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef WORDS_1_3_OASIS_GENERATOR
#define WORDS_1_3_OASIS_GENERATOR

#define STRICT_OOWRITER_VERSION_1

class QString;
class QTextStream;
class KZip;
class KoStore;
class KoGenStyle;
class KoXmlWriter;
class Words13Document;

#include <KoGenStyles.h>

class Words13OasisGenerator
{
public:
    explicit Words13OasisGenerator(void);
    ~Words13OasisGenerator(void);

    bool prepare(Words13Document& kwordDocument);
    bool generate(const QString& fileName, Words13Document& kwordDocument);
protected: // Generating phase
    /**
     * @param writer output writer
     * @param frameset the frameset to generate
     * @param main true if this is the man frameset.
     */
    void generateTextFrameset(KoXmlWriter& writer, WordsTextFrameset* frameset, bool main);
    void writeStylesXml(void);
    void writeContentXml(void);
    void writeMetaXml(void);
    void writePreviewFile(void);
    void writePictures(void);
protected: // Preparing phase
    /// Preparing page layout
    void preparePageLayout(void);
    void prepareTextFrameset(WordsTextFrameset* frameset);
    /// Also sets the name of the automatical style
    void declareLayout(Words13Layout& layout);
    /// Also sets the name of the automatical style
    void declareStyle(Words13Layout& layout);
    double numberOrNull(const QString& str) const;
    double positiveNumberOrNull(const QString& str) const;
    void fillGenStyleWithFormatOne(const Words13FormatOneData& one, KoGenStyle& gs, const bool style) const;
    void fillGenStyleWithLayout(const Words13Layout& layout, KoGenStyle& gs, const bool style) const;
protected:
    Words13Document* m_kwordDocument;
    KoStore* m_store;
    QString m_contentAutomaticStyles; ///< Automatic styles for content.xml (in OO format)
    KoGenStyles m_oasisGenStyles; ///< OASIS styles
    KoXmlWriter* m_manifestWriter; ///< Manifest file writer
};

#endif // WORDS_1_3_OASIS_GENERATOR
