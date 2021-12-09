#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <algorithm>

struct SnekAI_PMan_MSourek : public SnekAI
{
		enum Strategy
		{
				Hungry
		};

		struct S_MoveDir
		{
				Dir		m_Dir = Dir::Up;
				float m_Value = 0.f;
				Coord	m_Pos = { 0, 0 };
				int		m_TargetDist = 0;
		};

		struct BBox
		{
				Coord leftTop;
				Coord rightBottom;
				int GetSize()
				{
						return (rightBottom.x - leftTop.x + 1) * (rightBottom.y - leftTop.y + 1);
				}

				bool Intersect(const BBox& bb)
				{
						return !(bb.leftTop.x > rightBottom.x
								|| bb.rightBottom.x < leftTop.x
								|| bb.leftTop.y > rightBottom.y
								|| bb.rightBottom.y < leftTop.y);
				}

				void Absorb(const Coord& c)
				{
						if (c.x < leftTop.x)
								leftTop.x = c.x;
						else if (c.x > rightBottom.x)
								rightBottom.x = c.x;

						if (c.y < leftTop.y)
								leftTop.y = c.y;
						else if (c.y > rightBottom.y)
								rightBottom.y = c.y;
				}

				int Dist(const Coord& c)
				{
						if (c.x >= leftTop.x && c.x <= rightBottom.x
								&& c.y >= leftTop.y && c.y <= rightBottom.y)
						{
								return 0;
						}

						int dx1 = std::abs(int(leftTop.x) - int(c.x));
						int dx2 = std::abs(int(rightBottom.x) - int(c.x));
						int dy1 = std::abs(int(leftTop.y) - int(c.y));
						int dy2 = std::abs(int(rightBottom.y) - int(c.y));
						int minX = std::min(dx1, dx2);
						int minY = std::min(dy1, dy2);
						return minX + minY;
				}
		};

		struct Enemy
		{
				BBox	m_BBox;
				float m_Threat = 0; // Zatim asi neres
				float m_Dist = 0; // Neares dist from my head to his body

		};

		struct FreeSpace
		{
				BBox	m_BBox;
				float m_BBoxSize = 0; // Sort array
				float m_Dist = 0;			// Dist to my snake.

		};

		//==============================================================================
		void AnalyzeGame(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
				// TODO tady to napis
		}

		//==============================================================================
		//==============================================================================
		//==============================================================================
		//==============================================================================

		using T_MoveDirs = std::vector<S_MoveDir>;

		virtual Team GetTeam() override { return Team::PManMSourek; };

		//==============================================================================
		void AnalyzeMySnake(const Snek& snek)
		{
				m_MySnakeBBox.leftTop = snek.Body[0];
				m_MySnakeBBox.rightBottom = snek.Body[0];
				for (auto& body : snek.Body)
				{
						m_MySnakeBBox.Absorb(body);
				}
		}

		//==============================================================================
		void SelectStrategy(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
				m_Strategy = Strategy::Hungry;
		}

		struct S_Target
		{
				Coord m_Pos;
				int		m_Dist = 0;
				float m_Value = 0;
		};

		struct S_Vector
		{
				float x, y;
				S_Vector(float a, float b) : x(a), y(b) {}
				S_Vector(const Coord& c) : x((float)c.x), y((float)c.y) {}
				void Sub(const S_Vector& v)
				{
						x -= v.x;
						y -= v.y;
				}
				float GetLen()
				{
						return sqrt(x * x + (y * y));
				}
				void Normalize()
				{
						auto len = GetLen();
						x = x / len;
						y = y / len;
				}
				float Dot(const S_Vector& v)
				{
						return x * v.x + y * v.y;
				}
		};

		//==============================================================================
		float EvaluateTarget(const Treat& treat, S_Target& target, const Board& board, const Snek& snek)
		{
				// Dir penalty
				auto& head = snek.Body[0];
				S_Vector headV(head);
				S_Vector dirToTreat(treat.Coord);
				dirToTreat.Sub(headV);
				dirToTreat.Normalize();

				S_Vector snakeBody(S_Vector(snek.Body[1]));
				S_Vector snakeDir = headV;
				snakeDir.Sub(snakeBody);
				snakeDir.Normalize();

				auto dot = snakeDir.Dot(dirToTreat);
				auto dirNorm = (-dot + 1.f) * 0.5f;
				float randPenalty = float(std::rand()) / float(RAND_MAX);
				float maxDirPenalty = 10.f + randPenalty * 5.f;
				float dirPenalty = dirNorm * 10.f * dirNorm;

				// Dist to my snake penalty
				auto mySnakeDist = m_MySnakeBBox.Dist(target.m_Pos);
				auto maxMySnakeDistPenaltySize = 15.f;
				auto mySnakeDistPenaltyVal = 15.f;
				auto mySnakeDistClamped = std::clamp(float(mySnakeDist), 0.f, maxMySnakeDistPenaltySize);
				float mySnakeDistPenalty = mySnakeDistPenaltyVal * (1.f - float(mySnakeDistClamped) / maxMySnakeDistPenaltySize); // Far is better
				if (mySnakeDist < 1)
				{
						mySnakeDistPenalty += 1000.f;
				}

				return target.m_Dist + mySnakeDistPenalty + dirPenalty;
		}

