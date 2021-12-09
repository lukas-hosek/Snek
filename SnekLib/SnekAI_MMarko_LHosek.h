#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"

#include <deque>
#include <vector>
#include <algorithm>


namespace MMLH
{

	inline Coord GetCoordInDir(const Coord coord, const Dir dir)
	{
		switch (dir)
		{
		case Dir::Down:		return Coord(coord.x, coord.y + 1);
		case Dir::Up:			return Coord(coord.x, coord.y - 1);
		case Dir::Left:		return Coord(coord.x - 1, coord.y);
		case Dir::Right:	return Coord(coord.x + 1, coord.y);
		default:					return coord;

		}
	}

	inline Dir GetInverseDir(Dir dir)
	{
		switch (dir)
		{
		case Dir::Down:		return Dir::Up;
		case Dir::Up:			return Dir::Down;
		case Dir::Left:		return Dir::Right;
		case Dir::Right:	return Dir::Left;
		default:					return Dir::None;

		}
	}


	enum class EOccupiedBy
	{
		None = 0,
		Snek = 1,
		Treat = 2
	};

	class HraciPole
	{
	public:
		EOccupiedBy m_pole[Board::Cols][Board::Rows];
		
		HraciPole()
		{
			memset(m_pole, 0, sizeof(m_pole));
		}

		HraciPole(const HraciPole& odkud)
		{
			memcpy(m_pole, odkud.m_pole, sizeof(m_pole));
		}
		
		EOccupiedBy& operator()(Coord c)
		{
			return m_pole[c.x][c.y];
		}
	};
	
	
	class FloodFillFinder
	{
	public:
		typedef std::vector<Coord> Path;
		typedef std::vector<Dir> DirPath;


		Board& m_board;
		Coord m_endPos;
		Coord m_head;
		Dir m_prosleElementySmery[Board::Cols][Board::Rows]; // [x][y]
		HraciPole m_hraciPole;


		FloodFillFinder(const Board& board, Coord head) :
			m_board(const_cast<Board&>(board)),
			m_head(head)
		{
			memset(m_prosleElementySmery, 0, sizeof(m_prosleElementySmery));
		}


		void FillTreats(std::set<Treat> treats)
		{
			for (auto& treat : treats)
			{
				m_hraciPole(treat.Coord) = EOccupiedBy::Treat;
			}
		}


		void FillSnek(std::vector<Coord> body)
		{
			for (auto& c : body)
			{
				m_hraciPole(c) = EOccupiedBy::Snek;
			}
		}



		bool IsTreat(Coord c)
		{
			return m_hraciPole(c) == EOccupiedBy::Treat;
		}
		
		bool IsUnoccupiedBySnake(Coord c)
		{
			return m_hraciPole(c) != EOccupiedBy::Snek;
		}
		

	
		bool FloodFill()
		{
			std::deque<Coord> elementyKProchazeni;
			elementyKProchazeni.push_back(m_head);


			while (!elementyKProchazeni.empty())
			{
				auto current = elementyKProchazeni.front();
				elementyKProchazeni.pop_front();

				if (IsTreat(current))
				{ // nalezeno!
					m_endPos = current;
					return true;
				}


				// test left
				Coord lc = GetCoordInDir(current, Dir::Left);
				if (current.x > 0 && IsUnoccupiedBySnake(lc) && m_prosleElementySmery[lc.x][lc.y] == Dir::None)
				{
					m_prosleElementySmery[lc.x][lc.y] = Dir::Left;
					elementyKProchazeni.push_back(lc);
				}

				// right
				Coord rc = GetCoordInDir(current, Dir::Right);
				if (current.x < m_board.Cols-1 && IsUnoccupiedBySnake(rc) && m_prosleElementySmery[rc.x][rc.y] == Dir::None)
				{
					m_prosleElementySmery[rc.x][rc.y] = Dir::Right;
					elementyKProchazeni.push_back(rc);
				}

				// up
				Coord uc = GetCoordInDir(current, Dir::Up);
				if (current.y > 0 && IsUnoccupiedBySnake(uc) && m_prosleElementySmery[uc.x][uc.y] == Dir::None)
				{
					m_prosleElementySmery[uc.x][uc.y] = Dir::Up;
					elementyKProchazeni.push_back(uc);
				}

				// down
				Coord dc = GetCoordInDir(current, Dir::Down);
				if (current.y < m_board.Rows-1 && IsUnoccupiedBySnake(dc) && m_prosleElementySmery[dc.x][dc.y] == Dir::None)
				{
					m_prosleElementySmery[dc.x][dc.y] = Dir::Down;
					elementyKProchazeni.push_back(dc);
				}
			}

			// nenasli jsme zadny treat
			return false;
		}


		Dir FindDirectionToNearestTreat()
		{
			Coord pathPos = m_endPos;
			Dir smerPrichodu = Dir::None;
			while (pathPos != m_head)
			{
				smerPrichodu = m_prosleElementySmery[pathPos.x][pathPos.y];
				pathPos = GetCoordInDir(pathPos, GetInverseDir(smerPrichodu));
			}

			return smerPrichodu;
		}


		Path FindReversePath(const Coord endPos)
		{
			Coord pathPos = endPos;
			Path cesta;
			while (pathPos != m_head)
			{
				cesta.push_back(pathPos);
				Dir smerPrichodu = m_prosleElementySmery[pathPos.x][pathPos.y];
				pathPos = GetCoordInDir(pathPos, GetInverseDir(smerPrichodu));
			}

			return cesta;
		}

		DirPath FindReversDirPath(const Coord endPos)
		{
			DirPath dirPath;
			Dir smerPrichodu = Dir::None;
			Coord pathPos = endPos;
			while(pathPos != m_head)
			{
				smerPrichodu = m_prosleElementySmery[pathPos.x][pathPos.y];
				dirPath.push_back(smerPrichodu);
				pathPos = GetCoordInDir(pathPos, GetInverseDir(smerPrichodu));
			}

			return dirPath;
		}

	};

}


struct SnekAI_MMarko_LHosek : public SnekAI
{
		virtual Team GetTeam() override { return Team::MMarkoLHosek; };

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
			MMLH::FloodFillFinder fff(board, snek.Body[0]);
			
			for (auto& snek : board.Sneks)
				fff.FillSnek(snek->Body);
			fff.FillTreats(board.Treats);

			if (fff.FloodFill())
			{
				moveRequest = fff.FindDirectionToNearestTreat();
			}

			else
			{ // panik!
			
			}



			boost = false;
			moveRequest = fff.FindDirectionToNearestTreat();

			
			//DummyStep(board, snek, moveRequest, boost);
		}


		void ReportStepDuration(sf::Int64 durationMicroseconds)  override
		{
			//if (durationMicroseconds > MicrosecondsLimit)
			{
				std::cout << durationMicroseconds << "\n";
			}
		}
};
