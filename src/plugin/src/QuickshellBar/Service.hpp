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
* - From plugin/src/Caelestia/Services/service.hpp
*   - Code for abstract reference-counting service
*
****************/

#ifndef QUICKSHELL_BAR_SERVICE_HPP
#define QUICKSHELL_BAR_SERVICE_HPP

#include <QObject>
#include <QSet>



class Service : public QObject {
    Q_OBJECT

public:
    explicit Service(QObject *parent = nullptr);

    void addServiceReference(QObject *serviceReference);
    void removeServiceReference(QObject *serviceReference);

private:
    QSet<QObject *> m_serviceReferences;

    virtual void start() = 0;
    virtual void stop() = 0;
};

#endif
