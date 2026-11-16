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
* - From plugin/src/Caelestia/Services/cpu.hpp
*   - General structure of the CPU service
*
****************/

#ifndef QUICKSHELL_BAR_PROCESSOR_SERVICE_HPP
#define QUICKSHELL_BAR_PROCESSOR_SERVICE_HPP

#include <QPointer>
#include <QQmlListProperty>

#include "TickingService.hpp"
#include "ProcessorState.hpp"



/*
* Nomenclature:
* - CPU: The whole CPU, all cores in total, also the physical chip
* - Core: One logical CPU core, as is available to the Kernel
* - Processor: Refers to both of the above
*/
class ProcessorService : public TickingService {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString cpuModelName READ cpuModelName NOTIFY cpuModelNameChanged)
    Q_PROPERTY(qreal cpuTemperatureCelsius READ cpuTemperatureCelsius NOTIFY cpuTemperatureCelsiusChanged)
    Q_PROPERTY(ProcessorState *cpuState READ cpuState NOTIFY cpuStateChanged)
    Q_PROPERTY(QQmlListProperty<ProcessorState> possibleCoreStatesByPossibleCoreIndex READ possibleCoreStatesByPossibleCoreIndex NOTIFY possibleCoreStatesByPossibleCoreIndexChanged)

public:
    explicit ProcessorService(QObject *parent = nullptr);

    [[nodiscard]] QString cpuModelName() const;
    [[nodiscard]] qreal cpuTemperatureCelsius() const;
    [[nodiscard]] ProcessorState *cpuState() const;
    [[nodiscard]] QQmlListProperty<ProcessorState> possibleCoreStatesByPossibleCoreIndex();

signals:
    void cpuModelNameChanged();
    void cpuTemperatureCelsiusChanged();
    void cpuStateChanged();
    void possibleCoreStatesByPossibleCoreIndexChanged();

protected:
    void tick() override;

private:
    static const QString sm_cpuUnknownModelName;

    static qsizetype possibleCoreStatesByPossibleCoreIndexCount(QQmlListProperty<ProcessorState> *property);
    static ProcessorState *possibleCoreStatesByPossibleCoreIndexAt(QQmlListProperty<ProcessorState> *property, qsizetype index);

    bool m_isCPUModelNameSourced;
    bool m_isPossibleCoresSourced;
    qsizetype m_lastPossibleCoreIndex;

    QString m_cpuModelName;
    qreal m_cpuTemperatureCelsius;
    QPointer<ProcessorState> m_cpuState;
    QHash<qsizetype, QPointer<ProcessorState>> m_possibleCoreStatesByPossibleCoreIndex;

    void sourceCPUModelName();
    void sourceCPUTemperatureCelsius();
    void sourcePossibleCores();
    void sourceProcessorStates();
};

#endif
