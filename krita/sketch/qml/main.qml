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
import "components"

Item {
    id: base;
    width: 1280;
    height: 768;

    Flickable {
        id: screenScroller;
        boundsBehavior: Flickable.StopAtBounds;

        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
            bottom: keyboard.top;
        }

        contentWidth: base.width;
        contentHeight: base.height;

        PageStack {
            width: base.width;
            height: base.height;

            initialPage: welcomePage;

            transitionDuration: 500;

            Component { id: welcomePage; WelcomePage { } }

            MouseArea {
                anchors.fill: parent;
                onClicked: parent.focus = true;
            }
        }

        function ensureVisible(item) {
            if(item !== undefined && item !== null) {
                var targetPosition = item.mapToItem(screenScroller, item.x, item.y);
                if(targetPosition.y > base.height * 0.66) {
                    screenScroller.contentY = targetPosition.y - base.height / 2;
                    screenScroller.returnToBounds();
                }
            }
        }
    }

    VirtualKeyboard {
        id: keyboard;
        onKeyboardVisibleChanged: if(keyboardVisible) screenScroller.ensureVisible(Settings.focusItem);
    }

    Connections {
        target: Settings;

        onFocusItemChanged: if(keyboard.keyboardVisible) screenScroller.ensureVisible(Settings.focusItem);
    }
}