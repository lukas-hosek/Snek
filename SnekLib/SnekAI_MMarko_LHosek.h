#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"

#include <deque>
#include <vector>
#include <algorithm>
#include <array>
#include <assert.h>


namespace MMLH
{

	inline bool IsValidCoord(Coord c)
	{
		return c.x < Board::Cols&& c.y < Board::Rows;
	}

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


	template <typename T>
	class FastDeque
	{
	public:
		int m_start;
		int m_end;
		T m_items[Board::Rows * Board::Cols];

		FastDeque() :
			m_start(0), m_end(0)
		{}

		void push_back(T x)
		{
			m_items[m_end++] = x;
		}

		T front()
		{
			return m_items[m_start];
		}

		void pop_front()
		{
			++m_start;
		}

		bool empty()
		{
			return m_start == m_end;
		}
	};


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
			assert(c.x < Board::Cols&& c.y < Board::Rows);
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


		void BlockSuboptimalDirections(Coord from)
		{
			Dir dirs[] = { Dir::Left, Dir::Right, Dir::Up, Dir::Down };
			int vals[] = { 0, 0, 0, 0 };
			int maxVal = 0;


			for (int i = 0; i < 4; ++i)
			{
				Dir currentDir = dirs[i];
				Coord next = GetCoordInDir(from, currentDir);
				if (IsValidCoord(next) && IsUnoccupiedBySnake(next))
				{
					HraciPole pole2(*this);
					vals[i] = pole2.CalculateFreeAreaAround(next);
					if (vals[i] > maxVal)
						maxVal = vals[i];
				}

			}

			for (int i = 0; i < 4; ++i)
			{
				Coord c = GetCoordInDir(from, dirs[i]);
				if (IsValidCoord(c) && vals[i] > 0 && vals[i] < maxVal)
				{
					operator()(c) = EOccupiedBy::Snek;
				}
			}
		}


		int CalculateFreeAreaAround(Coord start)
		{
			operator()(start) = EOccupiedBy::Snek;
			FastDeque<Coord> fronta;
			fronta.push_back(start);
			int area = 0;

			while (!fronta.empty())
			{
				Coord current = fronta.front();
				fronta.pop_front();

				auto CheckDirection = [&](Dir dir)
				{
					Coord nc = GetCoordInDir(current, dir);
					if (IsValidCoord(nc) && IsUnoccupiedBySnake(nc))
					{
						operator()(nc) = EOccupiedBy::Snek;
						fronta.push_back(nc);
						++area;
					}
				};

				CheckDirection(Dir::Left);
				CheckDirection(Dir::Right);
				CheckDirection(Dir::Up);
				CheckDirection(Dir::Down);
			}

			return area;
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



		FloodFillFinder(HraciPole& pole, Coord head) :
			m_hraciPole(pole), m_head(head)
		{

		}





		bool FloodFill()
		{
			FastDeque<Coord> elementyKProchazeni;

			memset(m_prosleElementySmery, 0, sizeof(m_prosleElementySmery));

			elementyKProchazeni.push_back(m_head);

			while (!elementyKProchazeni.empty())
			{
				auto current = elementyKProchazeni.front();
				elementyKProchazeni.pop_front();

				if (m_hraciPole.IsTreat(current))
				{ // nalezeno!
					m_endPos = current;
					return true;
				}

				auto CheckDirection = [&](Dir dir)
				{
					Coord nc = GetCoordInDir(current, dir);
					if (IsValidCoord(nc) && m_hraciPole.IsUnoccupiedBySnake(nc) && m_prosleElementySmery[nc.x][nc.y] == Dir::None)
					{
						m_prosleElementySmery[nc.x][nc.y] = dir;
						elementyKProchazeni.push_back(nc);

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
			while (pathPos != m_head)
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
		MMLH::HraciPole hraciPole_SneksOnly;
		hraciPole_SneksOnly.FillSneks(board.Sneks);
		hraciPole_SneksOnly.BlockSuboptimalDirections(snek.Body[0]);

		switch (m_state)
		{
		case EState::FindingTreats:
		{
			MMLH::HraciPole hraciPole_treats(hraciPole_SneksOnly);
			hraciPole_treats.FillTreats(board.Treats);
			FindTreats(hraciPole_treats, snek.Body[0], snek.Body.size(), moveRequest, boost);
			break;
		}
		default:
		{
			Panik(hraciPole_SneksOnly, snek.Body[0], moveRequest, boost);
			break;
		}
		}
	}




	void FindTreats(MMLH::HraciPole& hraciPole_Treats, Coord start, int bodySize, Dir& moveRequest, bool& boost)
	{
		boost = false;

		int attempts = 0;
		int maxArea = 0;

		for (int attempts = 0; attempts < 8; ++attempts)
		{
			MMLH::FloodFillFinder fff(hraciPole_Treats, start);
			if (!fff.FloodFill())
				break;
			auto path = fff.FindReversePath(fff.m_endPos);

			// test if path doesn't block us
			MMLH::HraciPole testPole(hraciPole_Treats);
			testPole.FillSnek(path);
			int resultArea = testPole.CalculateFreeAreaAround(path.front());
			if (resultArea > maxArea)
			{
				maxArea = resultArea;
				moveRequest = fff.FindDirectionToNearestTreat();
			}
			hraciPole_Treats(fff.m_endPos) = MMLH::EOccupiedBy::None;
		}

		if (maxArea <= 5*bodySize)
		{ // oh shit...
			Panik(hraciPole_Treats, start, moveRequest, boost);
		}
	}


	void Panik(MMLH::HraciPole hraciPole, Coord head, Dir& moveRequest, bool& boost)
	{
		std::cout<< "Panik!" << std::endl;
		auto testDirection = [&](Dir dir) -> bool
		{
			Coord nextCoord = MMLH::GetCoordInDir(head, dir);
			if (nextCoord.x < Board::Cols && nextCoord.y < Board::Rows && hraciPole.IsUnoccupiedBySnake(nextCoord))
			{
				return true;
			}

			return false;
		};

		if (testDirection(Dir::Left))
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
		boost = false; boostResultCoord.x < Board::Cols&& boostResultCoord.y < Board::Rows&& hraciPole.IsUnoccupiedBySnake(boostResultCoord);
	}


	void ReportStepDuration(sf::Int64 durationMicroseconds)  override
	{
		if (durationMicroseconds > MicrosecondsLimit)
		{
			std::cout << durationMicroseconds << "\n";
		}
	}
};
