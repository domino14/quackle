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

#ifndef QUACKER_SIM_THREAD_H
#define QUACKER_SIM_THREAD_H

#include <QThread>

#include <sim.h>
#include <game.h>

class SimThread : public QThread {
Q_OBJECT

public:
	SimThread(QObject *parent = 0);
	~SimThread();

	void setPosition(const Quackle::GamePosition &position);
	const Quackle::GamePosition &position() const;

	const Quackle::MoveList &moves() const;
	void startSim(int plies);
	Quackle::Simulator* simulator();
protected:
	void run();

signals:
	void iterationsDone(int nIterations);

private:
	int m_plies;
	Quackle::GamePosition m_position;
	Quackle::MoveList m_moves;
	Quackle::Simulator *m_simulator;
};

class SimThreads : public QObject {
Q_OBJECT

public:
	SimThreads(QObject *parent = 0);
	~SimThreads();
	void startSim(int plies);
	void setCurrentPlayerRack(const Quackle::Rack &rack);
	void resetNumbers();
	void setPosition(const Quackle::GamePosition &position);
private:
	QList<SimThread*> m_threads;
private slots:
	void iterationsDone(int);
};

inline const Quackle::GamePosition &SimThread::position() const
{
	return m_position;
}

inline Quackle::Simulator* SimThread::simulator() {
	return m_simulator;
}


#endif