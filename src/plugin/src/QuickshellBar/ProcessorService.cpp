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
*   - Code for reading and processing /proc/stat file, with some additional inspiration from Waybar for how to handle offline CPUs:
*     https://github.com/Alexays/Waybar/blob/24a9886952297a3be27c26195b924c5bf975f260/src/modules/cpu_usage/linux.cpp
*   - Code for reading and cleaning the model name from /proc/cpuinfo
*
****************/

#include <iostream>
#include <QFile>
#include <QRegularExpression>

#include "ProcessorService.hpp"



using namespace Qt::StringLiterals;



const QString ProcessorService::sm_cpuUnknownModelName = u"Unknown"_s;

ProcessorService::ProcessorService(QObject *parent) : TickingService(parent),
m_isCPUModelNameSourced(false),
m_isPossibleCoresSourced(false),
m_lastPossibleCoreIndex(0),
m_cpuModelName(sm_cpuUnknownModelName),
m_cpuTemperatureCelsius(0.0),
m_cpuState(new ProcessorState(u"CPU"_s, this)) {
    sourceCPUModelName();
    sourcePossibleCores();
}

QString ProcessorService::cpuModelName() const {
    return m_cpuModelName;
}

ProcessorState *ProcessorService::cpuState() const {
    return m_cpuState.data();
}

QQmlListProperty<ProcessorState> ProcessorService::possibleCoreStatesByPossibleCoreIndex() {
    return { this, nullptr, &ProcessorService::possibleCoreStatesByPossibleCoreIndexCount, &ProcessorService::possibleCoreStatesByPossibleCoreIndexAt };
}

qsizetype ProcessorService::possibleCoreStatesByPossibleCoreIndexCount(QQmlListProperty<ProcessorState> *property) {
    ProcessorService *processorService = static_cast<ProcessorService *>(property->object);
    
    if (!processorService->m_isPossibleCoresSourced) {
        return 0;
    }

    return processorService->m_lastPossibleCoreIndex + 1;
}

ProcessorState *ProcessorService::possibleCoreStatesByPossibleCoreIndexAt(QQmlListProperty<ProcessorState> *property, qsizetype index) {
    ProcessorService *processorService = static_cast<ProcessorService *>(property->object);
    
    if (!processorService->m_isPossibleCoresSourced || index < 0) {
        return nullptr;
    }

    return processorService->m_possibleCoreStatesByPossibleCoreIndex.contains(index)
        ? processorService->m_possibleCoreStatesByPossibleCoreIndex.value(index).data()
        : nullptr;
}

qreal ProcessorService::cpuTemperatureCelsius() const {
    return m_cpuTemperatureCelsius;
}

void ProcessorService::tick() {
    if (!m_isCPUModelNameSourced) {
        sourceCPUModelName();
    }

    if (!m_isPossibleCoresSourced) {
        sourcePossibleCores();
    }

    sourceProcessorStates();
    sourceCPUTemperatureCelsius();
}

