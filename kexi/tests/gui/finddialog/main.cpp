/* This file is part of the KDE project
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>

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

#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>

#include "kexifinddialog.h"

int main(int argc, char ** argv)
{
    KAboutData aboutData("test", 0, kxi18n("KFind"), "0", KLocalizedString(), KAboutData::License_LGPL);
    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication app;

    KexiFindDialog dlg(true, 0, "dialog");

    return dlg.exec();
}