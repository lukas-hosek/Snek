#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
// #include "DummyAi.h"

#include <map>

inline Coord operator+(Coord& coord, Dir dir)
{
		switch (dir)
		{
		case Dir::None: return coord;
		case Dir::Left: return Coord(coord.x - 1, coord.y);
		case Dir::Right: return Coord(coord.x + 1, coord.y);
		case Dir::Up: return Coord(coord.x, coord.y - 1);
		case Dir::Down: return Coord(coord.x, coord.y + 1);
		}
		return coord;
}

enum EPhase {
		Undefined,
		Sweep,
		GoClear,
};

struct SnekAI_GMolnar_ASporka : public SnekAI
{
		Dir GetOpposite(Dir dir)
		{
				switch (dir)
				{
				case Dir::Left: return  Dir::Right;
				case Dir::Right: return Dir::Left;
				case Dir::Up: return    Dir::Down;
				case Dir::Down: return  Dir::Up;
				}
				return Dir::None;
		}

		virtual Team GetTeam() override { return Team::GMolnarASporka; };

		EPhase m_Phase = EPhase::Undefined;
		Dir m_SweepDir = Dir::Down;
		Dir m_Subphase = Dir::Up;
		Dir m_LastLateralMovement = Dir::Left;

		const int Padding = 3;
		int m_MinX = 70;
		int m_MaxX = 70;
		Coord m_Clear;

		bool IsSnakeAtPoint(const Board& board, Coord pt)
		{
				for (auto& s : board.Sneks)
				{
						for (auto& b : s->Body)
						{
								if (b.x == pt.x && b.y == pt.y) return true;
						}
				}
				return false;
		}

		bool IsTreatAtPoint(const Board& board, Coord pt)
		{
				for (auto& t : board.Treats)
				{
						if (t.Coord.x == pt.x && t.Coord.y == pt.y) return true;
				}
				return false;
		}

		bool IsTreatAtLine(const Board& board, int row, int from, int to)
		{
				if (from < to)
				{
						for (int x = from; x <= to; x++)
						{
								// Snake before treat --> avoid
								if (IsSnakeAtPoint(board, Coord(x, row))) return false;
								if (IsTreatAtPoint(board, Coord(x, row))) return true;
						}
				}
				else
				{
						for (int x = from; x >= to; x--)
						{
								// Snake before treat --> avoid
								if (IsSnakeAtPoint(board, Coord(x, row))) return false;
								if (IsTreatAtPoint(board, Coord(x, row))) return true;
						}
				}

				return false;
		}

		void SweepUp(const Board& board, const Snek& snek, Dir& moveRequest)
		{
				auto head = snek.Body[0];

				Dir nextRequest = Dir::None;

				switch (m_Subphase)
				{
				case Dir::Left:
						if (!IsTreatAtLine(board, head.y, m_MinX, head.x - 1))
						{
								m_Subphase = m_SweepDir;
								moveRequest = m_Subphase;
						}
						break;

				case Dir::Right:
						if (!IsTreatAtLine(board, head.y, head.x + 1, m_MaxX))
						{
								m_Subphase = m_SweepDir;
								moveRequest = m_Subphase;
						}
						break;

				case Dir::Up:
				case Dir::Down:
						// Treat?
						// --> change direction
						if (IsTreatAtLine(board, head.y, head.x + 1, m_MaxX))
						{
								m_Subphase = Dir::Right;
								moveRequest = m_Subphase;
								break;
						}
						else if (IsTreatAtLine(board, head.y, m_MinX, head.x - 1))
						{
								m_Subphase = Dir::Left;
								moveRequest = m_Subphase;
								break;
						}
						else
						{
								moveRequest = m_SweepDir;
						}

						if (m_SweepDir == Dir::Up)
						{
								if (head.y <= Padding + 3)
								{
										moveRequest = Dir::Right;
										m_Clear = Coord(75, Padding);
										m_Phase = EPhase::GoClear;
										break;
								}
						}
						else
						{
								if (head.y >= board.Rows - Padding - 3)
								{
										moveRequest = Dir::Left;
										m_Clear = Coord(25, board.Rows - Padding);
										m_Phase = EPhase::GoClear;
										break;
								}
						}

						break;
				}

				GetSafeDirection(board, snek, moveRequest);
		}

