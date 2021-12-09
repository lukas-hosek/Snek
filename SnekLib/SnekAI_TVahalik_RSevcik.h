#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <assert.h>

struct SnekAI_TVahalik_RSevcik : public SnekAI
{
		virtual Team GetTeam() override { return Team::TVahalikRSevcik; }

		//===========================================================
		enum E_Quadrants
		{
				NW,
				NE,
				SW,
				SE
		};

		//===========================================================
		class C_Result
		{
		public:
				float	m_values[Down + 1];

				C_Result()
				{
						memset(m_values, 0, sizeof(m_values));
				}

				void Add(const C_Result& other, float weight)
				{
						for (int i = Left; i <= Down; i++)
						{
								m_values[i] += other.m_values[i] * weight;
						}
				}
		};


		int tick = 0;
		Coord m_ActiveQuadrantCoord;
		E_Quadrants m_ActiveQuadrant;

		const float NoGoResult = -FLT_MAX;

		//===========================================================
		Coord Add(const Coord& a, const Coord& b)
		{
				Coord res;
				res.x = a.x + b.x;
				res.y = a.y + b.y;

				return res;
		}

		//===========================================================
		Coord Sub(const Coord& a, const Coord& b)
		{
				Coord res;
				res.x = a.x - b.x;
				res.y = a.y - b.y;

				return res;
		}

		//===========================================================
		Coord Step(Coord pos, Dir dir)
		{
				switch (dir)
				{
				case Left:
						pos.x--;
						break;

				case Right:
						pos.x++;
						break;

				case Up:
						pos.y--;
						break;

				case Down:
						pos.y++;
						break;

				default:
						assert(0);
						break;
				}

				return pos;
		}

		//===========================================================
		bool IsSnek(const Board& board, const Coord& pos)
		{
				for (const auto& s : board.Sneks)
				{
						if (s->IsDeath())
						{
								continue;
						}

						auto it = std::find(s->Body.rbegin(), s->Body.rend(), pos);
						if (it != s->Body.rend())
						{
								return true;
						}
				}

				return false;
		}

		//===========================================================
		bool IsFood(const Board& board, const Coord& coord)
		{
				if (board.IsFree(coord))
				{
						return false;
				}
				return (std::find_if(board.Treats.begin(), board.Treats.end(), [&](auto& t) {return t.Coord == coord; }) != board.Treats.end());
		}

		//===========================================================
		class C_SnekBox
		{
		public:
				unsigned int		m_Id;
				unsigned int		m_MinX;
				unsigned int		m_MaxX;
				unsigned int		m_MinY;
				unsigned int		m_MaxY;

				C_SnekBox()
				{
						Clear();
				}

				void Clear()
				{
						m_MinX = INT_MAX;
						m_MaxX = 0;
						m_MinY = INT_MAX;
						m_MaxY = 0;
				}

				bool IsValid() const
				{
						return m_MinX <= m_MaxX;
				}

				void Update(const Snek& snek)
				{
						Clear();

						m_Id = snek.ID;
						if (snek.IsDeath())
						{
								return;
						}

						for (const auto& it : snek.Body)
						{
								if (m_MinX > it.x)
								{
										m_MinX = it.x;
								}

								if (m_MaxX < it.x)
								{
										m_MaxX = it.x;
								}

								if (m_MinY > it.y)
								{
										m_MinY = it.y;
								}

								if (m_MaxY < it.y)
								{
										m_MaxY = it.y;
								}
						}
				}

				Coord GetDiff(const Coord& pos) const
				{
						Coord diff;

						if (pos.x < m_MinX)
						{
								diff.x = m_MinX - pos.x;
						}
						else if (pos.x > m_MinX)
						{
								diff.x = pos.x - m_MaxX;
						}
						else
						{
								diff.x = 0;
						}

						if (pos.y < m_MinY)
						{
								diff.y = m_MinY - pos.y;
						}
						else if (pos.y > m_MinX)
						{
								diff.y = pos.y - m_MaxY;
						}
						else
						{
								diff.y = 0;
						}

						return diff;
				}
		};

		std::vector<C_SnekBox>	m_SnekBoxes;
		unsigned int						m_LastSnekBoxUpdated = 0;

