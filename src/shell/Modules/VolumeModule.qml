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

import QtQuick
import qs.Services



Module {
    id: volumeModule

    implicitWidth: (
        Config.moduleExternalSpacing +
        volumeModuleSinkText.implicitWidth +
        Config.moduleInternalSpacing +
        volumeModuleSourceText.implicitWidth +
        Config.moduleExternalSpacing
    )

    readonly property var sinkVolumeThresholds: { return { silent: 0.0, quiet: 0.25, medium: 0.75, loud: 1.0 } }
    readonly property var sinkVolumeIcons: { return { muted: '', silent: '', quiet: '', medium: '', loud: '' } }
    readonly property string sinkVolumeIcon: getVolumeIcon(sinkVolumeThresholds, sinkVolumeIcons, Audio.isCurrentSinkMuted, Audio.currentSinkVolume)

    readonly property var sourceVolumeThresholds: { return { silent: 0.0, quiet: 0.25, medium: 0.75, loud: 1.0 } }
    readonly property var sourceVolumeIcons: { return { muted: '', silent: '', quiet: '', medium: '', loud: '' } }
    readonly property string sourceVolumeIcon: getVolumeIcon(sourceVolumeThresholds, sourceVolumeIcons, Audio.isCurrentSourceMuted, Audio.currentSourceVolume)



    function getVolumeIcon(volumeThresholds: var, volumeIcons: var, isMuted: bool, volume: real): string {
        if (isMuted) {
            return volumeIcons.muted
        }
        else if (volume <= volumeThresholds.silent) {
            return volumeIcons.silent
        }
        else if (volume <= volumeThresholds.quiet) {
            return volumeIcons.quiet
        }
        else if (volume <= volumeThresholds.medium) {
            return volumeIcons.medium
        }
        else if (volume <= volumeThresholds.loud) {
            return volumeIcons.loud
        }
        else {
            console.warn(`[VolumeModule] Volume ${volume} is louder than the loud volume threshold! Defaulting to loud volume icon...`)

            return volumeIcons.loud
        }
    }



    Text {
        id: volumeModuleSinkText

        anchors.left: volumeModule.left
        anchors.leftMargin: Config.moduleExternalSpacing
        anchors.verticalCenter: volumeModuleSourceText.verticalCenter

        text: `${Math.round(100.0 * Audio.currentSinkVolume)}% ${volumeModule.sinkVolumeIcon}`
    }

    MouseArea {
        anchors.left: volumeModule.left
        anchors.right: volumeModule.horizontalCenter
        anchors.top: volumeModule.top
        anchors.bottom: volumeModule.bottom
    
        onWheel: wheel => {
            if (wheel.angleDelta.y > 0.0) {
                Audio.increaseCurrentSinkVolume(0.01)
            }
            else if (wheel.angleDelta.y < 0.0) {
                Audio.decreaseCurrentSinkVolume(0.01)
            }
        }

        onClicked: Audio.toggleCurrentSinkMuted()
    }

    Text {
        id: volumeModuleSourceText

        anchors.right: volumeModule.right
        anchors.rightMargin: Config.moduleExternalSpacing
        anchors.verticalCenter: volumeModule.verticalCenter

        text: `${Math.round(100.0 * Audio.currentSourceVolume)}% ${volumeModule.sourceVolumeIcon}`
    }

    MouseArea {
        anchors.left: volumeModule.horizontalCenter
        anchors.right: volumeModule.right
        anchors.top: volumeModule.top
        anchors.bottom: volumeModule.bottom
    
        onWheel: wheel => {
            if (wheel.angleDelta.y > 0.0) {
                Audio.increaseCurrentSourceVolume(0.01)
            }
            else if (wheel.angleDelta.y < 0.0) {
                Audio.decreaseCurrentSourceVolume(0.01)
            }
        }

        onClicked: Audio.toggleCurrentSourceMuted()
    }
}
