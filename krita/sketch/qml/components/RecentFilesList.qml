/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

import QtQuick 1.1
import org.krita.sketch 1.0

Item {
    id: base;
    signal openClicked();

    RecentImagesModel {
        id: recentImagesModel;
        recentFileManager: RecentFileManager;
    }

    ListView {
        id: view;
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.top: parent.top;

        height: parent.height - Constants.GridHeight * 2;

        clip: true;

        delegate: ListItem {
            width: ListView.view.width;

            title: model.text;
            description: model.url;
            image: model.image;
            imageShadow: true;
            imageSmooth: false;
            imageFillMode: Image.PreserveAspectCrop;

            onClicked: {
                recentImagesModel.addRecent(model.url);
                Settings.currentFile = model.url;
            }
        }

        model: recentImagesModel;
    }

    ListItem {
        anchors.top: view.bottom;

        title: "Open Image";
        image: "../images/svg/icon-fileopen-red.svg";

        onClicked: base.openClicked();
    }
}