		//===========================================================
		C_Result StrategySurvive(const Board& board, const Snek& snek)
		{
				C_Result res;
				Coord src = snek.Body[0];

				for (int dirIdx = Left; dirIdx <= Down; ++dirIdx)
				{
						Dir dir = static_cast<Dir>(dirIdx);

						// kam vubec nelezt
						Coord dest = Step(src, dir);
						if (!board.IsWithinBounds(dest)
								|| IsSnek(board, dest))
						{
								res.m_values[dirIdx] = NoGoResult;
								continue;
						}
				} // for dirIdx

				// bonus za vzdalenost ke zdi
				const float wallDistBonus = 0.01f;
				if (res.m_values[Left] != NoGoResult)
				{
						res.m_values[Left] += src.x * wallDistBonus;
				}

				if (res.m_values[Right] != NoGoResult)
				{
						res.m_values[Right] += (Board::Cols - 1 - src.x) * wallDistBonus;
				}

				if (res.m_values[Up] != NoGoResult)
				{
						res.m_values[Up] += src.y * wallDistBonus;
				}

				if (res.m_values[Down] != NoGoResult)
				{
						res.m_values[Down] += (Board::Rows - 1 - src.x) * wallDistBonus;
				}

				if (m_StrikeCount < 2)
				{
						// malus za vzdalenost k jinym hadum
						for (const auto& snekBox : m_SnekBoxes)
						{
								if (snekBox.m_Id == snek.ID)
								{
										continue;
								}

								if (!snekBox.IsValid())
								{
										continue;
								}

								const Coord diff = snekBox.GetDiff(src);
								const float snakeBoxMalus = 0.002f;
								if ((res.m_values[Left] != NoGoResult)
										&& (diff.x <= 0))
								{
										res.m_values[Left] -= (Board::Cols + diff.x) * snakeBoxMalus;
								}

								if ((res.m_values[Right] != NoGoResult)
										&& (diff.x >= 0))
								{
										res.m_values[Right] -= (Board::Cols - diff.x) * snakeBoxMalus;
								}

								if ((res.m_values[Up] != NoGoResult)
										&& (diff.y <= 0))
								{
										res.m_values[Up] -= (Board::Rows + diff.y) * snakeBoxMalus;
								}

								if ((res.m_values[Down] != NoGoResult)
										&& (diff.y >= 0))
								{
										res.m_values[Down] -= (Board::Rows - diff.y) * snakeBoxMalus;
								}
						} // for snekBox

						// pryc sam od sebe
						for (const auto& snekBox : m_SnekBoxes)
						{
								if (snekBox.m_Id != snek.ID)
								{
										continue;
								}

								const float mySnakeBoxMalus = 0.005f;

								const unsigned int midX = (snekBox.m_MaxX + snekBox.m_MinX) / 2;
								const unsigned int sizeX = snekBox.m_MaxX - snekBox.m_MinX;
								const unsigned int diffX = src.x - midX;

								if ((res.m_values[Left] != NoGoResult)
										&& (diffX < 0))
								{
										res.m_values[Left] -= (sizeX + diffX) * mySnakeBoxMalus;
								}

								if ((res.m_values[Right] != NoGoResult)
										&& (diffX > 0))
								{
										res.m_values[Right] -= (sizeX - diffX) * mySnakeBoxMalus;
								}

								const unsigned int midY = (snekBox.m_MaxY + snekBox.m_MinY) / 2;
								const unsigned int sizeY = snekBox.m_MaxY - snekBox.m_MinY;
								const unsigned int diffY = src.y - midY;

								if ((res.m_values[Up] != NoGoResult)
										&& (diffY < 0))
								{
										res.m_values[Up] -= (sizeY + diffY) * mySnakeBoxMalus;
								}

								if ((res.m_values[Down] != NoGoResult)
										&& (diffY > 0))
								{
										res.m_values[Down] -= (sizeY - diffY) * mySnakeBoxMalus;
								}

						}	// for snekBox
				}

				return res;
		}

		//===========================================================
		C_Result StrategyFeed(const Board& board, const Snek& snek)
		{
				//zatim hloupy feed co jde slepe k nejblizsimu jidlu
				C_Result res;
				Coord head = snek.Body[0];
				auto it = std::min_element(board.Treats.begin(), board.Treats.end(),
						[&](Treat t1, Treat t2) { return t1.Coord.ManhattanDist(head) < t2.Coord.ManhattanDist(head); }); //noice
				Coord favorite = it->Coord;
				if (head.x > favorite.x && snek.Heading() != Right) res.m_values[Left] = 1;
				if (head.x < favorite.x && snek.Heading() != Left)  res.m_values[Right] = 1;
				if (head.y > favorite.y && snek.Heading() != Down)  res.m_values[Up] = 1;
				if (head.y < favorite.y && snek.Heading() != Up)  res.m_values[Down] = 1;
				return res;
		}

		//==============================================================================

