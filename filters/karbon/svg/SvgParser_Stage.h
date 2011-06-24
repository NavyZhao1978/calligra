/* This file is part of the KDE project
    * Copyright (C) 2011 Aakriti Gupta <aakriti.a.gupta@gmail.com>
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

#include "SvgParser.h"
//#include <Frame.h>

class SvgParser;
class KoResourceManager;
class Frame;

class SvgParser_Stage : public SvgParser
{
public:
  
  SvgParser_Stage(KoResourceManager *documentResourceManager);
    virtual ~SvgParser_Stage();

    void parseAppData(const KoXmlElement& e);
    void setAppDataTag();
    void createAppData();
    
private:
    
    QList<Frame*> m_frameList;
    QList<QString> m_attributes;
    
    Frame * m_frame;
    QString *NS; //Namespace
  
 // Frame * frame;

};
