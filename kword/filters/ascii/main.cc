/******************************************************************/
/* KWord - (c) by Reginald Stadlbauer and Torben Weis 1997-1998   */
/* Version: 0.0.1                                                 */
/* Author: Reginald Stadlbauer, Torben Weis                       */
/* E-Mail: reggie@kde.org, weis@kde.org                           */
/* Homepage: http://boch35.kfunigraz.ac.at/~rs                    */
/* needs c++ library Qt (http://www.troll.no)                     */
/* written for KDE (http://www.kde.org)                           */
/* needs mico (http://diamant.vsb.cs.uni-frankfurt.de/~mico/)     */
/* needs OpenParts and Kom (weis@kde.org)                         */
/* License: GNU GPL                                               */
/******************************************************************/
/* Module: Filter                                                 */
/******************************************************************/

#include "main.h"
#include "main.moc"

#include <iostream>
#include <stdio.h>
#include <assert.h>

#include <komAutoLoader.h>
#include <kom.h>

#include <qmsgbox.h>
#include <qstring.h>

#include <komlMime.h>
#include <koStream.h>
#include <komlParser.h>
#include <komlStreamFeed.h>
#include <komlWriter.h>

#include <strstream.h>
#include <string.h>

typedef KOMAutoLoader<Factory> MyAutoLoader;

/******************************************************************/
/* MyApplication                                                  */
/******************************************************************/

/*================================================================*/
MyApplication::MyApplication(int &argc,char **argv) :
    KoApplication(argc,argv,"kword_ascii")
{
}

/*================================================================*/
void MyApplication::start()
{
}

/******************************************************************/
/* Factory                                                        */
/******************************************************************/

/*================================================================*/
Factory::Factory(const CORBA::ORB::ObjectTag &_tag)
    : KOffice::FilterFactory_skel(_tag)
{
}

/*================================================================*/
Factory::Factory(CORBA::Object_ptr _obj)
    : KOffice::FilterFactory_skel(_obj)
{
}

/******************************************************************/
/* Filter                                                         */
/******************************************************************/

/*================================================================*/
KOffice::Filter_ptr Factory::create()
{
    Filter *f = new Filter;
    KOMShutdownManager::self()->watchObject( f );
    return KOffice::Filter::_duplicate( f );
}

/*================================================================*/
Filter::Filter()
    : KOMComponent(), KOffice::Filter_skel()
{
}

/*================================================================*/
void Filter::filter(KOffice::Filter::Data& data,const QCString &from,const QCString &to)
{
    if ( to == "application/x-kword" &&
         from == "text/plain" ) 
    {
        if (data.size() == 0)
            return;

        QString str;

        str += "<?xml version=\"1.0\"?>\n";
        str += "<DOC  author=\"Reginald Stadlbauer and Torben Weis\" email=\"reggie@kde.org and weis@kde.org\" editor=\"KWord\" mime=\"application/x-kword\">\n";
        str += "<PAPER format=\"1\" ptWidth=\"595\" ptHeight=\"841\" mmWidth =\"210\" mmHeight=\"297\" inchWidth =\"8.26772\" inchHeight=\"11.6929\" orientation=\"0\" columns=\"1\" ptColumnspc=\"2\" mmColumnspc=\"1\" inchColumnspc=\"0.0393701\" hType=\"0\" fType=\"0\" ptHeadBody=\"9\" ptFootBody=\"9\" mmHeadBody=\"3.5\" mmFootBody=\"3.5\" inchHeadBody=\"0.137795\" inchFootBody=\"0.137795\">\n";
        str += "<PAPERBORDERS mmLeft=\"10\" mmTop=\"15\" mmRight=\"10\" mmBottom=\"15\" ptLeft=\"28\" ptTop=\"42\" ptRight=\"28\" ptBottom=\"42\" inchLeft=\"0.393701\" inchTop=\"0.590551\" inchRight=\"0.393701\" inchBottom=\"0.590551\"/>\n";
        str += "</PAPER>\n";
        str += "<ATTRIBUTES processing=\"0\" standardpage=\"1\" hasHeader=\"0\" hasFooter=\"0\" unit=\"mm\"/>\n";
        str += "<FRAMESETS>\n";
        str += "<FRAMESET frameType=\"1\" autoCreateNewFrame=\"1\" frameInfo=\"0\" removeable=\"0\">\n";
        str += "<FRAME left=\"28\" top=\"42\" right=\"566\" bottom=\"798\" runaround=\"1\" runaGapPT=\"2\" runaGapMM=\"1\" runaGapINCH=\"0.0393701\"  lWidth=\"1\" lRed=\"255\" lGreen=\"255\" lBlue=\"255\" lStyle=\"0\"  rWidth=\"1\" rRed=\"255\" rGreen=\"255\" rBlue=\"255\" rStyle=\"0\"  tWidth=\"1\" tRed=\"255\" tGreen=\"255\" tBlue=\"255\" tStyle=\"0\"  bWidth=\"1\" bRed=\"255\" bGreen=\"255\" bBlue=\"255\" bStyle=\"0\" bkRed=\"255\" bkGreen=\"255\" bkBlue=\"255\" bleftpt=\"0\" bleftmm=\"0\" bleftinch=\"0\" brightpt=\"0\" brightmm=\"0\" brightinch=\"0\" btoppt=\"0\" btopmm=\"0\" btopinch=\"0\" bbottompt=\"0\" bbottommm=\"0\" bbottominch=\"0\"/>\n";



        str += "<PARAGRAPH>\n";
        str += "<TEXT>";

        char * buffer = data.data();
        for ( int i = 0 ;i < data.size() ; ++i )
        {
            QChar c = buffer[ i ];
            if ( c == QChar( '\n' ) )
            {
                str += "</TEXT>\n";
                str += "</PARAGRAPH>\n";
                str += "<PARAGRAPH>\n";
                str += "<TEXT>";
            }
            else if ( c == QChar( '<' ) )
                str += "&lt;";
            else if ( c == QChar( '>' ) )
                str += "&gt;";
            else
                str += c;
        }

        str += "</TEXT>\n";
        str += "</PARAGRAPH>\n";

        str += "</FRAMESET>\n";
        str += "</FRAMESETS>\n";
        str += "</DOC>\n";

        QCString cstr=QCString(str.utf8());
        char * ret = strdup( cstr.data() );
        data.assign( ret, cstr.length() );

    }
    else if ( from == "application/x-kword" &&
              to == "text/plain") 
    {
        QString str;
        QCString buf ( data.data() );

        int i = buf.find( "<TEXT>" );
        while ( i != -1 )
        {
            int j = buf.find( "</TEXT>", i );
            if ( j - ( i + 6 ) > 0 )
            {
                str += buf.mid( i + 6, j - ( i + 6 ) );
                str += "\n";
            }
            i = buf.find( "<TEXT>", j );
        }
        
        QCString cstr=QCString(str.utf8());
        char * ret = strdup( cstr.data() );
        data.assign( ret, cstr.length() );
    }
    else
    {		
        KOffice::Filter::UnsupportedFormat exc;
        exc.format = to;
        mico_throw(exc);
        KOffice::Filter::UnsupportedFormat exc2;
        exc2.format = from;
        mico_throw(exc2);
        return;
    }
}


/*================================================================*/
int main(int argc,char **argv)
{
    MyApplication app(argc,argv);

    MyAutoLoader loader("IDL:KOffice/FilterFactory:1.0","KWordAscii");

    app.exec();
}



