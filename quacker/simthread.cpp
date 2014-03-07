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
    int nThreads = qMax(QThread::idealThreadCount(), 1);
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
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->setPosition(position);
    }
}

void SimThreads::startSim(int plies) {
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->startSim(plies);
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
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->simulator()->currentPosition().setCurrentPlayerRack(rack);
    }
}

void SimThreads::resetNumbers() {
    m_totalIterations = 0;
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->simulator()->resetNumbers();
    }
}

void SimThreads::abort() {
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->abort();
    }
}

void SimThreads::setIncludedMoves(const Quackle::MoveList &moves) {
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->simulator()->setIncludedMoves(moves);
    }
}

void SimThreads::setIgnoreOppos(bool ignore) {
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->simulator()->setIgnoreOppos(ignore);
    }
}

void SimThreads::setPartialOppoRack(const Quackle::Rack &rack) {
    SimThread* thread;
    foreach(thread, m_threads) {
        thread->simulator()->setPartialOppoRack(rack);
    }
}

Quackle::MoveList SimThreads::moves(bool prune, bool byWin) {
    // This needs to act like Simulator::moves but combine across all
    // simulators. Luckily m_simmedMoves for every simulator has
    // a deterministic order.
    Quackle::MoveList ret;
    const Quackle::SimmedMove* simmedMove;
    const bool useCalculatedEquity = hasSimulationResults();
    // Need to go through std::vectors by index instead of iterator. Again
    // assume these are all in the same order for all simulators.
    
    for (size_t i = 0; i < m_threads[0]->simulator()->simmedMoves().size(); i++) {
        simmedMove = &(m_threads[0]->simulator()->simmedMoves()[i]);
        if (prune && !simmedMove->includeInSimulation())
            continue;

        Quackle::Move move(simmedMove->move);
        if (useCalculatedEquity) {
            move.equity = avgAcrossThreads(i, MetricEquity);
            move.win = avgAcrossThreads(i, MetricWins);
        }
        ret.push_back(move);
    }
    if (byWin && useCalculatedEquity) {
        Quackle::MoveList::sort(ret, Quackle::MoveList::Win);
    } else {
        Quackle::MoveList::sort(ret, Quackle::MoveList::Equity);
    }
    return ret;
}

/**
 * Calculates the metric of the simmed move at the given index.
 * NOTE: This assumes that all simmedMoves are in the same order across
 * all threads. They should be because they are not sorted in the
 * sim threads at all and they are added in order.
 * @param  index 
 * @param  metric
 */
double SimThreads::avgAcrossThreads(int index, MetricType metric) {
    double metricSum = 0;
    int iterations = 0;
    const Quackle::SimmedMove* simmedMove;
    foreach(SimThread* thread, m_threads) {
        int this_iter;
        simmedMove = &(thread->simulator()->simmedMoves()[index]);
        this_iter = simmedMove->iterations();
        iterations += this_iter;
        if (metric == MetricEquity) {
            metricSum += simmedMove->calculateEquity() * this_iter;
        } else if (metric == MetricWins) {
            metricSum += simmedMove->wins.averagedValue() * this_iter;
        }
    }   
    return metricSum / (double)iterations;
}