		C_Result StrategyFeedQuadrant(const Board& board, const Snek& snek)
		{
				C_Result res;

				Coord head = snek.Body[0];
				//every 50 ticks recompute quadrants
				if (tick % 50 == 0)
				{
						float clusters[4];
						memset(clusters, 0, sizeof(clusters));
						for (auto& treat : board.Treats)
						{
								int distance = head.ManhattanDist(treat.Coord);
								if (treat.Coord.x <= head.x && treat.Coord.y <= head.y) clusters[NW] ++;
								else if (treat.Coord.x >= head.x && treat.Coord.y <= head.y) clusters[NE] ++;
								else if (treat.Coord.x <= head.x && treat.Coord.y >= head.y) clusters[SW] ++;
								else if (treat.Coord.x >= head.x && treat.Coord.y >= head.y) clusters[SE] ++;
						}
						int bestCluster = NW;
						for (int i = 1; i <= SE; i++)
						{
								if (clusters[i] > clusters[bestCluster])
								{
										bestCluster = i;
								}
						}
						m_ActiveQuadrant = static_cast<E_Quadrants>(bestCluster);
						m_ActiveQuadrantCoord = head;
				}

				//find closest treat in active quadrant
				float minDistance = FLT_MAX;
				Coord bestCoords;
				for (auto& treat : board.Treats)
				{
						int distance = head.ManhattanDist(treat.Coord);
						if (treat.Coord.x <= m_ActiveQuadrantCoord.x && treat.Coord.y <= m_ActiveQuadrantCoord.y && m_ActiveQuadrant == NW && distance < minDistance) bestCoords = treat.Coord;//= 1 / distance;
						else if (treat.Coord.x >= m_ActiveQuadrantCoord.x && treat.Coord.y <= m_ActiveQuadrantCoord.y && m_ActiveQuadrant == NE && distance < minDistance) bestCoords = treat.Coord;
						else if (treat.Coord.x <= m_ActiveQuadrantCoord.x && treat.Coord.y >= m_ActiveQuadrantCoord.y && m_ActiveQuadrant == SW && distance < minDistance) bestCoords = treat.Coord;
						else if (treat.Coord.x >= m_ActiveQuadrantCoord.x && treat.Coord.y >= m_ActiveQuadrantCoord.y && m_ActiveQuadrant == SE && distance < minDistance) bestCoords = treat.Coord;
				}

				if (head.x > bestCoords.x) res.m_values[Left] = 1;
				if (head.x < bestCoords.x) res.m_values[Right] = 1;
				if (head.y > bestCoords.y) res.m_values[Up] = 1;
				if (head.y < bestCoords.y) res.m_values[Down] = 1;

				return res;
		}

		//===========================================================
		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				if (m_StrikeCount < 2)
				{
						m_LastSnekBoxUpdated++;
						if (m_LastSnekBoxUpdated >= board.Sneks.size())
						{
								m_LastSnekBoxUpdated = 0;
						}

						while (m_SnekBoxes.size() < board.Sneks.size())
						{
								m_SnekBoxes.emplace_back();
						}

						m_SnekBoxes[m_LastSnekBoxUpdated].Update(*board.Sneks[m_LastSnekBoxUpdated]);
				}


				//				DummyStep(board, snek, moveRequest, boost);

				C_Result surviveResult = StrategySurvive(board, snek);
				C_Result finalResult;
				finalResult.Add(surviveResult, 1.0f);

				C_Result feedResults = StrategyFeed(board, snek);
				finalResult.Add(feedResults, 1.0f);

				C_Result feedResultsQuadrant = StrategyFeedQuadrant(board, snek);
				feedResultsQuadrant.Add(feedResults, 0.5f);

				Dir bestDir = Left;
				float bestValue = finalResult.m_values[Left];

				for (int dirIdx = Left + 1; dirIdx <= Down; ++dirIdx)
				{
						if (finalResult.m_values[dirIdx] <= bestValue)
						{
								continue;
						}

						bestValue = finalResult.m_values[dirIdx];
						bestDir = static_cast<Dir>(dirIdx);
				}

				moveRequest = bestDir;
				boost = false;

				Coord pos = snek.Body[0];
				/*
							printf("x: %d, y: %d, dir: %d   L: %.3f, R: %.3f, U: %.3f D:%.3f\n",
								pos.x, pos.y, moveRequest, finalResult.m_values[Left], finalResult.m_values[Right], finalResult.m_values[Up], finalResult.m_values[Down]);
				*/
				tick++;
		}

		int m_StrikeCount = 0;

		virtual void ReportStepDuration(sf::Int64 durationMicroseconds) override
		{
				//			printf("duration: %I64d us\n", durationMicroseconds);

				if (durationMicroseconds >= MicrosecondsLimit)
				{
						m_StrikeCount++;
				}
		}
};


