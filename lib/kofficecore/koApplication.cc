/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#include <koApplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kglobal.h>

KoApplication::KoApplication(int &argc, char **argv, 
			     const QString& rAppName)
    : OPApplication(argc, argv, rAppName),
      kded( argc, argv, opapp_orb )
{
    KGlobal::locale()->insertCatalogue("koffice");
    KGlobal::iconLoader()->insertDirectory(0, kde_datadir() + 
				     "/koffice/toolbar");
    KGlobal::iconLoader()->insertDirectory(3, localkdedir() + 
				     "/share/apps/koffice/toolbar");
}

KoApplication::~KoApplication() {}
    
#include "koApplication.moc"
