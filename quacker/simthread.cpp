/*
 *  Quackle -- Crossword game artificial intelligence and analysis tool
 *  Copyright (C) 2005-2006 Jason Katz-Brown and John O'Laughlin.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

#include <uv.h>

#include "simthread.h"

SimThread::SimThread(QObject *parent) : QThread(parent) {
    m_simulator = new Quackle::Simulator;
}

SimThread::~SimThread() {
    wait();
    delete m_simulator;
}

void SimThread::startSim(int plies) {
    // XXX: need to set a dispatch for m_simulator
    if (isRunning()) {
        UVcout << "SimThread is already running!" << endl;
        return;
    }
    m_plies = plies;
    start(QThread::HighPriority);
}

void SimThread::run() {
    UVcout << "i am a sim thread and i am running!" << endl;
    // Simulate 10 at a time?
    m_simulator->simulate(m_plies, 10);
    UVcout << "simulated 10 plies!" << endl;
    // keep simulating...
    UVcout << m_simulator->simmedMoves() << endl;
}

void SimThread::setPosition(const Quackle::GamePosition &position) {
    m_simulator->setPosition(position);
}

//////////////////////////////////////////
/** Thread manager */
SimThreads::SimThreads(QObject *parent) : QObject(parent) {
    int nThreads = QThread::idealThreadCount();
    UVcout << "Will create " << nThreads << " threads.\n";
    for (int i = 0; i < nThreads; i++) {
        SimThread *t = new SimThread;
        m_threads.push_back(t);
        connect(t, SIGNAL(iterationsDone()), this, SLOT(iterationsDone()));
    }
}

SimThreads::~SimThreads() {
}

void SimThreads::setPosition(const Quackle::GamePosition &position) {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->setPosition(position);
    }
}

void SimThreads::startSim(int plies) {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->startSim(plies);
    }
}

void SimThreads::iterationsDone(int nIterations) {
    // This many iterations have been done so far. Communicate back to parent.
}

void SimThreads::setCurrentPlayerRack(const Quackle::Rack &rack) {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->simulator()->currentPosition().setCurrentPlayerRack(rack);
    }
}

void SimThreads::resetNumbers() {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->simulator()->resetNumbers();
    }
}