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
* - From plugin/src/Caelestia/tickingservice.cpp
*   - Code for abstract ticking service
*
****************/

#include "TickingService.hpp"



TickingService::TickingService(QObject *parent, int intervalMilliseconds) :
Service(parent),
m_timer(new QTimer(this)),
m_intervalMilliseconds(intervalMilliseconds),
m_isRunning(false) {
    m_timer->setSingleShot(false);
    QObject::connect(m_timer, &QTimer::timeout, this, [this] { tick(); });
}

int TickingService::intervalMilliseconds() const {
    return m_intervalMilliseconds;
}

void TickingService::setIntervalMilliseconds(int intervalMilliseconds) {
    if (intervalMilliseconds <= 0 || intervalMilliseconds == m_intervalMilliseconds) {
        return;
    }

    m_intervalMilliseconds = intervalMilliseconds;
    if (m_isRunning) {
        m_timer->start(m_intervalMilliseconds);
    }

    emit intervalMillisecondsChanged();
}

void TickingService::start() {
    m_isRunning = true;

    if (m_intervalMilliseconds > 0) {
        m_timer->start(m_intervalMilliseconds);
    }

    tick();
}

void TickingService::stop() {
    m_isRunning = false;
    m_timer->stop();
}
