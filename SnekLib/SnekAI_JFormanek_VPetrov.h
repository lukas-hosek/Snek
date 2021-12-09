#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"
#include <optional>
#include <functional>
#include <queue>

struct SnekAI_JFormanek_VPetrov : public SnekAI
{
		virtual Team GetTeam() override { return Team::JFormanekVPetrov; };

		struct Vertex
		{
				using T_Distance = int;
				static const T_Distance s_MaxDistance = std::numeric_limits<T_Distance>::max();

				bool Occupied = false;
				bool Food = false;
				bool VirtualOccupied = false;
				T_Distance Distance = s_MaxDistance;
				const Vertex* Prev = nullptr;
				bool Visited = false;
				Coord	Coords;

				void ClearBeforeSync()
				{
						Occupied = Food = false;
				}

				void PrepareSearch()
				{
						Distance = s_MaxDistance;
						Prev = nullptr;
						Visited = false;
				}
		};

		using T_VertexCallback = std::function<void(Vertex& v)>;
		using T_VertexCondition = std::function<bool(Vertex& v)>;

		struct Path
		{
				std::vector<Coord>	VerticesCoords;
		};

		struct Graph
		{
				Vertex	m_Vertices[Board::Cols][Board::Rows];
				std::vector<Vertex*> unvisited;
				const int maxSize = Board::Cols * Board::Rows;

				Graph()
				{
						for (int x = 0; x < Board::Cols; ++x)
						{
								for (int y = 0; y < Board::Rows; ++y)
								{
										Coord c;
										c.x = x;
										c.y = y;
										m_Vertices[x][y].Coords = c;
								}
						}

						unvisited.reserve(maxSize);
				}

				Vertex* GetVertex(const Coord& c)
				{
						if (c.x >= Board::Cols)
								return nullptr;
						if (c.y >= Board::Rows)
								return nullptr;
						return &m_Vertices[c.x][c.y];
				}

				Vertex* GetVertex(const unsigned x, const unsigned y)
				{
						return GetVertex(Coord(x, y));
				}

				void Iterate(T_VertexCallback callback)
				{
						for (int x = 0; x < Board::Cols; ++x)
						{
								for (int y = 0; y < Board::Rows; ++y)
								{
										callback(m_Vertices[x][y]);
								}
						}
				}

				bool IsFree(const Board& board, Coord& c)
				{
						for (auto& s : board.Sneks)
						{
								for (auto& b : s->Body)
								{
										if (b.x == c.x && b.y == c.y)
										{
												return false;
										}
								}
						}

						return true;
				}

				void SyncWithBoard(const Board& board)
				{
						Iterate([this, &board](Vertex& v)
								{
										v.ClearBeforeSync();
										v.Occupied = !board.IsFree(v.Coords);
								});

						for (const auto& f : board.Treats)
						{
								m_Vertices[f.Coord.x][f.Coord.y].Food = true;
								m_Vertices[f.Coord.x][f.Coord.y].Occupied = false;
						}
				}

				bool WriteVirtualOccupation(Path& path)
				{
						for (auto& c : path.VerticesCoords)
						{
								m_Vertices[c.x][c.y].VirtualOccupied = true;
						}
				}

				bool ClearVirtualOccupation()
				{
						Iterate([](Vertex& v)
								{
										v.VirtualOccupied = false;
								});
				}

				bool ComputeNeighbour(const Vertex& current, Vertex* n)
				{
						if (n == nullptr || n->Visited || n->Occupied)
						{
								return false;
						}

						if (n->Distance > current.Distance + 1)
						{
								n->Distance = current.Distance + 1;
								n->Prev = &current;
								return true;
						}

						return false;
				}

				struct VertexComp
				{
						bool operator()(const Vertex* lhs, const Vertex* rhs)
						{
								return lhs->Distance < rhs->Distance;
						}
				};


