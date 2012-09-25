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

PageStack {
    id: base;
    initialPage: createNewListPage;
    clip: true;

    signal clicked();

    Component { id: createNewListPage; Page {
        ListView {
            anchors.fill: parent;

            delegate: ListItem {
                width: ListView.view.width;

                title: model.name;
                image: model.image;
                imageShadow: false;

                gradient: Gradient {
                    GradientStop { position: 0; color: "#FAFCFD"; }
                    GradientStop { position: 0.4; color: "#F0F5FA"; }
                }

                onClicked: {
                    if (model.bnrole === "a4p") {
                        Settings.currentFile = Krita.ImageBuilder.createBlankImage(2400, 3500, 300);
                        Settings.temporaryFile = true;
                        base.clicked();
                    }
                    else if (model.bnrole === "a4l") {
                        Settings.currentFile = Krita.ImageBuilder.createBlankImage(3500, 2400, 300);
                        Settings.temporaryFile = true;
                        base.clicked();
                    }
                    else if (model.bnrole === "custom") {
                        pageStack.push( createNewPage );
                    }
                    else if (model.bnrole === "clip") {
                        Settings.currentFile = Krita.ImageBuilder.createImageFromClipboard();
                        Settings.temporaryFile = true;
                        base.clicked();
                    }
                    else if (model.bnrole === "webcam") {
                        Settings.currentFile = Krita.ImageBuilder.createImageFromWebcam();
                        Settings.temporaryFile = true;
                        base.clicked();
                    }
                }
            }

            model: ListModel {
                ListElement { bnrole: "a4p";    name: "Blank Image (A4 Portrait)"; image: "../images/svg/icon-A4portrait-green.svg" }
                ListElement { bnrole: "a4l";    name: "Blank Image (A4 Landscape)"; image: "../images/svg/icon-A4landscape-green.svg" }
                ListElement { bnrole: "custom"; name: "Custom Size"; image: "../images/svg/icon-filenew-green.svg" }
                ListElement { bnrole: "clip";   name: "From Clipboard"; image: "../images/svg/icon-fileclip-green.svg" }
                ListElement { bnrole: "webcam"; name: "From Camera"; image: "../images/svg/icon-camera-green.svg" }
            }
        }
    } }

    Component { id: createNewPage; Page {
        Column {
            anchors.fill: parent;
            Item {
                width: parent.width;
                height: Constants.GridHeight;
                Image {
                    id: titleImage;
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.margins: Constants.DefaultMargin;
                    source: "../images/svg/icon-filenew-green.svg"
                }
                Label {
                    anchors.left: titleImage.right;
                    anchors.margins: Constants.DefaultMargin;
                    anchors.verticalCenter: parent.verticalCenter;
                    text: "Custom Size";
                    font.pixelSize: Constants.LargeFontSize;
                }
            }
            TextField {
                id: width;
                placeholder: "Width";
                validator: IntValidator{bottom: 0; top: 10000;}
            }
            TextField {
                id: height;
                placeholder: "Height"
                validator: IntValidator{bottom: 0; top: 10000;}
            }
            TextField {
                id: resolution;
                placeholder: "Resolution"
                validator: IntValidator{bottom: 0; top: 600;}
            }
            Item { width: parent.width; height: Constants.GridHeight * 2; }
            Row {
                width: parent.width;
                Button {
                    width: parent.width / 2;
                    color: Constants.Theme.NegativeColor;
                    text: "Cancel";
                    textColor: "white";
                    onClicked: pageStack.pop();
                }
                Button {
                    width: parent.width / 2;
                    color: Constants.Theme.PositiveColor;
                    text: "Create";
                    textColor: "white";
                    onClicked: {
                        Settings.currentFile = Krita.ImageBuilder.createBlankImage(parseInt(width.text), parseInt(height.text), parseInt(resolution.text));
                        Settings.temporaryFile = true;
                        base.clicked();
                    }
                }
            }
        }
    } }
}
