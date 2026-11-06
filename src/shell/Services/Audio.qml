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
* - From services/Audio.qml
*   - Most of the audio service code, simplified to remove some parts that I don't need
*
****************/

pragma Singleton

import QtQuick
import Quickshell
import Quickshell.Services.Pipewire



Singleton {
    id: audioService



    property list<PwNode> sinks: []
    property list<PwNode> sources: []

    property string previousSinkName: ''
    property string previousSourceName: ''

    readonly property PwNode currentSink: Pipewire.defaultAudioSink
    readonly property PwNode currentSource: Pipewire.defaultAudioSource

    readonly property bool isCurrentSinkMuted: !!currentSink?.audio?.muted
    readonly property bool isCurrentSourceMuted: !!currentSource?.audio?.muted

    readonly property real currentSinkVolume: currentSink?.audio?.volume ?? 0.0
    readonly property real currentSourceVolume: currentSource?.audio?.volume ?? 0.0



    function setCurrentSinkVolume(volume: real): void {
        if (currentSink?.ready && currentSink?.audio) {
            currentSink.audio.muted = false
            currentSink.audio.volume = Math.max(0.0, Math.min(1.0, volume))
        }
        else {
            console.warn('[Audio] Failed to set current sink volume!')
        }
    }

    function increaseCurrentSinkVolume(volumeIncrement: real): void {
        setCurrentSinkVolume(currentSinkVolume + volumeIncrement)
    }

    function decreaseCurrentSinkVolume(volumeDecrement: real): void {
        setCurrentSinkVolume(currentSinkVolume - volumeDecrement)
    }

    function toggleCurrentSinkMuted(): void {
        if (currentSink?.ready && currentSink?.audio) {
            currentSink.audio.muted = !currentSink.audio.muted
        }
        else {
            console.warn('[Audio] Failed to toggle current sink muted!')
        }
    }

    function setCurrentSourceVolume(volume: real): void {
        if (currentSource?.ready && currentSource?.audio) {
            currentSource.audio.muted = false
            currentSource.audio.volume = Math.max(0.0, Math.min(1.0, volume))
        }
        else {
            console.warn('[Audio] Failed to set current source volume!')
        }
    }

    function increaseCurrentSourceVolume(volumeIncrement: real): void {
        setCurrentSourceVolume(currentSourceVolume + volumeIncrement)
    }

    function decreaseCurrentSourceVolume(volumeDecrement: real): void {
        setCurrentSourceVolume(currentSourceVolume - volumeDecrement)
    }

    function toggleCurrentSourceMuted(): void {
        if (currentSource?.ready && currentSource?.audio) {
            currentSource.audio.muted = !currentSource.audio.muted
        }
        else {
            console.warn('[Audio] Failed to toggle current source muted!')
        }
    }

    function refreshNodes(): void {
        const newSinks = []
        const newSources = []

        for (const node of Pipewire.nodes.values) {
            if (!node.isStream) {
                if (node.isSink) {
                    newSinks.push(node)
                }
                else if (node.audio) {
                    newSources.push(node)
                }
            }
        }

        sinks = newSinks
        sources = newSources
    }



    onCurrentSinkChanged: {
        if (!currentSink?.ready) {
            return
        }

        const newSinkName = currentSink.description || currentSink.name || 'Unknown audio device'

        if (previousSinkName && previousSinkName !== newSinkName) {
            Notifications.sendNotification('Audio output changed', `Now using: ${newSinkName}`, Notifications.UrgencyNormal)
        }

        previousSinkName = newSinkName
    }

    onCurrentSourceChanged: {
        if (!currentSource?.ready) {
            return
        }

        const newSourceName = currentSource.description || currentSource.name || 'Unknown audio device'

        if (previousSourceName && previousSourceName !== newSourceName) {
            Notifications.sendNotification('Audio input changed', `Now using: ${newSourceName}`, Notifications.UrgencyNormal)
        }

        previousSourceName = newSourceName
    }



    Component.onCompleted: {
        refreshNodes()

        previousSinkName = currentSink?.description || currentSink?.name || 'Unknown audio device'
        previousSourceName = currentSource?.description || currentSource?.name || 'Unknown audio device'
    }

    Connections {
        target: Pipewire.nodes

        function onValuesChanged(): void {
            audioService.refreshNodes()
        }
    }

    PwObjectTracker {
        objects: [
            audioService.currentSink,
            audioService.currentSource,
            ...audioService.sinks,
            ...audioService.sources
        ].filter(pwNode => pwNode)
    }
}
