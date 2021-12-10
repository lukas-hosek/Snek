#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <deque>
#include <functional>
#include <algorithm>

struct SnekAI_PSmrcek_LSuk : public SnekAI
{
	virtual Team GetTeam() override { return Team::PSmrcekLSuk; };

	enum State
	{
		Free = 0,
		Food,
		Wall,
	};

	using StateArray = State[Board::Cols][Board::Rows];
	StateArray boardState;
	StateArray floodState;

	State& GetState(StateArray& stateArray, const Coord& coord)
	{
		return stateArray[coord.x][coord.y];
	}

	void InitFreeStateArray(StateArray& stateArray, const Board& board)
	{
		for (int x = 0; x < Board::Cols; ++x)
		{
			std::fill(stateArray[x], stateArray[x] + Board::Rows, Free);
		}

		for (auto& snekIn : board.Sneks)
		{
			for (auto& bodyCoord : snekIn->Body)
			{
				if (const_cast<Board&>(board).IsWithinBounds(bodyCoord))
				{
					GetState(stateArray, bodyCoord) = Wall;
				}
			}
		}

		for (auto& treat : board.Treats)
		{
			if (const_cast<Board&>(board).IsWithinBounds(treat.Coord))
			{
				GetState(stateArray, treat.Coord) = Food;
			}
		}
	}

	void ForEachWalkableNeighbor(StateArray& boardState, const Board& board, const Coord& origin, const std::function<void(Coord&, Dir)>& visitor)
	{
		Coord sentCoord;

		if (origin.x < board.Cols - 1)
		{
			sentCoord.x = origin.x + 1;
			sentCoord.y = origin.y;
			if (GetState(boardState, sentCoord) != Wall)
			{
				visitor(sentCoord, Right);
			}
		}
		if (origin.x > 0)
		{
			sentCoord.x = origin.x - 1;
			sentCoord.y = origin.y;
			if (GetState(boardState, sentCoord) != Wall)
			{
				visitor(sentCoord, Left);
			}
		}
		if (origin.y < board.Rows - 1)
		{
			sentCoord.x = origin.x;
			sentCoord.y = origin.y + 1;
			if (GetState(boardState, sentCoord) != Wall)
			{
				visitor(sentCoord, Down);
			}
		}
		if (origin.y > 0)
		{
			sentCoord.x = origin.x;
			sentCoord.y = origin.y - 1;
			if (GetState(boardState, sentCoord) != Wall)
			{
				visitor(sentCoord, Up);
			}
		}
	}

	void ComputeSizeRemaining(const Coord& coord, const Board& board, int& sizeRemain, int& foodRemain)
	{
		InitFreeStateArray(floodState, board);

		std::deque<Coord> openCoords;
		openCoords.push_back(coord);

		sizeRemain = 0;
		foodRemain = 0;

		while (!openCoords.empty())
		{
			auto curCoord = openCoords.front();
			openCoords.pop_front();

			if (GetState(floodState, curCoord) == Wall)
			{
				continue;
			}
			++sizeRemain;
			if (sizeRemain >= 100)
			{
				return;
			}

			ForEachWalkableNeighbor(floodState, board, curCoord, [&](Coord& coord, Dir dir)
				{
					openCoords.push_back(coord);
				});

			GetState(floodState, curCoord) = Wall;	// already went trough
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

		for (const Treat& t : board.Treats)
		{
			int manhattanDist = t.Coord.ManhattanDist(snek.Body[0]);
			if (manhattanDist > 30 || manhattanDist < 1)
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
		InitFreeStateArray(boardState, board);

		const Coord& head = snek.Body[0];
		float bestScore = 0;

		auto lowestDist = ManhattanForAll(board, snek);

		ForEachWalkableNeighbor(boardState, board, head, [&](Coord& coord, Dir dir)
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
