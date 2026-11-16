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

#ifndef QUICKSHELL_BAR_PROCESSOR_STATE_HPP
#define QUICKSHELL_BAR_PROCESSOR_STATE_HPP

#include <QObject>
#include <qqmlintegration.h>




struct ProcessorTimes {
    quint64 m_idleTime;
    quint64 m_totalTime;

    ProcessorTimes();
};

class ProcessorState : public QObject {
    enum class ProcessorAvailability : quint8 {
        // The initial state of every processor before
        // the first tick of the ProcessorService is Unknown,
        // except the CPU which is always online.
        // (or else we wouldn't be running...)
        Unknown = 0x00,
        // The Offline state incorporates ALL possible cores that
        // are possible but not listed in /proc/stat.
        // (https://docs.kernel.org/core-api/cpu_hotplug.html)
        Offline = 0x01,
        // The Online state incorporates ALL possible cores that are not offline,
        // and also the entire CPU.
        Online = 0x02
    };

    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ProcessorState can only be created by ProcessorService.")

    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString availability READ availability NOTIFY availabilityChanged)
    Q_PROPERTY(qreal usedPercent READ usedPercent NOTIFY usedPercentChanged)

public:
    explicit ProcessorState(const QString &name, QObject *parent = nullptr);

    [[nodiscard]] QString name() const;
    [[nodiscard]] QString availability() const;
    [[nodiscard]] qreal usedPercent() const;

    void updateOffline();
    void updateOnline(const ProcessorTimes &processorTimes);

signals:
    void availabilityChanged();
    void usedPercentChanged();

private:
    quint64 m_prevIdleTime;
    quint64 m_prevTotalTime;

    const QString m_name;
    ProcessorAvailability m_availability;
    qreal m_usedPercent;
};

#endif