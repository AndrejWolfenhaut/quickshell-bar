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
* - From plugin/src/Caelestia/Services/sensorslib.cpp
*   - Most of the code for sourcing CPU/GPU temperatures using libsensors (init, cleanup, reading sensors, ...)
*
****************/

#include <atomic>
#include <mutex>
#include <iostream>
#include <functional>

#include <QByteArray>

#include <sensors/sensors.h>

#include "Sensors.hpp"



namespace Sensors {

namespace {

//
// Init OK flag and once flag
//

std::atomic<bool> g_initOK{ false };
std::once_flag g_initOnceFlag;

//
// Init and cleanup
//

static void init() {
    if (sensors_init(nullptr) != 0) {
        std::cerr << "[Sensors] Sensors initialization failed!" << std::endl;
        g_initOK.store(false, std::memory_order_release);
        return;
    }
    else {
        g_initOK.store(true, std::memory_order_release);
        std::atexit([]() -> void {
            if (g_initOK.load(std::memory_order_acquire)) {
                sensors_cleanup();
            }
        });
    }
}

static void ensureInit() {
    std::call_once(g_initOnceFlag, init);
}

//
// Helpers
//

[[nodiscard]] static QByteArray getSensorsFeatureLabel(const sensors_chip_name *sensorsChipName, const sensors_feature *sensorsFeature) {
    char *sensorsFeatureLabelRaw = sensors_get_label(sensorsChipName, sensorsFeature);
    if (sensorsFeatureLabelRaw == nullptr) {
        // TODO: How do we print the chip and feature?
        std::cerr << "[Sensors] Could not get sensors feature label for chip " << sensorsChipName->path << std::endl;
        return {};
    }

    const QByteArray sensorsFeatureLabel(sensorsFeatureLabelRaw);
    std::free(sensorsFeatureLabelRaw);

    return sensorsFeatureLabel;
}

[[nodiscard]] static bool sensorsFeatureLabelEquals(const QByteArray sensorsFeatureLabel, const char *full) {
    return sensorsFeatureLabel == QByteArrayView(full);
}

[[nodiscard]] static bool sensorsFeatureLabelStartsWith(const QByteArray sensorsFeatureLabel, const char *prefix) {
    const size_t stringPrefixLength = std::strlen(prefix);
    return (
        std::cmp_greater_equal(sensorsFeatureLabel.size(), stringPrefixLength) &&
        std::memcmp(sensorsFeatureLabel.constData(), prefix, stringPrefixLength) == 0
    );
}

static void forEachTemperatureFeature(
    std::function<void(const sensors_chip_name *, const sensors_feature *, const QByteArray)> &&action,
    qint32 sensorsBusType = SENSORS_BUS_TYPE_ANY
) {
    qint32 sensorsChipNumber = 0;
    const sensors_chip_name *sensorsChipName = nullptr;
    while ((sensorsChipName = sensors_get_detected_chips(nullptr, &sensorsChipNumber)) != nullptr) {
        if (sensorsBusType != SENSORS_BUS_TYPE_ANY && sensorsChipName->bus.type != sensorsBusType) {
            continue;
        }

        qint32 sensorsFeatureNumber = 0;
        const sensors_feature* sensorsFeature = nullptr;
        while ((sensorsFeature = sensors_get_features(sensorsChipName, &sensorsFeatureNumber)) != nullptr) {
            if (sensorsFeature->type != SENSORS_FEATURE_TEMP) {
                continue;
            }

            const QByteArray sensorsFeatureLabel = getSensorsFeatureLabel(sensorsChipName, sensorsFeature);
            if (sensorsFeatureLabel.isEmpty()) {
                continue;
            }

            action(sensorsChipName, sensorsFeature, sensorsFeatureLabel);
        }
    }
}

[[nodiscard]] std::optional<qreal> readTemperatureInput(const sensors_chip_name* sensorsChipName, const sensors_feature* sensorsFeature) {
    const sensors_subfeature *sensorsTemperatureSubfeature = sensors_get_subfeature(sensorsChipName, sensorsFeature, SENSORS_SUBFEATURE_TEMP_INPUT);

    if (sensorsTemperatureSubfeature == nullptr) {
        return std::nullopt;
    }

    qreal value = 0.0;
    if (sensors_get_value(sensorsChipName, sensorsTemperatureSubfeature->number, &value) != 0) {
        std::cerr << "[Sensors] Could not read temperature input!" << std::endl;
        return std::nullopt;
    }
    else {
        return value;
    }
}

//
// Temperature accumulator
//

struct TemperatureAccumulator {
    qreal m_temperatureSum;
    qsizetype m_temperatureCount;

    TemperatureAccumulator();

    void registerTemperature(qreal temperature);

