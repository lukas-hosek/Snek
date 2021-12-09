#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <map>


struct SnekAI_TGrunwald_DMikes : public SnekAI
{
		virtual Team GetTeam() override { return Team::TGrunwaldDMikes; };

		struct Vec2
		{
				int x, y;

				Coord toCoord()
				{
						return { static_cast<unsigned>(x), static_cast<unsigned>(y) };
				}
		};

		std::map<Dir, Vec2> dirToVec2;
		//==============================================================================
		Vec2 Add(Vec2 a, Vec2 b)
		{
				Vec2 c;
				c.x = a.x + b.x;
				c.y = a.y + b.y;
				return c;
		}

		//==============================================================================
		Coord Add(Coord a, Vec2 b)
		{
				Coord c;
				c.x = a.x + b.x;
				c.y = a.y + b.y;
				return c;
		}

		//==============================================================================
		void Init(const Board& board, const Snek& snek)
		{
				dirToVec2[Left] = { -1, 0 };
				dirToVec2[Right] = { 1, 0 };
				dirToVec2[Up] = { 0,-1 };
				dirToVec2[Down] = { 0, 1 };
		}

		//==============================================================================
		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				if (dirToVec2.empty())
				{
						Init(board, snek);
				}

				Coord head = snek.Body[0];
				auto it = std::min_element(board.Treats.begin(), board.Treats.end(),
						[&](Treat t1, Treat t2)
						{
								return t1.Coord.ManhattanDist(head) < t2.Coord.ManhattanDist(head);
						});

				Coord favorite = it->Coord;
				if (head.x > favorite.x && snek.Heading() != Right) moveRequest = Left;
				if (head.x < favorite.x && snek.Heading() != Left) moveRequest = Right;
				if (head.y > favorite.y && snek.Heading() != Down) moveRequest = Up;
				if (head.y < favorite.y && snek.Heading() != Up) moveRequest = Down;

				//--------------------
				int size = 1;
				Vec2 headVec = { snek.Body[0].x, snek.Body[0].y };
				int maxSum = (2 * size + 1) * (2 * size + 1);
				int minSum = maxSum;
				Dir defenceDir = None;


				auto coord = Add(headVec, dirToVec2[moveRequest]);
				int attackSum = SumSurrounding(board, coord, size);

				///------------------

				for (const auto& pair : dirToVec2)
				{
						auto coord = Add(headVec, pair.second);
						int sum = SumSurrounding(board, coord, size);
						if (sum < minSum)
						{
								minSum = sum;
								defenceDir = pair.first;
						}
				}

				if (attackSum > 2)
				{
						moveRequest = defenceDir;
				}
				//moveRequest = MinimalWeightStep(board, snek);
		}


		//==============================================================================
		bool IsFreeOrTreat(const Board& board, Vec2 vec)
		{
				Coord c = vec.toCoord();
				if (std::find_if(board.Treats.begin(), board.Treats.end(), [&](auto& t) {return t.Coord == c; }) != board.Treats.end())
				{
						return true;
				}

				if (board.IsFree(c))
				{
						return true;
				}

				return false;
		}

		//==============================================================================
		// is withing bound with signed coord
		bool IsBoardWithinBounds(const Board& board, Vec2 c) const
		{
				return c.x >= 0 && c.x < board.Cols&& c.y >= 0 && c.y < board.Rows;
		}

		//==============================================================================
		bool IsCollision(const Board& board, Vec2 vec)
		{
				if (!IsBoardWithinBounds(board, vec))
				{
						return true;
				}

				Coord c = vec.toCoord();
				if (std::find_if(board.Treats.begin(), board.Treats.end(), [&](auto& t) {return t.Coord == c; }) != board.Treats.end())
				{
						return false;
				}

				return !board.IsFree(vec.toCoord());
		}

		//==============================================================================
		unsigned SumSurrounding(const Board& board, Vec2 vec, int size)
		{
				if (IsCollision(board, vec))
				{
						return (2 * size + 1) * (2 * size + 1);
				}

				int sum = 0;
				Vec2 pointer;
				for (int y = -size; y <= size; y++)
				{
						for (int x = -size; x <= size; x++)
						{
								pointer = vec;
								pointer.x += x;
								pointer.y += y;

								sum += static_cast<int>(IsCollision(board, pointer));
						}
				}

				return sum;
		}

		//==============================================================================
		/*Dir MinimalWeightStep(const Board& board, const Snek& snek)
		{
			Vec2 head = { snek.Body[0].x, snek.Body[0].y };
			int minSum = 10; // higher than 9 = max value
			Dir dir = None;

			for(const auto& pair : dirToVec2)
			{
				auto coord = Add(head, pair.second);
				int sum = SumSurrounding8(board, coord);
				if (sum < minSum)
				{
					minSum = sum;
					dir = pair.first;
				}
			}

			return dir;
		}
		*/

		//==============================================================================
		void CheckBoardBorderCollision(const Board& board, const Snek& snek)
		{

		}
};

