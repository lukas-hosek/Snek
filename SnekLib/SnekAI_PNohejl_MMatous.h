#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"

struct SnekAI_PNohejl_MMatous : public SnekAI
{
		virtual Team GetTeam() override { return Team::PNohejlMMatous; };

		const int TYPE_OF_MOVEMENT = 1;
		const int TYPE_OF_BOOST = 1;
		int boostCounter = 0;

		struct DirectionCluster
		{
				Dir Direction = Dir::None;
				float TreatsDistScaled = 0.f;
				float OurSnekDanger = 0.f;
				float EnemyDanger = 0.f; // How???

				float GetScore() const
				{
						return TreatsDistScaled - EnemyDanger;
				}
		};

		using T_Clusters = std::unordered_map<Dir, DirectionCluster>;
		using T_ClustersSorted = std::vector<DirectionCluster>;
		using T_BestDirections = std::vector<Dir>;

		inline Dir GetDirection(const Coord& coord, const Snek& snek)
		{
				Coord snakeHead = snek.Body[0];

				int xOffset = coord.x - snakeHead.x;
				int yOffset = coord.y - snakeHead.y;

				if (std::abs(yOffset) < std::abs(xOffset))
				{
						// Side

						if (xOffset < 0)
						{
								return Dir::Left;
						}
						else if (xOffset > 0)
						{
								return Dir::Right;
						}
				}
				else
				{
						// Up/Down

						if (yOffset < 0)
						{
								return Dir::Up;
						}
						else if (yOffset > 0)
						{
								return Dir::Down;
						}
				}

				return (xOffset < 0) ? Dir::Left : Dir::Right;
		}

		inline T_Clusters GetClusters(const Board& board, const Snek& ourSnek)
		{
				T_Clusters clusters;

				Coord snakeHead = ourSnek.Body[0];

				Coord center{ board.Cols / 2, board.Rows / 2 };
				for (Treat treat : board.Treats)
				{
						Dir dir = GetDirection(treat.Coord, ourSnek);
						clusters[dir].Direction = dir;
						int dist = std::max(treat.Coord.ManhattanDist(snakeHead), 1);

						float cornerCoeff = 1.f;
						if (treat.Coord.x < 3 || treat.Coord.y < 3 || treat.Coord.x > board.Cols - 4 || treat.Coord.y > board.Rows - 4)
						{
								cornerCoeff = 0.f;
						}

						clusters[dir].TreatsDistScaled += 1.f / (dist * dist) * cornerCoeff;
				}

				for (auto& snek : board.Sneks)
				{
						for (auto coord : ourSnek.Body)
						{
								int distCheck = 20;
								if (snek->ID == ourSnek.ID)
								{
										distCheck = 5;
								}

								int dist = coord.ManhattanDist(snakeHead);
								if (dist < distCheck)
								{
										Dir dir = GetDirection(coord, ourSnek);
										clusters[dir].EnemyDanger += 0.05f;
								}
						}
				}

				return clusters;
		}

		inline T_BestDirections GetBestDirection(const Board& board, const Snek& snek)
		{
				T_Clusters clusters = GetClusters(board, snek);

				T_ClustersSorted clustersSorted;
				for (auto& [dir, cluster] : clusters)
				{
						clustersSorted.push_back(cluster);
				}

				std::sort(clustersSorted.begin(), clustersSorted.end(), [](const auto& a, const auto& b)
						{
								return a.GetScore() > b.GetScore();
						});

				T_BestDirections directions;
				for (auto& cluster : clustersSorted)
				{
						directions.push_back(cluster.Direction);
				}

				return directions;
		}

		inline int isSnakeCoord(Coord c, const std::vector<Coord>& body)
		{
				auto it = std::find(body.rbegin(), body.rend(), c);
				return it == body.rend();
		}

		inline bool checkBoardBounds(const Board& board, Coord predictedCoord)
		{
				return board.IsWithinBounds(predictedCoord);
		}

		inline bool checkSneks(const Board& board, Coord predictedCoord)
		{
				for (auto& s : board.Sneks)
				{
						if (!isSnakeCoord(predictedCoord, s->Body))
								return false;
				}
				return true;
		}

		inline bool checkCollisions(Coord head, const Board& board, Dir moveRequest)
		{
				auto predictedCoord = head;
				if (moveRequest == Right) predictedCoord.x++;
				if (moveRequest == Left) predictedCoord.x--;
				if (moveRequest == Up) predictedCoord.y--;
				if (moveRequest == Down) predictedCoord.y++;
				return checkSneks(board, predictedCoord) && checkBoardBounds(board, predictedCoord);
		}

