#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <deque>
#include <functional>

struct SnekAI_PSmrcek_LSuk : public SnekAI
{
		virtual Team GetTeam() override { return Team::PSmrcekLSuk; };

		enum State
		{
				Free = 0,
				Food,
				Wall,
		};

		State* curState = nullptr;
		State& GetState(const Board& board, const Coord& coord)
		{
				return curState[coord.x * board.Rows + coord.y];
		}

		void ForEachWalkableNeighbor(const Board& board, Coord& origin, const std::function<void(Coord&, Dir)>& visitor)
		{
				Coord sentCoord;

				if (origin.x < board.Cols - 1)
				{
						sentCoord.x = origin.x + 1;
						sentCoord.y = origin.y;
						if (GetState(board, sentCoord) != Wall)
						{
								visitor(sentCoord, Right);
						}
				}
				if (origin.x > 0)
				{
						sentCoord.x = origin.x - 1;
						sentCoord.y = origin.y;
						if (GetState(board, sentCoord) != Wall)
						{
								visitor(sentCoord, Left);
						}
				}
				if (origin.y < board.Rows - 1)
				{
						sentCoord.x = origin.x;
						sentCoord.y = origin.y + 1;
						if (GetState(board, sentCoord) != Wall)
						{
								visitor(sentCoord, Down);
						}
				}
				if (origin.y > 0)
				{
						sentCoord.x = origin.x;
						sentCoord.y = origin.y - 1;
						if (GetState(board, sentCoord) != Wall)
						{
								visitor(sentCoord, Up);
						}
				}

		}

		void ComputeSizeRemaining(const Coord& coord, const Board& board, int& sizeRemain, int& foodRemain)
		{
				sizeRemain = 0;
				foodRemain = 0;
				State usedCoords[board.Cols][board.Rows] = { Free };
				std::deque<Coord> openCoords;
				openCoords.push_back(coord);

				for (auto& snek : board.Sneks)
				{
						for (auto& bodyCoord : snek->Body)
						{
								usedCoords[bodyCoord.x][bodyCoord.y] = Wall;
						}
				}

				for (auto& treat : board.Treats)
				{
						usedCoords[treat.Coord.x][treat.Coord.y] = Food;
				}

				while (!openCoords.empty())
				{
						auto& curCoord = openCoords.front();

						if (usedCoords[curCoord.x][curCoord.y] == Wall)
						{
								openCoords.pop_front();
								continue;
						}
						++sizeRemain;
						if (sizeRemain >= 100)
						{
								return;
						}

						if (curCoord.x > 0 && usedCoords[curCoord.x - 1][curCoord.y] != Wall)
						{
								openCoords.push_back({ curCoord.x - 1, curCoord.y });
						}
						if (curCoord.x < board.Cols - 1 && usedCoords[curCoord.x + 1][curCoord.y] != Wall)
						{
								openCoords.push_back({ curCoord.x + 1, curCoord.y });
						}
						if (curCoord.y < board.Rows - 1 && usedCoords[curCoord.x][curCoord.y + 1] != Wall)
						{
								openCoords.push_back({ curCoord.x, curCoord.y + 1 });
						}
						if (curCoord.y > 0 && usedCoords[curCoord.x][curCoord.y - 1] != Wall)
						{
								openCoords.push_back({ curCoord.x, curCoord.y - 1 });
						}

						usedCoords[curCoord.x][curCoord.y] = Wall;	// already went trough
						openCoords.pop_front();
				}

				if (sizeRemain < 100)
				{
						foodRemain = 1000;
				}

		}

		float foodRating[5];

		Dir getDirToPoint(const Snek& snek, Coord point)
		{
				Coord head = snek.Body[0];

				int xDiff = head.x - static_cast<int>(point.x);
				int yDiff = head.y - static_cast<int>(point.y);

				if (std::abs(xDiff) > std::abs(yDiff))
				{
						if (xDiff > 0)
						{
								return Left;
						}
						else
						{
								return Right;
						}
				}
				else
				{
						if (yDiff > 0)
						{
								return Up;
						}
						else
						{
								return Down;
						}
				}
		}

		int ManhattanForAll(const Board& board, const Snek& snek)
		{
				float left = 0;
				float right = 0;
				float up = 0;
				float down = 0;
				int lowestDist = 1000;

				for (Treat t : board.Treats)
				{
						int manhattanDist = t.Coord.ManhattanDist(snek.Body[0]);
						if (manhattanDist > 30)
						{
								continue;
						}
						lowestDist = std::min(lowestDist, manhattanDist);

						switch (getDirToPoint(snek, t.Coord))
						{
						case Left:
								left += 1.f / manhattanDist;
								break;
						case Right:
								right += 1.f / manhattanDist;
								break;
						case Up:
								up += 1.f / manhattanDist;
								break;
						case Down:
								down += 1.f / manhattanDist;
								break;
						default:
								break;
						}
				}

				foodRating[Left] = left;
				foodRating[Right] = right;
				foodRating[Up] = up;
				foodRating[Down] = down;

				return lowestDist;
		}

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				Coord head = snek.Body[0];

				State usedCoords[board.Cols * board.Rows] = { Free };
				curState = usedCoords;
				for (auto& snek : board.Sneks)
				{
						for (auto& bodyCoord : snek->Body)
						{
								GetState(board, bodyCoord) = Wall;
						}
				}

				for (auto& treat : board.Treats)
				{
						GetState(board, treat.Coord) = Food;
				}

				float bestScore = 0;

				auto lowestDist = ManhattanForAll(board, snek);

				ForEachWalkableNeighbor(board, head, [&](Coord& coord, Dir dir)
						{
								int sizeRemaining = 0;
								int foodRemaining = 0;
								ComputeSizeRemaining(coord, board, sizeRemaining, foodRemaining);

								float score = sizeRemaining + foodRating[dir];

								if (score > bestScore)
								{
										bestScore = score;
										moveRequest = dir;
								}
						});

				boost = false;
		}
};
