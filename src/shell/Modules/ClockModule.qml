/****************
*
* quickshell-bar - A simple desktop bar built with Quickshell that I'm developing mostly for myself :)
* Copyright (C) 2026 The caelestia-shell contributors (see caelestia-shell-contributors.txt), Andrej Wolfenhaut <andrejwolfenhaut@gmail.com>
* 
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Some code is reused from the caelestia-shell project (https://github.com/caelestia-dots/shell).
* In the following section of every source file, the relevant reused code and/or ideas is/are described.
* Otherwise, this comment ends here.
*
****************/

import Quickshell
import QtQuick
import qs.Services



Module {
    id: clockModule

    property var precision: SystemClock.Minutes

    implicitWidth: clockModuleText.implicitWidth + 2 * Config.moduleExternalSpacing

    Text {
        id: clockModuleText

        anchors.centerIn: clockModule

        text: clockModule.precision === SystemClock.Minutes ? Time.timeMinutePrecision : Time.timeSecondPrecision
    }

    MouseArea {
        id: clockModuleMouseArea

        anchors.fill: clockModule

        onClicked: {
            clockModule.precision = clockModule.precision === SystemClock.Minutes ? SystemClock.Seconds : SystemClock.Minutes
        }
    }
}
