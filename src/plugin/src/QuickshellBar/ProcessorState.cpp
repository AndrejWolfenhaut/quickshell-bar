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
* - From plugin/src/Caelestia/Services/cpu.cpp
*   - Code for calculating the CPU usage
*
****************/

#include <iostream>

#include "ProcessorState.hpp"



using namespace Qt::StringLiterals;



ProcessorTimes::ProcessorTimes() :
m_idleTime(0ull),
m_totalTime(0ull) {}



ProcessorState::ProcessorState(QString name, QObject *parent) : QObject(parent),
m_prevIdleTime(0ull),
m_prevTotalTime(0ull),
m_name(name),
m_availability(ProcessorState::ProcessorAvailability::Unknown),
m_usedPercent(0.0) {}

QString ProcessorState::name() const {
    return m_name;
}

QString ProcessorState::availability() const {
    switch (m_availability) {
        case ProcessorState::ProcessorAvailability::Unknown:
            return u"unknown"_s;
        case ProcessorState::ProcessorAvailability::Offline:
            return u"offline"_s;
        case ProcessorState::ProcessorAvailability::Online:
            return u"online"_s;
        default:
            std::cerr
                << "[ProcessorState] Availability (0x"
                << std::hex
                << static_cast<quint8>(m_availability)
                << std::dec
                << ") is invalid!"
                << std::endl;
            return u"invalid"_s;
    }
}

qreal ProcessorState::usedPercent() const {
    return m_usedPercent;
}

void ProcessorState::updateOnline(const ProcessorTimes &processorTimes) {
    if (m_availability != ProcessorState::ProcessorAvailability::Online) {
        m_availability = ProcessorState::ProcessorAvailability::Online;
        emit availabilityChanged();
    }

    const quint64 idleTimeDelta = processorTimes.m_idleTime > m_prevIdleTime ? processorTimes.m_idleTime - m_prevIdleTime : 0ull;
    const quint64 totalTimeDelta = processorTimes.m_totalTime > m_prevTotalTime ? processorTimes.m_totalTime - m_prevTotalTime : 0ull;
    const qreal usedPercent = totalTimeDelta > 0ull ? (1.0 - static_cast<qreal>(idleTimeDelta) / static_cast<qreal>(totalTimeDelta)) * 100.0 : 0.0;

    m_prevIdleTime = processorTimes.m_idleTime;
    m_prevTotalTime = processorTimes.m_totalTime;

    if (std::abs(usedPercent - m_usedPercent) > 0.0001) {
        m_usedPercent = usedPercent;
        emit usedPercentChanged();
    }
}

void ProcessorState::updateOffline() {
    if (m_availability != ProcessorState::ProcessorAvailability::Offline) {
        m_availability = ProcessorState::ProcessorAvailability::Offline;
        emit availabilityChanged();
    }
}