    [[nodiscard]] std::optional<qreal> averageTemperature();
};

TemperatureAccumulator::TemperatureAccumulator() :
m_temperatureSum(0.0),
m_temperatureCount(0) {}

void TemperatureAccumulator::registerTemperature(qreal temperature) {
    m_temperatureSum += temperature;
    ++m_temperatureCount;
}

std::optional<qreal> TemperatureAccumulator::averageTemperature() {
    if (m_temperatureCount == 0) {
        return std::nullopt;
    }
    else {
        return m_temperatureSum / static_cast<qreal>(m_temperatureCount);
    }
}

//
// GPU temperature classification
//

enum class GPUTemperatureType : quint8 {
    None = 0x00,
    Primary = 0x01,
    Fallback = 0x02
};

GPUTemperatureType getGPUTemperatureType(const QByteArray sensorsFeatureLabel) {
    // TODO: Is this necessary?
    if (sensorsFeatureLabel.isEmpty()) {
        return GPUTemperatureType::None;
    }

    const bool isTempIndexed = (
        sensorsFeatureLabelStartsWith(sensorsFeatureLabel, "temp") &&
        sensorsFeatureLabel.size() > 4 &&
        std::isdigit(static_cast<unsigned char>(sensorsFeatureLabel[4])) != 0
    );

    // Indexed tempN labels and the vendor die labels are the reading we want,
    // junction/mem only stand in when no primary sensor exists
    if (
        isTempIndexed ||
        sensorsFeatureLabelEquals(sensorsFeatureLabel, "GPU core") ||
        sensorsFeatureLabelEquals(sensorsFeatureLabel, "edge")
    ) {
        return GPUTemperatureType::Primary;
    }
    else if (
        sensorsFeatureLabelEquals(sensorsFeatureLabel, "junction") ||
        sensorsFeatureLabelEquals(sensorsFeatureLabel, "mem")
    ) {
        return GPUTemperatureType::Fallback;
    }
    else {
        return GPUTemperatureType::None;
    }
}

} // namespace

//
// Externally accessible functions
//

std::optional<qreal> sourceCPUTemperature() {
    ensureInit();
    if (!g_initOK.load(std::memory_order_acquire)) {
        return std::nullopt;
    }

    std::optional<qreal> primaryCPUTemperature;
    std::optional<qreal> fallbackCPUTemperature;

    forEachTemperatureFeature([&](const sensors_chip_name* sensorsChipName, const sensors_feature* sensorsFeature, const QByteArray sensorsFeatureLabel) -> void {
        if (sensorsFeatureLabelStartsWith(sensorsFeatureLabel, "Package id ") || sensorsFeatureLabelEquals(sensorsFeatureLabel, "Tdie")) {
            const std::optional<qreal> sensorsPrimaryTemperatureInputValue = readTemperatureInput(sensorsChipName, sensorsFeature);
            if (sensorsPrimaryTemperatureInputValue.has_value()) {
                primaryCPUTemperature = sensorsPrimaryTemperatureInputValue;
            }
        }
        else if (sensorsFeatureLabelEquals(sensorsFeatureLabel, "Tctl")) {
            const std::optional<qreal> sensorsFallbackTemperatureInputValue = readTemperatureInput(sensorsChipName, sensorsFeature);
            if (sensorsFallbackTemperatureInputValue.has_value()) {
                fallbackCPUTemperature = sensorsFallbackTemperatureInputValue;
            }
        }
    });

    return primaryCPUTemperature.has_value()
        ? primaryCPUTemperature
        : fallbackCPUTemperature;
}

std::optional<qreal> sourceGPUTemperature() {
    ensureInit();
    if (!g_initOK.load(std::memory_order_acquire)) {
        return std::nullopt;
    }

    TemperatureAccumulator primaryGPUTemperatureAccumulator;
    TemperatureAccumulator fallbackGPUTemperatureAccumulator;

    forEachTemperatureFeature([&](const sensors_chip_name* sensorsChipName, const sensors_feature* sensorsFeature, const QByteArray sensorsFeatureLabel) -> void {
        const GPUTemperatureType gpuTemperatureType = getGPUTemperatureType(sensorsFeatureLabel);

        if (gpuTemperatureType == GPUTemperatureType::None) {
            return;
        }

        const std::optional<qreal> sensorsGPUTemperatureInputValue = readTemperatureInput(sensorsChipName, sensorsFeature);
        if (!sensorsGPUTemperatureInputValue.has_value()) {
            return;
        }

        if (gpuTemperatureType == GPUTemperatureType::Primary) {
            primaryGPUTemperatureAccumulator.registerTemperature(*sensorsGPUTemperatureInputValue);
        }
        else {
            fallbackGPUTemperatureAccumulator.registerTemperature(*sensorsGPUTemperatureInputValue);
        }
    }, SENSORS_BUS_TYPE_PCI);

    std::optional<qreal> primaryGPUTemperature = primaryGPUTemperatureAccumulator.averageTemperature();
    if (primaryGPUTemperature.has_value()) {
        return primaryGPUTemperature;
    }
    else {
        return fallbackGPUTemperatureAccumulator.averageTemperature();
    }
}

} // namespace Sensors
