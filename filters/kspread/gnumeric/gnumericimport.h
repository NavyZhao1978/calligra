/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

#ifndef GNUMERICFILTER_H
#define GNUMERICFILTER_H

#include <koFilter.h>

#include <qdatetime.h>
#include <qdom.h>

class KSpreadCell;
class KSpreadSheet;

class GNUMERICFilter : public KoFilter 
{
    Q_OBJECT
 public:
    GNUMERICFilter(KoFilter *parent, const char *name, const QStringList&);
    virtual ~GNUMERICFilter() {}

    virtual KoFilter::ConversionStatus convert( const QCString& from, const QCString& to );


 private:
  class GnumericDate : public QDate
  {
   public:
    static uint greg2jul( int y, int m, int d );
    static void jul2greg( double num, int & y, int & m, int & d );
    static QTime getTime( double num );

  };

  void dateInit();
  QString convertVars( QString const & str, KSpreadSheet * table ) const;
  void ParsePrintInfo( QDomNode const & printInfo, KSpreadSheet * table );
  void ParseFormat(QString const & formatString, KSpreadCell * kspread_cell);
  void setStyleInfo(QDomNode * sheet, KSpreadSheet * table);
  bool setType( KSpreadCell * kspread_cell, QString const & formatString, QString & cell_content );
  void convertFormula( QString & formula ) const;
};
#endif // GNUMERICFILTER_H
