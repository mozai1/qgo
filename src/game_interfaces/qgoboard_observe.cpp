/***************************************************************************
 *   Copyright (C) 2006 by EB   *
 *      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "qgoboard.h"
#include "qgtp.h"
#include "tree.h"
#include "move.h"


qGoBoardObserveInterface::qGoBoardObserveInterface(BoardWindow *bw, Tree * t, GameData *gd) : qGoBoard(bw,  t, gd) //, QObject(bw)
{
	game_Id = QString::number(gd->gameNumber);
}



/*
 * This functions initialises the board for observing a game
 */
bool qGoBoardObserveInterface::init()
{
	boardwindow->getUi().board->clearData();

//	emit signal_sendCommandFromBoard("games " + game_Id, FALSE);
	emit signal_sendCommandFromBoard("moves " +  game_Id, FALSE);
	emit signal_sendCommandFromBoard("all " +  game_Id, FALSE);
	
	QSettings settings;
	// value 1 = no sound, 0 all games, 2 my games
	playSound = (settings.value("SOUND") == 0);

	startTimer(1000);

	return TRUE;

}

/*
 * Result has been sent byu the server.
 */
void qGoBoardObserveInterface::setResult(QString res, QString xt_res)
{
	if (tree->getCurrent() == NULL)
		return;
	
//	tree->getCurrent()->setComment(res);
//	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());

	kibitzReceived("\n" + res);

	QMessageBox::information(boardwindow , tr("Game n� ") + QString::number(boardwindow->getId()), xt_res);

	QSettings settings;
	if( settings.value("AUTOSAVE").toBool())
		boardwindow->doSave(boardwindow->getCandidateFileName(),TRUE);

	boardwindow->setGamePhase(phaseEnded);
}


/*
 * Comment line - return sent
 */
void qGoBoardObserveInterface::slotSendComment()
{
	emit signal_sendCommandFromBoard("kibitz " + game_Id + " " + boardwindow->getUi().commentEdit2->text() , FALSE);

	boardwindow->getUi().commentEdit->append("-> " + boardwindow->getUi().commentEdit2->text());

	boardwindow->getUi().commentEdit2->clear();
}


/*
 *  time info has been send by parser
 *  TODO : make sure we won't need this in qgoboard (code duplicate)
 */
void qGoBoardObserveInterface::setTimerInfo(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	int bt_i = btime.toInt();
	int wt_i = wtime.toInt();
//	b_stones = bstones;
//	w_stones = wstones;
/*
#ifdef SHOW_INTERNAL_TIME
	if (chk_b < 0)
	{
		chk_b = bt_i;
		chk_w = wt_i;
	}
#endif
*/
	// set string in any case
//	bt = secToTime(bt_i);
//	wt = secToTime(wt_i);
	QTime t0 = QTime::QTime(0,0);
//	t0.addSecs(bt_i).toString("m:ss");
	QTime t1 = t0;

	// set initial timer until game is initialized
//	if (!have_gameData)
//		win->getInterfaceHandler()->setTimes(bt, bstones, wt, wstones);

	if (boardwindow->getGamePhase() != phaseInit)
		boardwindow->getInterfaceHandler()->setTimes(t0.addSecs(bt_i).toString("m:ss"), bstones, t0.addSecs(wt_i).toString("m:ss"), wstones);

	// if time info available, sound can be played
	// cause no moves cmd in execution
//	sound = true;
}


/*
 * A move string is incoming from the interface (server)
 */
