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



const QString ProcessorService::sm_unknownModelName = u"Unknown"_s;

ProcessorService::ProcessorService(QObject *parent) : TickingService(parent),
m_isModelNameSourced(false),
m_isPossibleCoresSourced(false),
m_lastPossibleCoreIndex(0),
m_modelName(sm_unknownModelName),
m_stateTotal(new ProcessorState(u"CPU"_s, this)),
m_temperatureCelsius(0.0) {
    sourceModelName();
    sourcePossibleCores();
}

QString ProcessorService::modelName() const {
    return m_modelName;
}

ProcessorState *ProcessorService::stateTotal() const {
    return m_stateTotal.data();
}

QQmlListProperty<ProcessorState> ProcessorService::statesByPossibleCore() {
    return { this, nullptr, &ProcessorService::statesByPossibleCoreCount, &ProcessorService::statesByPossibleCoreAt };
}

qsizetype ProcessorService::statesByPossibleCoreCount(QQmlListProperty<ProcessorState> *property) {
    ProcessorService *processorService = static_cast<ProcessorService *>(property->object);
    
    if (!processorService->m_isPossibleCoresSourced) {
        return 0;
    }

    return processorService->m_lastPossibleCoreIndex + 1;
}

ProcessorState *ProcessorService::statesByPossibleCoreAt(QQmlListProperty<ProcessorState> *property, qsizetype index) {
    ProcessorService *processorService = static_cast<ProcessorService *>(property->object);
    
    if (!processorService->m_isPossibleCoresSourced || index < 0) {
        return nullptr;
    }

    return processorService->m_statesByPossibleCore.contains(index)
        ? processorService->m_statesByPossibleCore.value(index).data()
        : nullptr;
}

qreal ProcessorService::temperatureCelsius() const {
    return m_temperatureCelsius;
}

void ProcessorService::tick() {
    if (!m_isModelNameSourced) {
        sourceModelName();
    }

    if (!m_isPossibleCoresSourced) {
        sourcePossibleCores();
    }

    sourceUsage();
    sourceTemperatureCelsius();
}

void ProcessorService::sourceModelName() {
    QFile procCPUInfoFile(u"/proc/cpuinfo"_s);
    if (!procCPUInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could not open /proc/cpuinfo! Proceeding with unknown model name..." << std::endl;
        return;
    }
    const QByteArray procCPUInfoData = procCPUInfoFile.readAll();
    procCPUInfoFile.close();

    static const QRegularExpression modelNameRegex(u"model name[\\t ]*:[\\t ]*(.+)"_s);
    const QRegularExpressionMatch modelNameRegexMatch = modelNameRegex.match(QString::fromLatin1(procCPUInfoData));
    if (!modelNameRegexMatch.hasMatch()) {
        std::cerr << "[ProcessorService] Model name field not found in /proc/cpuinfo! Proceeding with unknown model name..." << std::endl;
        return;
    }

    QString modelName = modelNameRegexMatch.captured(1);

    static const QRegularExpression modelNameNoiseRegex(u"\\(R\\)|\\(TM\\)|CPU|\\d+(?:st|nd|rd|th) Gen |Core |Processor"_s, QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression modelNameWhitespaceRegex(u"\\s+"_s);
    modelName.replace(modelNameNoiseRegex, {});
    modelName.replace(modelNameWhitespaceRegex, u" "_s);
    modelName = modelName.trimmed();

    m_isModelNameSourced = true;
    if (m_modelName != modelName) {
        m_modelName = modelName;
        emit modelNameChanged();
    }
}

void ProcessorService::sourcePossibleCores() {
    QFile possibleCoresFile(u"/sys/devices/system/cpu/possible"_s);
    if (!possibleCoresFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could  not open /sys/devices/system/cpu/possible! Disabling by-core usage..." << std::endl;
        return;
    }
    const QByteArray possibleCoresData = possibleCoresFile.readAll();
    possibleCoresFile.close();

    const QString possibleCoresText = QString::fromLatin1(possibleCoresData);
    static const QRegularExpression possibleCoresRegex(u"^\\d+-\\d+(?:,\\d+-\\d+)*$"_s);
    const QRegularExpressionMatch possibleCoresRegexMatch = possibleCoresRegex.match(possibleCoresText);
    if (!possibleCoresRegexMatch.hasMatch()) {
        std::cerr << "[ProcessorService] /sys/devices/system/cpu/possible seems to be malformed! Disabling by-core usage..." << std::endl;
        return;
    }

    const QStringList possibleCoresRanges = possibleCoresText.split(u","_s);
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
            m_statesByPossibleCore[possibleCoreIndex] = new ProcessorState(possibleCoreName, this);
            lastPossibleCoreIndex = std::max(lastPossibleCoreIndex, possibleCoreIndex);
        }
    }
    m_isPossibleCoresSourced = true;
    m_lastPossibleCoreIndex = lastPossibleCoreIndex;
}

