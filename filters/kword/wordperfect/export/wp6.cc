/* This file is part of the KDE project
   Copyright (C) 2002 Ariya Hidayat <ariya@kde.org>

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

#include <config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <qtextcodec.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qvaluevector.h>

#include <KWEFBaseWorker.h>
#include <KWEFKWordLeader.h>
#include <KWEFUtil.h>

#include <wp6.h>

bool WPSixWorker::doOpenFile(const QString& filenameOut, const QString& /*to*/)
{
  filename = filenameOut;
  outfile.setName( filename );
  if( !outfile.open( IO_WriteOnly ) )
    return false;

  output.setDevice( &outfile );
  output.setByteOrder (QDataStream::LittleEndian);
  return true;
}

bool WPSixWorker::doCloseFile(void)
{
  // asssume we're at the end of the file
  Q_UINT32 total_filesize = outfile.at();

  // close the file first
  outfile.close();

  // reopen for read and write
  if( !outfile.open( IO_ReadWrite ) )
    return false;
  output.setDevice( &outfile );

  // now it's time to fix-up some header fields

  // fix for offset 12, int32, filesize
  outfile.at( 20 );
  output << total_filesize;

qDebug(  "Filesize %d\n", total_filesize );

  // offset 4, int32, pointer to document area
  outfile.at( 4 );
  output << document_area_ptr;

  outfile.close();
  return true;
}

bool WPSixWorker::doOpenDocument(void)
{
  // write WP document header
  // note that some fields are still "dummy"

  // magic id: -1, "WPC"
  Q_UINT8 magic[] = { 0xff, 0x57, 0x50, 0x43 };
  for( int i=0; i<4; i++ ) output << magic[i];

  // pointer to document area (dummy, will be fixed later)
  Q_UINT8 docptr[] = { 0x0E, 0x02, 0x00, 0x00 } ;
  for( int i=0; i<4; i++ ) output << docptr[i];

  // write product type ( 1 = WordPerfect )
  Q_UINT8 product_type = 1;
  output << product_type;

  // write file type ( 10 = WordPerfect document )
  Q_UINT8 file_type = 10;
  output << file_type;

  // write version (TODO explain)
  Q_UINT16 version = 0x0202;
  output << version;

  // write encryption flag ( 0 = not encrypted )
  Q_UINT16 encrypt = 0;
  output << encrypt;

  // offset to index header (always 0x200?)
  Q_UINT16 index_header_ptr = 0x200;
  output << index_header_ptr;

  // beginning of extended file header (always 5)
  Q_UINT32 extheader = 5;
  output << extheader;

  // filesize (dummy, will be fixed later)
  Q_UINT32 filesize = 0;
  output << filesize;

  // filler 488 bytes
  Q_UINT8 dummy = 0;
  for( int i=0; i<488; i++ ) output << dummy;

  // index header (specified 0 index!)
  Q_UINT8 index_header[] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0 };
  for( int i=0; i<14; i++ ) output << index_header[i];

  // document area starts, mark it for the header fields
  document_area_ptr = output.device()->at();

  return true;
}

bool WPSixWorker::doCloseDocument(void)
{
  return true;
}

// this helper functions "escape" plain Unicode string to WP-compliance string
// what it does: converting spaces and tabs to hard-spaces and hard-tabs
// TODO handle Unicode characters using WP charsets
static QCString WPSixEscape( const QString& text )
{
  QCString result;

  for( unsigned int i=0; i < text.length(); i++ )
  {
    int c = text[i].unicode();
    if( c < 32 ) result += '.'; 
    else if ( c == 32 ) result += 0x80 ; // hard space
    else if ( c < 128 ) result += text[i].latin1();
    else result += '.';
  }

  return result;
}

// puts Attr On before and Attr Off after
void WPSetAttr( QCString& buf, int attr )
{
   buf.insert( 0, 0xf2 ).insert( 0, attr ).insert( 0, 0xf2 );
   buf += 0xf3; buf += attr; buf += 0xf3;
}

bool WPSixWorker::doFullParagraph(const QString& paraText, 
  const LayoutData& layout, const ValueListFormatData& paraFormatDataList)
{
  // handle paragraph alignment/justification (default to left)
  unsigned char wp_align[] = { 0xd3, 5, 12, 0, 0, 1, 0, 0, 2, 12, 0, 0xd3 };
  if( layout.alignment == "left" ) wp_align[7] = 0;
  if( layout.alignment == "justify" ) wp_align[7] = 1;
  if( layout.alignment == "center" ) wp_align[7] = 2;
  if( layout.alignment == "right" ) wp_align[7] = 3;
  output.writeRawBytes( (const char*)wp_align, 12 );

  ValueListFormatData::ConstIterator it;
  for( it = paraFormatDataList.begin(); it!=paraFormatDataList.end(); ++it )
  {
    const FormatData& formatData = *it;

    // only if the format is for text (id==1)
    if( formatData.id == 1 )
    {
       QString partialText = paraText.mid( formatData.pos, formatData.len );

       // "escape" the string first, see WPSixEscape above
       QCString out = WPSixEscape( partialText );

       if( formatData.text.weight >= 75 ) WPSetAttr( out, 12 ); // bold
       if( formatData.text.italic ) WPSetAttr( out, 8 ); 
       if( formatData.text.underline ) WPSetAttr( out, 14 ); 
       if( formatData.text.underlineIsDouble ) WPSetAttr( out, 11 ); 
       if( formatData.text.verticalAlignment == 1 ) WPSetAttr( out, 6 ); //subscript
       if( formatData.text.verticalAlignment == 2 ) WPSetAttr( out, 5 ); //superscript
       if( formatData.text.strikeout ) WPSetAttr( out, 13 ); 

       // text foreground color
       Q_UINT8 wp_color[] = { 0xd4, 0x18, 16, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0xd4 }; 
       wp_color[7] = (Q_UINT8) formatData.text.fgColor.red();
       wp_color[8] = (Q_UINT8) formatData.text.fgColor.green();
       wp_color[9] = (Q_UINT8) formatData.text.fgColor.blue();
       output.writeRawBytes( (const char*)wp_color, 16 );

       // write out what we have already
       output.writeRawBytes( (const char*)out, out.length() );
    }

  }
    
  // write hard-return
  output << (Q_UINT8) 0xcc;
 
  return true;
}