void ProcessorService::sourceCPUModelName() {
    QFile originFile(u"/proc/cpuinfo"_s);
    if (!originFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could not open /proc/cpuinfo! Proceeding with unknown model name..." << std::endl;
        return;
    }
    const QString originText = QString::fromLatin1(originFile.readAll());
    originFile.close();

    static const QRegularExpression cpuModelNameRegex(u"model name[\\t ]*:[\\t ]*(.+)"_s);
    const QRegularExpressionMatch cpuModelNameRegexMatch = cpuModelNameRegex.match(originText);
    if (!cpuModelNameRegexMatch.hasMatch()) {
        std::cerr << "[ProcessorService] Model name field not found in /proc/cpuinfo! Proceeding with unknown model name..." << std::endl;
        return;
    }

    QString cpuModelName = cpuModelNameRegexMatch.captured(1);

    static const QRegularExpression cpuModelNameNoiseRegex(u"\\(R\\)|\\(TM\\)|CPU|\\d+(?:st|nd|rd|th) Gen |Core |Processor"_s, QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression cpuModelNameWhitespaceRegex(u"\\s+"_s);
    cpuModelName.replace(cpuModelNameNoiseRegex, {});
    cpuModelName.replace(cpuModelNameWhitespaceRegex, u" "_s);
    cpuModelName = cpuModelName.trimmed();

    m_isCPUModelNameSourced = true;
    if (m_cpuModelName != cpuModelName) {
        m_cpuModelName = cpuModelName;
        emit cpuModelNameChanged();
    }
}

void ProcessorService::sourcePossibleCores() {
    QFile originFile(u"/sys/devices/system/cpu/possible"_s);
    if (!originFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could  not open /sys/devices/system/cpu/possible! Disabling by-core usage..." << std::endl;
        return;
    }
    const QString originText = QString::fromLatin1(originFile.readAll());
    originFile.close();

    static const QRegularExpression possibleCoresRegex(u"^\\d+-\\d+(?:,\\d+-\\d+)*$"_s);
    const QRegularExpressionMatch possibleCoresRegexMatch = possibleCoresRegex.match(originText);
    if (!possibleCoresRegexMatch.hasMatch()) {
        std::cerr << "[ProcessorService] /sys/devices/system/cpu/possible seems to be malformed! Disabling by-core usage..." << std::endl;
        return;
    }

    const QStringList possibleCoresRanges = originText.split(u","_s);
    qsizetype lastPossibleCoreIndex = 0;
    for (const QString &possibleCoresRange : std::as_const(possibleCoresRanges)) {
        const QStringList possibleCoresRangeParts = possibleCoresRange.split(u"-"_s);

        if (possibleCoresRangeParts.size() != 2) {
            std::cerr << "[ProcessorService] /sys/devices/system/cpu/possible seems to be malformed even though the regex check succeeded! Disabling by-core usage..." << std::endl;
            return;
        }

        bool possibleCoresRangeStartOK = false;
        bool possibleCoresRangeStopOK = false;
        const qsizetype possibleCoresRangeStart = possibleCoresRangeParts[0].toLongLong(&possibleCoresRangeStartOK);
        const qsizetype possibleCoresRangeStop = possibleCoresRangeParts[1].toLongLong(&possibleCoresRangeStopOK);
        if (!possibleCoresRangeStartOK || !possibleCoresRangeStopOK) {
            std::cerr << "[ProcessorService] /sys/devices/system/cpu/possible seems to be malformed even though the regex check succeeded! Disabling by-core usage..." << std::endl;
            return;
        }

        for (qsizetype possibleCoreIndex = possibleCoresRangeStart; possibleCoreIndex <= possibleCoresRangeStop; ++possibleCoreIndex) {
            QString possibleCoreName;
            QTextStream(&possibleCoreName) << u"Core "_s << possibleCoreIndex;
            m_possibleCoreStatesByPossibleCoreIndex[possibleCoreIndex] = new ProcessorState(possibleCoreName, this);
            lastPossibleCoreIndex = std::max(lastPossibleCoreIndex, possibleCoreIndex);
        }
    }
    m_isPossibleCoresSourced = true;
    m_lastPossibleCoreIndex = lastPossibleCoreIndex;
}

void ProcessorService::sourceProcessorStates() {
    QFile originFile(u"/proc/stat"_s);
    if (!originFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could not open /proc/stat! Skipping usage update..." << std::endl;
        return;
    }
    const QStringList originLines = QString::fromLatin1(originFile.readAll()).split(u"\n"_s);
    originFile.close();

    QSet<qsizetype> onlineCoreIndices;
    ProcessorTimes cpuTimes;
    QHash<qsizetype, ProcessorTimes> onlineCoreTimesByOnlineCoreIndex;

    for (const QString &originLine : std::as_const(originLines)) {
        if (!originLine.startsWith(u"cpu"_s)) {
            continue;
        }
        
        static const QRegularExpression processorTimesRegex(u"^cpu(\\d*)(?:[\\t ]+\\d+){4,10}$"_s);
        const QRegularExpressionMatch processorTimesRegexMatch = processorTimesRegex.match(originLine);
        if (!processorTimesRegexMatch.hasMatch()) {
            std::cerr << "[ProcessorService] A line of /proc/stat starting with \"cpu\" seems to be malformed! Skipping usage update..." << std::endl;
            return;
        }

        static const QRegularExpression processorTimesSplitRegex(u"[\\t ]+"_s);
        const QStringList originLineColumns = originLine.split(processorTimesSplitRegex);
        if (originLineColumns.size() < 5 || originLineColumns.size() > 11) {
            std::cerr
                << "[ProcessorService] A line of /proc/stat starting with \"cpu\" has an invalid number ("
                << originLineColumns.size()
                << ") of columns! Skipping usage update..."
                << std::endl;
            return;
        }

        // First column is at index 1, not 0!
        ProcessorTimes processorTimes = {};
        for (qsizetype originLineColumnIndex = 1; originLineColumnIndex < originLineColumns.size(); ++originLineColumnIndex) {
            bool processorTimeOK = false;
            const quint64 processorTime = originLineColumns[originLineColumnIndex].toULongLong(&processorTimeOK);
            if (!processorTimeOK) {
                std::cerr
                    << "[ProcessorService] A column ("
                    << originLineColumns[originLineColumnIndex].toStdString()
                    << ") of a line of /proc/stat starting with \"cpu\" can not be converted to an integer! Skipping usage update..."
                    << std::endl;
                return;
            }
            if (originLineColumnIndex == 4 || originLineColumnIndex == 5) {
                processorTimes.m_idleTime += processorTime;
            }
            processorTimes.m_totalTime += processorTime;
        }

        const QString onlineCoreIndexString = processorTimesRegexMatch.captured(1);
        const bool isOnlineCoreIndexSpecified = !onlineCoreIndexString.isEmpty();

        if (isOnlineCoreIndexSpecified) {
            bool onlineCoreIndexOK = false;
            const qsizetype onlineCoreIndex = onlineCoreIndexString.toLongLong(&onlineCoreIndexOK);
            if (!onlineCoreIndexOK) {
                std::cerr
                    << "[ProcessorService] A core index ("
                    << onlineCoreIndexString.toStdString()
                    << ") in a line of /proc/stat starting with \"cpu<core index>\" can not be converted to an integer! Skipping usage update..."
                    << std::endl;
                return;
            }
            onlineCoreIndices << onlineCoreIndex;
            onlineCoreTimesByOnlineCoreIndex[onlineCoreIndex] = processorTimes;
        }
        else {
            cpuTimes = processorTimes;
        }
    }

    m_cpuState->updateOnline(cpuTimes);
    emit cpuStateChanged();

    if (m_isPossibleCoresSourced) {
        for (const auto &[possibleCoreIndex, possibleCoreState] : std::as_const(m_possibleCoreStatesByPossibleCoreIndex).asKeyValueRange()) {
            if (onlineCoreIndices.contains(possibleCoreIndex)) {
                possibleCoreState->updateOnline(onlineCoreTimesByOnlineCoreIndex[possibleCoreIndex]);
            }
            else {
                possibleCoreState->updateOffline();
            }
        }
        emit possibleCoreStatesByPossibleCoreIndexChanged();
    }
}

void ProcessorService::sourceCPUTemperatureCelsius() {

}
