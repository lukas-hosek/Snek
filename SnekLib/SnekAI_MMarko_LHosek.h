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

		void FillTreats(const std::set<Treat>& treats)
		{
			for (const auto& treat : treats)
			{
				operator()(treat.Coord) = EOccupiedBy::Treat;
			}
		}

		void FillSnek(const std::vector<Coord>& body)
		{
			for (const auto& c : body)
			{
				operator()(c) = EOccupiedBy::Snek;
			}
		}

		void FillSneks(const std::vector<std::unique_ptr<Snek>>& sneks)
		{
			for (const auto& snek : sneks)
			{
				FillSnek(snek->Body);
			}
		}

		void FillBoard(const Board& board)
		{
			FillTreats(board.Treats);
			FillSneks(board.Sneks);
		}

		bool IsTreat(Coord c)
		{
			return 	operator()(c) == EOccupiedBy::Treat;
		}

		bool IsUnoccupiedBySnake(Coord c)
		{
			return 	operator()(c) != EOccupiedBy::Snek;
		}
	};
	
	
	class FloodFillFinder
	{
	public:
		typedef std::vector<Coord> Path;
		typedef std::vector<Dir> DirPath;


		Coord m_endPos;
		Coord m_head;
		Dir m_prosleElementySmery[Board::Cols][Board::Rows]; // [x][y]
		HraciPole& m_hraciPole;
		int m_floodFillArea;
		std::deque<Coord> m_elementyKProchazeni;



		FloodFillFinder(HraciPole& pole, Coord head) :
			m_hraciPole(pole), m_head(head)
		{
			
		}

		
		

	
		bool FloodFill()
		{
			m_elementyKProchazeni.clear();
			
			m_floodFillArea = 0;
			memset(m_prosleElementySmery, 0, sizeof(m_prosleElementySmery));
			
			m_elementyKProchazeni.push_back(m_head);

			while (!m_elementyKProchazeni.empty())
			{
				auto current = m_elementyKProchazeni.front();
				m_elementyKProchazeni.pop_front();

				if (m_hraciPole.IsTreat(current))
				{ // nalezeno!
					m_endPos = current;
					return true;
				}

				auto CheckDirection = [&](Dir dir)
				{
					Coord nc = GetCoordInDir(current, dir);
					if (nc.x < Board::Cols && nc.y < Board::Rows && m_hraciPole.IsUnoccupiedBySnake(nc) && m_prosleElementySmery[nc.x][nc.y] == Dir::None)
					{
						m_prosleElementySmery[nc.x][nc.y] = dir;
						m_elementyKProchazeni.push_back(nc);
						++m_floodFillArea;

					}
				};

				CheckDirection(Dir::Left);
				CheckDirection(Dir::Right);
				CheckDirection(Dir::Up);
				CheckDirection(Dir::Down);
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
	enum class EState
	{
		FindingTreats,
		AttackPrepare,
		AttackExecute,
		Panik

	};
	
	
	EState m_state;


	SnekAI_MMarko_LHosek() :
		m_state(EState::FindingTreats)
	{}
	
	
	
	virtual Team GetTeam() override { return Team::MMarkoLHosek; };

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
			switch (m_state)
			{
			case EState::FindingTreats:
				FindTreats(board, snek, moveRequest, boost);
				break;
			default:
				Panik(board, snek, moveRequest, boost);
				break;
			
			}
		}

		void FindTreats(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
			boost = false;

			MMLH::HraciPole hraciPole_SneksOnly;
			hraciPole_SneksOnly.FillSneks(board.Sneks);
			MMLH::HraciPole hraciPole_Treats(hraciPole_SneksOnly);
			hraciPole_Treats.FillTreats(board.Treats);

			MMLH::FloodFillFinder fff(hraciPole_Treats, snek.Body[0]);
			int attempts = 0;


			while (fff.FloodFill() && attempts < 3)
			{
				++attempts;
				auto path = fff.FindReversePath(fff.m_endPos);

				// test if path doesn't block us
				MMLH::HraciPole testPole(hraciPole_SneksOnly);
				testPole.FillSnek(path);
				MMLH::FloodFillFinder fff2(testPole, fff.m_endPos);
				fff2.FloodFill();
				if (fff2.m_floodFillArea > snek.Body.size() * 2)
				{
					// parada, uspech
					moveRequest = fff.FindDirectionToNearestTreat();
					return;
				}
				else
				{
					hraciPole_Treats(fff.m_endPos) = MMLH::EOccupiedBy::None;
				}
			}

			{ // panik!
				Panik(board, snek, moveRequest, boost);
			}

			//DummyStep(board, snek, moveRequest, boost);
		}

		void Panik(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
			MMLH::HraciPole hraciPole;
			hraciPole.FillBoard(board);

			Coord head = snek.Body[0];

			auto testDirection = [&](Dir dir) -> bool
			{
				Coord nextCoord = MMLH::GetCoordInDir(head, dir);
				if(const_cast<Board&>(board).IsWithinBounds(nextCoord) && hraciPole.IsUnoccupiedBySnake(nextCoord))
				{
					return true;
				}

				return false;
			};

			if(testDirection(Dir::Left))
			{
				moveRequest = Dir::Left;
			}

			else if (testDirection(Dir::Right))
			{
				moveRequest = Dir::Right;
			}

			else if (testDirection(Dir::Up))
			{
				moveRequest = Dir::Up;
			}

			else if (testDirection(Dir::Down))
			{
				moveRequest = Dir::Down;
			}
			else
			{
				
			}

			Coord boostResultCoord = MMLH::GetCoordInDir(MMLH::GetCoordInDir(head, moveRequest), moveRequest);
			boost = const_cast<Board&>(board).IsWithinBounds(boostResultCoord) && hraciPole.IsUnoccupiedBySnake(boostResultCoord) && snek.Body.size() > 3;
		}


		void ReportStepDuration(sf::Int64 durationMicroseconds)  override
		{
			if (durationMicroseconds > MicrosecondsLimit)
			{
				std::cout << durationMicroseconds << "\n";
			}
		}
};