		////////////////////////////////////////////////////////////////
		bool IsBorder(const Board& board, const Snek& snek, Dir test_dir, bool test_boost)
		{
				auto now = snek.Body[0];
				auto would_go = now + test_dir;
				if (would_go.x < Padding) return true;
				if (would_go.x + Padding >= board.Cols) return true;
				if (would_go.y < Padding) return true;
				if (would_go.y + Padding > board.Rows) return true;
				return false;
		}

		////////////////////////////////////////////////////////////////
		bool IsSnake(const Board& board, const Snek& snek, Dir test_dir, bool test_boost)
		{
				auto now = snek.Body.front();
				auto would_go = now + test_dir;

				for (auto& s : board.Sneks)
				{
						for (auto& b : s->Body)
						{
								if (would_go.x == b.x && would_go.y == b.y) return true;
						}
				}

				return false;
		}

		////////////////////////////////////////////////////////////////
		bool IsNotSafe(const Board& board, const Snek& snek, Dir test_dir, bool test_boost)
		{
				if (IsBorder(board, snek, test_dir, test_boost)) return true;
				if (IsSnake(board, snek, test_dir, test_boost)) return true;
				return false;
		}

		////////////////////////////////////////////////////////////////
		void GetSafeDirection(const Board& board, const Snek& snek, Dir& moveRequest)
		{
				if (IsNotSafe(board, snek, moveRequest, false))
				{
						Dir d[4];
						d[0] = Left;
						d[1] = Right;
						d[2] = Up;
						d[3] = Down;
						for (int a = 0; a < 4; a++)
						{
								if (IsNotSafe(board, snek, d[a], false) == 0)
								{
										moveRequest = d[a];
										break;
								}
						}
				}
		}

		////////////////////////////////////////////////////////////////
		void SetupBounds(const Snek& snek)
		{
				auto head = snek.Body[0];
				if (head.x > 50)
				{
						m_MinX = 50;
						m_MaxX = 96;
						m_SweepDir = Dir::Down;
						m_Phase = EPhase::Sweep;
				}
				else
				{
						m_MinX = Padding;
						m_MaxX = 49;
						m_SweepDir = Dir::Up;
						m_Phase = EPhase::Sweep;
				}
		}

		////////////////////////////////////////////////////////////////
		void GoTowards(const Board& board, const Snek& snek, Coord point, Dir& moveRequest)
		{
				Coord head = snek.Body[0];

				if (head.x > point.x && snek.Heading() != Right) moveRequest = Left;
				if (head.x < point.x && snek.Heading() != Left) moveRequest = Right;
				if (head.y > point.y && snek.Heading() != Down) moveRequest = Up;
				if (head.y < point.y && snek.Heading() != Up) moveRequest = Down;

				GetSafeDirection(board, snek, moveRequest);

				Coord would_go = head + moveRequest;
				if (would_go == point)
				{
						SetupBounds(snek);
				}
		}

		////////////////////////////////////////////////////////////////
		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				if (m_Phase == EPhase::Undefined)
				{
						SetupBounds(snek);
				}
				else if (m_Phase == EPhase::GoClear)
				{
						GoTowards(board, snek, m_Clear, moveRequest);
				}
				else if (m_Phase == EPhase::Sweep)
				{
						SweepUp(board, snek, moveRequest);
				}

				if (moveRequest == Dir::Left || moveRequest == Dir::Right)
				{
						m_LastLateralMovement = moveRequest;
				}
		}
};