				void FindDistancesFromHead(Vertex& head)
				{
						Iterate([&](Vertex& v)
								{
										v.PrepareSearch();
										if (head.Coords.ManhattanDist(v.Coords) < 15)
										{
												unvisited.push_back(&v);
										}
								});

						head.Distance = 0;

						while (!unvisited.empty())
						{
								auto it = std::min_element(unvisited.begin(), unvisited.end(), VertexComp());
								Vertex* current = *it;
								unvisited.erase(it);
								current->Visited = true;

								if (current->Distance != Vertex::s_MaxDistance)
								{
										bool computed = false;
										computed |= ComputeNeighbour(*current, GetVertex(current->Coords.x - 1, current->Coords.y));
										computed |= ComputeNeighbour(*current, GetVertex(current->Coords.x + 1, current->Coords.y));
										computed |= ComputeNeighbour(*current, GetVertex(current->Coords.x, current->Coords.y - 1));
										computed |= ComputeNeighbour(*current, GetVertex(current->Coords.x, current->Coords.y + 1));
								}
						}
				}

				std::optional<Path> FindShortest(const Coord& /*start*/, const Coord& end)
				{
						Path p;

						const Vertex* current = &m_Vertices[end.x][end.y];

						while (current)
						{
								p.VerticesCoords.push_back(current->Coords);
								current = current->Prev;
						}

						std::reverse(std::begin(p.VerticesCoords), std::end(p.VerticesCoords));

						return p;
				}

				Vertex* FindBestTarget(T_VertexCondition condition)
				{
						float bestDistance = Vertex::s_MaxDistance;
						Vertex* bestVertex = nullptr;

						Iterate([&](Vertex& v)
								{
										if (v.Distance < bestDistance && condition(v))
										{
												bestVertex = &v;
												bestDistance = v.Distance;
										}
								});

						return bestVertex;
				}

				Vertex* FindBestFood()
				{
						return FindBestTarget([](Vertex& v)
								{
										return v.Food;
								});
				}

				Vertex& GetCurrentHead(const Snek& snek)
				{
						auto c = snek.Body[0];
						return *GetVertex(c);
				}

		};

		Graph		m_Graph;
		Vertex* m_CurrentTarget = nullptr;

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				m_Graph.SyncWithBoard(board);
				m_Graph.FindDistancesFromHead(m_Graph.GetCurrentHead(snek));

				if (!m_CurrentTarget || m_CurrentTarget == &m_Graph.GetCurrentHead(snek) || !m_CurrentTarget->Food)
				{
						m_CurrentTarget = m_Graph.FindBestFood();
				}

				if (m_CurrentTarget)
				{
						auto currentCoords = m_Graph.GetCurrentHead(snek).Coords;
						auto path = m_Graph.FindShortest(currentCoords, m_CurrentTarget->Coords);
						if (path->VerticesCoords.size() > 1)
						{
								int i = 0;
								for (auto c : path->VerticesCoords)
								{
										if (i && (m_Graph.m_Vertices[c.x][c.y].Occupied || !board.IsWithinBounds(c) || !m_Graph.IsFree(board, c)))
										{
												__debugbreak();
										}
										++i;
								}

								auto nextMove = path->VerticesCoords[1];
								moveRequest = GetMovementDirection(currentCoords, nextMove);
								if (!board.IsWithinBounds(nextMove) || !m_Graph.IsFree(board, nextMove))
								{
										__debugbreak();
								}
						}
				}
				else
				{
						DummyStep(board, snek, moveRequest, boost);
				}
		}

		Dir GetMovementDirection(Coord start, Coord target)
		{
				if (start.y == target.y)
				{
						if (start.x < target.x)
						{
								return Dir::Right;
						}
						else
						{
								return Dir::Left;
						}
				}
				else
				{
						if (start.y < target.y)
						{
								return Dir::Down;
						}
						else
						{
								return Dir::Up;
						}
				}
		}
};
