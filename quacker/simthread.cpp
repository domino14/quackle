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
    m_iterationsPerLoop = 10;
    m_simulator = new Quackle::Simulator;
}

SimThread::~SimThread() {
    wait();
    delete m_simulator;
}

void SimThread::startSim(int plies) {
    if (isRunning()) {
        UVcout << "SimThread is already running!" << endl;
        return;
    }
    m_shouldAbort = false;
    m_plies = plies;
    start(QThread::HighPriority);
}

void SimThread::run() {
    while (!m_shouldAbort) {
        m_simulator->simulate(m_plies, m_iterationsPerLoop);
        // UVcout << m_simulator->simmedMoves() << endl;
        emit iterationsDone(m_iterationsPerLoop);
    }
    UVcout << "Ending simulation." << m_simulator->simmedMoves() << endl;
}

void SimThread::abort() {
    // Abort the thread, ending the run function above.
    m_shouldAbort = true;
}

void SimThread::setPosition(const Quackle::GamePosition &position) {
    m_simulator->setPosition(position);
}

//////////////////////////////////////////
/**
 * Thread manager. This class should replace uses of m_simulator in
 * quacker.cpp and hopefully in other places later (so computer players 
 * can also use all threads available to them).
 */
SimThreads::SimThreads(QObject *parent) : QObject(parent) {
    int nThreads = QThread::idealThreadCount();
    m_totalIterations = 0;
    UVcout << "Will create " << nThreads << " threads.\n";
    for (int i = 0; i < nThreads; i++) {
        SimThread *t = new SimThread;
        m_threads.push_back(t);
        connect(t, SIGNAL(iterationsDone(int)), this, SLOT(partialIterations(int)));
    }
}

SimThreads::~SimThreads() {
    for (int i = 0; i < m_threads.length(); i++) {
        delete m_threads[i];
    }
}

void SimThreads::setPosition(const Quackle::GamePosition &position) {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->setPosition(position);
    }
}

void SimThreads::startSim(int plies) {
    m_totalIterations = 0;
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->startSim(plies);
    }
}

int SimThreads::numThreads() {
    return m_threads.length();
}

void SimThreads::partialIterations(int nIterations) {
    m_totalIterations += nIterations;
    // This many iterations have been done so far. Communicate back to parent.
    emit iterationsDone(m_totalIterations);
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

void SimThreads::abort() {
    for (int i = 0; i < m_threads.length(); i++) {
        m_threads[i]->abort();
    }
}