		//==============================================================================
		Coord FindTarget(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
				auto& head = snek.Body[0];
				std::vector<S_Target> targets;
				targets.resize(board.Treats.size());
				int i = 0;
				for (auto& treat : board.Treats)
				{
						auto& target = targets[i];
						target.m_Dist = treat.Coord.ManhattanDist(head);
						target.m_Pos = treat.Coord;
						target.m_Value = EvaluateTarget(treat, target, board, snek);
						++i;
				}

				std::sort(targets.begin(), targets.end(), [](auto& a, auto& b)
						{
								return a.m_Value < b.m_Value;
						});

				if (targets.size())
				{
						return targets.front().m_Pos;
				}


				// 		if (m_Strategy == Strategy::Hungry)
				// 		{
				// 			auto it = std::min_element(board.Treats.begin(), board.Treats.end(),
				// 				[&](Treat t1, Treat t2) { return t1.Coord.ManhattanDist(head) < t2.Coord.ManhattanDist(head); });
				// 			return it->Coord;
				// 		}
				return head;
		}

		//==============================================================================
		int	GetCollisionsInBBox(const BBox& bbox, const Board& board, bool coverBoardBounds = true)
		{
				auto lx = bbox.rightBottom.x - bbox.leftTop.x + 1;
				auto ly = bbox.rightBottom.y - bbox.leftTop.y + 1;
				int collisions = 0;
				for (unsigned x = 0; x < lx; ++x)
				{
						for (unsigned y = 0; y < ly; ++y)
						{
								Coord pos;
								pos.x = x + bbox.leftTop.x;
								pos.y = y + bbox.leftTop.y;

								if (IsObstacleOnCoord(pos, board, coverBoardBounds))
								{
										++collisions;
								}
						}
				}
				return collisions;
		}

		//==============================================================================
		bool IsObstacleOnCoord(const Coord& pos, const Board& board, bool coverBoardBounds = true)
		{
				if (coverBoardBounds && !board.IsWithinBounds(pos))
				{
						return true;
				}

				if (!board.IsFree(pos))
				{
						if (std::find_if(board.Treats.begin(), board.Treats.end(), [&](auto& t) {return t.Coord == pos; }) != board.Treats.end())
						{
								return false;
						}
						return true;
				}
				return false;
		}

		//==============================================================================
		Coord GetCoordInDir(const Coord& start, Dir dir)
		{
				auto p = start;
				switch (dir)
				{
				case Left:--p.x; break;
				case Right:++p.x;	break;
				case Up:--p.y; break;
				case Down:++p.y; break;
				default:
						break;
				}
				return p;
		}

		//==============================================================================
		void ApplyStrategyForDir(T_MoveDirs& dirs, const Coord& target, const Board& board, const Snek& snek, bool& boost)
		{
				auto& headPos = snek.Body[0];
				if (m_Strategy == Strategy::Hungry)
				{
						for (auto& dir : dirs)
						{
								// Dist metric
								dir.m_Value = static_cast<float>(dir.m_TargetDist);

								// Coll metric
								int s = 1;
								BBox critArea;
								critArea.leftTop = dir.m_Pos;
								critArea.rightBottom = dir.m_Pos;
								int rx, ry, lx, ly;
								rx = lx = dir.m_Pos.x;
								ry = ly = dir.m_Pos.y;
								switch (dir.m_Dir)
								{
								case Left:	lx -= s; ly += s; rx -= s; ry -= s; break;
								case Right:	lx += s; ly += s; rx += s; ry -= s; break;
								case Up:		lx -= s; ly -= s; rx += s; ry -= s; break;
								case Down:	lx -= s; ly += s; rx += s; ry += s; break;
								}
								lx = std::clamp(lx, 0, (int)board.Cols); rx = std::clamp(rx, 0, (int)board.Cols);
								ly = std::clamp(ly, 0, (int)board.Rows); ry = std::clamp(ry, 0, (int)board.Rows);
								critArea.Absorb(Coord(lx, ly));
								critArea.Absorb(Coord(rx, ry));
								auto maxSize = critArea.GetSize();
								auto cols = GetCollisionsInBBox(critArea, board);
								float colsRate = static_cast<float>(cols) / static_cast<float>(maxSize);
								if (cols)
								{
										dir.m_Value = 10.f + colsRate * 10.f;
								}
						}
				}

				// Sort
				std::sort(dirs.begin(), dirs.end(), [](auto& a, auto& b) {
						return a.m_Value > b.m_Value;
						});
		}

		//==============================================================================
		void FindNextMove(const Coord& target, const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
				auto& headPos = snek.Body[0];
				std::vector<S_MoveDir> dirs = { {Right}, {Left}, {Down}, {Up} };
				for (auto& dir : dirs)
				{
						dir.m_Pos = GetCoordInDir(headPos, dir.m_Dir);
						dir.m_TargetDist = target.ManhattanDist(dir.m_Pos);
				}
				ApplyStrategyForDir(dirs, target, board, snek, boost);

				while (dirs.size())
				{
						auto& dirAttemp = dirs.back();
						if (!IsObstacleOnCoord(dirAttemp.m_Pos, board))
						{
								moveRequest = dirAttemp.m_Dir;
								return;
						}
						dirs.pop_back();
				}
				moveRequest = Dir::Up;
		}

		//==============================================================================
		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				AnalyzeMySnake(snek);
				AnalyzeGame(board, snek, moveRequest, boost);
				SelectStrategy(board, snek, moveRequest, boost);

				auto target = FindTarget(board, snek, moveRequest, boost);
				FindNextMove(target, board, snek, moveRequest, boost);
		}

		// Analyze
		std::vector<Enemy>	m_Enemies;		// Sortovat podle velikosti
		std::vector<BBox>		m_FreeSpaces; // Sortovat podle velikosti
		std::vector<Coord>	m_SnakeHeads;	// Sortovat podle vzdalesti od moji hlavy

		// Snake
		BBox m_MySnakeBBox;

		// Strategy
		Strategy m_Strategy = Strategy::Hungry;

};
