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

pragma Singleton

import Quickshell



Singleton {
    enum Urgency { UrgencyLow = 0, UrgencyNormal = 1, UrgencyCritical = 2 }

    function sendNotification(summary: string, body: string, urgency: int) {
        let urgencyString = undefined

        switch (urgency) {
            case Notifications.UrgencyLow:
                urgencyString = 'low'
                break
            case Notifications.UrgencyNormal:
                urgencyString = 'normal'
                break
            case Notifications.UrgencyCritical:
                urgencyString = 'critical'
                break
            default:
                break
        }

        urgencyString = urgencyString || 'normal'

        if (!summary && !body) {
            return
        }
        else if (!summary) {
            Quickshell.execDetached(['notify-send', `--urgency=${urgencyString}`, '', body])
        }
        else if (!body) {
            Quickshell.execDetached(['notify-send', `--urgency=${urgencyString}`, summary])
        }
        else {
            Quickshell.execDetached(['notify-send', `--urgency=${urgencyString}`, summary, body])
        }
    }
}