		inline bool checkBackDirection(const Dir& previousDirection, const Dir& possibleDirection)
		{
				if (previousDirection == Left && possibleDirection == Right
						|| previousDirection == Right && possibleDirection == Left
						|| previousDirection == Up && possibleDirection == Down
						|| previousDirection == Down && possibleDirection == Up) {
						return false;
				}
				return true;
		}

		inline T_BestDirections getPossibleMoves(const Board& board, const Coord& head, const Snek& snek) {
				T_BestDirections possibleMoves;
				if (checkCollisions(head, board, Left) && checkBackDirection(snek.Heading(), Left)) possibleMoves.push_back(Left);
				if (checkCollisions(head, board, Right) && checkBackDirection(snek.Heading(), Right)) possibleMoves.push_back(Right);
				if (checkCollisions(head, board, Up) && checkBackDirection(snek.Heading(), Up)) possibleMoves.push_back(Up);
				if (checkCollisions(head, board, Down) && checkBackDirection(snek.Heading(), Down)) possibleMoves.push_back(Down);
				return possibleMoves;
		}

		inline bool inPossibleMoves(T_BestDirections moves, const Dir& move)
		{
				return std::find(moves.begin(), moves.end(), move) != moves.end();
		}

		inline bool advancedMovement(const Board& board, const Snek& snek, Dir& moveRequest, T_BestDirections possibleMoves)
		{
				auto bestDirections = GetBestDirection(board, snek);
				for (auto direction : bestDirections)
				{
						if (inPossibleMoves(possibleMoves, direction))
						{
								moveRequest = direction;
								return true;
						}
				}
				return false;
		}

		inline bool basicMovement(const Snek& snek, Dir& moveRequest, Coord head, Coord favorite, T_BestDirections possibleMoves)
		{
				if (head.x > favorite.x && snek.Heading() != Right && inPossibleMoves(possibleMoves, Left))
				{
						moveRequest = Left;
						return true;
				}
				if (head.x < favorite.x && snek.Heading() != Left && inPossibleMoves(possibleMoves, Right))
				{
						moveRequest = Right;
						return true;
				}
				if (head.y > favorite.y && snek.Heading() != Down && inPossibleMoves(possibleMoves, Up)) {
						moveRequest = Up;
						return true;
				}
				if (head.y < favorite.y && snek.Heading() != Up && inPossibleMoves(possibleMoves, Down)) {
						moveRequest = Down;
						return true;
				}
				return false;
		}

		inline bool isEnemyNear(const Board& board, const Coord& head, const Snek& ourSnek)
		{
				int gridSize = 4;
				auto gridCoord = head;
				for (int i = -gridSize; i <= gridSize; i++)
				{
						gridCoord.x = head.x + i;
						for (int j = -gridSize; j <= gridSize; j++)
						{
								gridCoord.y = head.y + j;
								for (auto& s : board.Sneks)
								{
										if (s->ID == ourSnek.ID) // if our snek, skip
												continue;
										if (!isSnakeCoord(gridCoord, s->Body))
												return true;
								}
						}
				}
				return false;
		}

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				Coord head = snek.Body[0];
				auto it = std::min_element(board.Treats.begin(), board.Treats.end(),
						[&](Treat t1, Treat t2) { return t1.Coord.ManhattanDist(head) < t2.Coord.ManhattanDist(head); }); //noice
				Coord favorite = it->Coord;
				auto possibleMoves = getPossibleMoves(board, head, snek);

				bool hasMoved = false;
				if (TYPE_OF_MOVEMENT == 0)
				{
						hasMoved = basicMovement(snek, moveRequest, head, favorite, possibleMoves);
				}
				else if (TYPE_OF_MOVEMENT == 1)
				{
						hasMoved = advancedMovement(board, snek, moveRequest, possibleMoves);
				}

				if (hasMoved == false) {
						if (!possibleMoves.empty())
								moveRequest = possibleMoves.front();
						// else: jsme v pici
				}

				if (TYPE_OF_BOOST == 0)
						boost = head.ManhattanDist(favorite) > 9;
				else if (TYPE_OF_BOOST == 1)
						boost = isEnemyNear(board, head, snek);

				//check for frequent boosts
				if (boost && boostCounter < 6) {
						boostCounter += 3;
				}
				else if (boostCounter > 0) {
						boostCounter--;
				}
		}
};