void qGoBoardObserveInterface::set_move(StoneColor sc, QString pt, QString mv_nr)
{
	//if we are observing a game, we might not be at the last move
	Move *remember = tree->getCurrent();
	Move *last = tree->findLastMoveInMainBranch();

	tree->setCurrent(last);

	int mv_nr_int;
	int mv_counter = tree->getCurrent()->getMoveNumber();
	bool hcp_move = tree->getCurrent()->isHandicapMove();
	// IGS: case undo with 'mark': no following command
	// -> from qgoIF::slot_undo(): set_move(stoneNone, 0, 0)
	if (mv_nr.isEmpty())
		// undo one move
		mv_nr_int = mv_counter - 1;
	else
		mv_nr_int = mv_nr.toInt();


	//special case : after an undo, IGS will give the last move before the undo move
	// since we are already there, we skip
	if ((mv_nr_int + 1 == mv_counter) && (mv_nr_int > 0))
	{
		qDebug("move given is before last move");
		return;
	}


	// We are observing a game, and we take the game in the middle
	if (mv_nr_int > mv_counter)
	{
		if (mv_nr_int != mv_counter + 1 && mv_counter != 0)
			// case: move has done but "moves" cmd not executed yet
			qWarning("**** LOST MOVE !!!! ****");
		// if the move number given by the server is over current, we skip 
		// (means the 'move' command moves have not yet come in)
		// exception : the fist move, if a handicap, has number 0 and next incoming IGS move has number 1 (instead of 0)
		else if (mv_counter == 0 && mv_nr_int != 0 && !hcp_move)
		{
			qDebug("move skipped");
			// skip moves until "moves" cmd executed
			return;
		}
		else
			mv_counter++;
	}
/*	else if (mv_nr_int + 1 == mv_counter)
	{
		// scoring mode? (NNGS)
		if (gameMode == modeScore)
		{
			// back to matchMode
			win->doRealScore(false);
			win->getBoard()->setMode(modeMatch);
		}

		// special case: undo handicap
		if (mv_counter <= 0 && gd.handicap)
		{
			gd.handicap = 0;
			win->getBoard()->getBoardHandler()->setHandicap(0);
			qDebug("set Handicap to 0");
		}

		if (!pt)
		{
			// case: undo
			qDebug("set_move(): UNDO in game " + QString::number(id));
			qDebug("...mv_nr = " + mv_nr);

			                                                                       //added eb 9
			Move *m=win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			Move *last=win->getBoard()->getBoardHandler()->lastValidMove ; //win->getBoard()->getBoardHandler()->getTree()->findLastMoveInMainBranch();

			if (m!=last)                          // equivalent to display_incoming_move = false
				win->getBoard()->getBoardHandler()->getTree()->setCurrent(last) ;
			else
				m = m->parent ;                   //we are going to delete the node. We will bactrack to m

			win->getBoard()->deleteNode();
			win->getBoard()->getBoardHandler()->lastValidMove = win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			win->getBoard()->getBoardHandler()->getTree()->getCurrent()->marker = NULL ; //(just in case)
			win->getBoard()->getBoardHandler()->getStoneHandler()->checkAllPositions();
                             
			win->getBoard()->getBoardHandler()->getTree()->setCurrent(m) ;   //we return where we were observing the game
			// this is not very nice, but it ensures things stay clean
			win->getBoard()->getBoardHandler()->updateMove(m);                      // end add eb 9

			mv_counter--;

			// ok for sound - no moves cmd in execution
			sound = true;
		}

		return;
	}
	else
		// move already done...
		return;
*/
	if (pt.contains("Handicap"))
	{
		QString handi = pt.simplified();
		int h = handi.section(" ",-1).toInt();//element(handi, 1, " ").toInt();
		
		setHandicap(h);
		// check if handicap is set with initGame() - game data from server do not
		// contain correct handicap in early stage, because handicap is first move!
		if ( boardwindow->getGameData()->handicap != h)
		{
			boardwindow->getGameData()->handicap = h;
			qDebug("corrected Handicap");
		}
	}

	else if (pt.contains("Pass",Qt::CaseInsensitive))
	{
//		win->getBoard()->doSinglePass();
//		if (win->getBoard()->getBoardHandler()->local_stone_sound)
//			qgo->playPassSound();
		doPass();
	}
	else
	{
/*		if ((gameMode == modeMatch) && (mv_counter < 2) && !(myColorIsBlack))
		{
			// if black has not already done - maybe too late here???
			if (requests_set)
			{
				qDebug(QString("qGoBoard::set_move() : check_requests at move %1").arg(mv_counter));
				check_requests();
			}
		}
*/
		int i = pt[0].unicode() - QChar::fromAscii('A').unicode() + 1;
		// skip j
		if (i > 8)
			i--;

		int j ;//= boardwindow->getGameData()->size + 1 - pt.mid(1).toInt();

		if (pt[2] >= '0' && pt[2] <= '9')
			j = boardwindow->getGameData()->size + 1 - pt.mid(1,2).toInt();
		else
			j = boardwindow->getGameData()->size + 1 - pt[1].digitValue();


		if (!doMove(sc, i, j))
			QMessageBox::warning(boardwindow, tr("Invalid Move"), tr("The incoming move %1 seems to be invalid").arg(pt.toLatin1().constData()));
//		qDebug ("QGoboard:setMove - move %d %d done",i,j);
/*
		Move *m = tree->getCurrent();
		if (stated_mv_count > mv_counter ||
			//gameMode == modeTeach ||
			//ExtendedTeachingGame ||
			wt_i < 0 || bt_i < 0)
		{
			m->setTimeinfo(false);
		}
		else
		{
			m->setTimeLeft(sc == stoneBlack ? bt_i : wt_i);
			int stones = -1;
			if (sc == stoneBlack)
				stones = b_stones.toInt();
			else
				stones = w_stones.toInt();
			m->setOpenMoves(stones);
			m->setTimeinfo(true);

			// check for a common error -> times and open moves identical
			if (m->parent &&
				m->parent->parent &&
				m->parent->parent->getTimeinfo() &&
				m->parent->parent->getTimeLeft() == m->getTimeLeft() &&
				m->parent->parent->getOpenMoves() == m->getOpenMoves())
			{
				m->parent->parent->setTimeinfo(false);
			}
		}
*/	
	}
	
	//check wether we should update to the incoming move or not
	if (remember != last)
		tree->setCurrent(remember);
		
	boardwindow->getBoardHandler()->updateMove(tree->getCurrent());
}