void ProcessorService::sourceUsage() {
    QFile procStatFile(u"/proc/stat"_s);
    if (!procStatFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ProcessorService] Could not open /proc/stat! Skipping usage update..." << std::endl;
        return;
    }
    const QStringList procStatLines = QString::fromLatin1(procStatFile.readAll()).split(u"\n"_s);
    procStatFile.close();

    QSet<qsizetype> onlineCores;
    ProcessorTimes timesTotal;
    QHash<qsizetype, ProcessorTimes> timesByOnlineCore;

    for (const QString &procStatLine : std::as_const(procStatLines)) {
        if (!procStatLine.startsWith(u"cpu"_s)) {
            continue;
        }
        
        static const QRegularExpression procStatLineRegex(u"^cpu(\\d*)(?:[\\t ]+\\d+){4,10}$"_s);
        const QRegularExpressionMatch procStatLineRegexMatch = procStatLineRegex.match(procStatLine);
        if (!procStatLineRegexMatch.hasMatch()) {
            std::cerr << "[ProcessorService] A line of /proc/stat starting with \"cpu\" seems to be malformed! Skipping usage update..." << std::endl;
            return;
        }

        static const QRegularExpression procStatLineSplitRegex(u"[\\t ]+"_s);
        const QStringList procStatLineColumns = procStatLine.split(procStatLineSplitRegex);
        if (procStatLineColumns.size() < 5 || procStatLineColumns.size() > 11) {
            std::cerr
                << "[ProcessorService] A line of /proc/stat starting with \"cpu\" has an invalid number ("
                << procStatLineColumns.size()
                << ") of columns! Skipping usage update..."
                << std::endl;
            return;
        }

        // First column is at index 1, not 0!
        ProcessorTimes times = {};
        for (qsizetype procStatLineColumnIndex = 1; procStatLineColumnIndex < procStatLineColumns.size(); ++procStatLineColumnIndex) {
            bool timeOK = false;
            const quint64 time = procStatLineColumns[procStatLineColumnIndex].toULongLong(&timeOK);
            if (!timeOK) {
                std::cerr
                    << "[ProcessorService] A column ("
                    << procStatLineColumns[procStatLineColumnIndex].toStdString()
                    << ") of a line of /proc/stat starting with \"cpu\" can not be converted to an integer! Skipping usage update..."
                    << std::endl;
                return;
            }
            if (procStatLineColumnIndex == 4 || procStatLineColumnIndex == 5) {
                times.m_idleTime += time;
            }
            times.m_totalTime += time;
        }

        const QString coreIndexString = procStatLineRegexMatch.captured(1);
        const bool isCoreIndexSpecified = !coreIndexString.isEmpty();

        if (isCoreIndexSpecified) {
            bool coreIndexOK = false;
            const qsizetype coreIndex = coreIndexString.toLongLong(&coreIndexOK);
            if (!coreIndexOK) {
                std::cerr
                    << "[ProcessorService] A core index ("
                    << coreIndexString.toStdString()
                    << ") in a line of /proc/stat starting with \"cpu<core index>\" can not be converted to an integer! Skipping usage update..."
                    << std::endl;
                return;
            }
            onlineCores << coreIndex;
            timesByOnlineCore[coreIndex] = times;
        }
        else {
            timesTotal = times;
        }
    }

    m_stateTotal->updateOnline(timesTotal);
    emit stateTotalChanged();

    if (m_isPossibleCoresSourced) {
        for (const auto &[possibleCoreIndex, possibleCoreState] : std::as_const(m_statesByPossibleCore).asKeyValueRange()) {
            if (onlineCores.contains(possibleCoreIndex)) {
                possibleCoreState->updateOnline(timesByOnlineCore[possibleCoreIndex]);
            }
            else {
                possibleCoreState->updateOffline();
            }
        }
        emit statesByPossibleCoreChanged();
    }
}

void ProcessorService::sourceTemperatureCelsius() {

}
