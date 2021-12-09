#pragma once

#include "SnekGame.h"
#include "SFML/Window.hpp"
#include "DummyAi.h"

#include <functional>
#include <queue>
#include <vector>
#include <iostream>

namespace TBarak_MJarolimek
{
		using TScore = float;
		static const TScore ScoreDefault = 0.f;
		static const TScore ScoreHeadingTreat = 3.f;
		static const TScore ScoreHeadingEnemy = 2.f;
		static const TScore ScoreHeadingUpDown = 1.f;

		using TMoveRequests = std::vector<Dir>;
		using TBody = std::vector<Coord>;

		inline Coord Move(const Coord& p, Dir dir, int offset = 1)
		{
				Coord ret = p;
				switch (dir)
				{
				case Left: ret.x -= offset; break;
				case Right:ret.x += offset; break;
				case Up:   ret.y -= offset; break;
				case Down: ret.y += offset; break;
				default:;
				}

				return ret;
		}

		inline bool HeadingToTarget(Coord head, Coord target, Dir dir)
		{
				Dir dx = ((int)target.x - (int)head.x) > 0 ? Right : Left;
				Dir dy = ((int)target.y - (int)head.y) > 0 ? Down : Up;
				return (dir == dx || dir == dy);
		}

		inline bool HeadingUpDown(Coord head, Coord target, Dir dir)
		{
				return (dir == Up || dir == Down);
		}

		template<class Fn>
		void ForEachSnekMoveRequest(Dir heading, Fn fn)
		{
				switch (heading)
				{
				case Left: fn(Left); fn(Down); fn(Up); break;
				case Right: fn(Right); fn(Up); fn(Down); break;
				case Up: fn(Up); fn(Left); fn(Right); break;
				case Down: fn(Down); fn(Right); fn(Left); break;
				default: fn(Up); // the fuck?
				}
		}

		inline bool IsCollision(const Board& board, Coord c)
		{
				for (auto& s : board.Sneks)
				{
						if (std::find(s->Body.begin(), s->Body.end(), c) != s->Body.end())
						{
								return true;
						}
				}

				return board.IsWithinBounds(c) == false;
		}

		inline bool IsTreat(const Board& board, Coord c)
		{
				return std::find_if(board.Treats.begin(), board.Treats.end(), [&](auto& t) {return t.Coord == c; }) != board.Treats.end();
		}

		inline Coord FindClosestTreat(const Board& board, Coord head)
		{
				auto it = std::min_element(board.Treats.begin(), board.Treats.end(),
						[&](Treat t1, Treat t2)
						{
								return t1.Coord.ManhattanDist(head) < t2.Coord.ManhattanDist(head);
						});

				return it->Coord;
		}

		inline Coord FindClosestEnemy(const Board& board, Coord head)
		{
				auto it = std::min_element(board.Sneks.begin(), board.Sneks.end(),
						[&](const std::unique_ptr<Snek>& s1, const std::unique_ptr<Snek>& s2)
						{
								return s1->Body.front().ManhattanDist(head) < s2->Body.front().ManhattanDist(head);
						});

				if (it == board.Sneks.end())
				{
						return Coord();
				}

				const auto& enemyHead = (*it)->Body.front();

				return Move(enemyHead, (*it)->Heading(), 2);
		}

		struct STargets
		{
				Coord Treat;
				Coord Enemy;
		};

		struct SState
		{
				bool operator<(const SState& r) const
				{
						return Score < r.Score;
				}

				Dir GetHeading(const Snek& snek) const
				{
						if (MoveRequests.size())
						{
								return MoveRequests.back();
						}

						return snek.Heading();
				}

				Coord GetHead(const Snek& snek) const
				{
						if (AdditionalBody.size())
						{
								return AdditionalBody.back();
						}

						return snek.Body.front();
				}

				bool IsCollision(const Board& board, Coord c)
				{
						if (TBarak_MJarolimek::IsCollision(board, c))
						{
								return true;
						}

						// todo there is no movement of this snake nor the others
						return std::find(AdditionalBody.begin(), AdditionalBody.end(), c) != AdditionalBody.end();
				}

				void GetFirstAction(Dir& dir)
				{
						if (MoveRequests.empty())
						{
								dir = None;
						}
						else
						{
								dir = MoveRequests[0];
						}
				}

				static SState NextState(const SState& currState, TScore addScore, Dir moveRequest, Coord newHead)
				{
						SState ret = currState;

						ret.Score += addScore;
						ret.MoveRequests.push_back(moveRequest);
						ret.AdditionalBody.push_back(newHead);

						return ret;
				}

				TScore Score = ScoreDefault;
				TMoveRequests MoveRequests; // last request last
				TBody AdditionalBody; // head last
		};

		using TStateQueue = std::priority_queue<SState>;

		inline bool ExpandQueue(TStateQueue& queue, SState& bestClosed, const Board& board, const Snek& snek, const STargets& targets)
		{
				if (queue.empty())
				{
						return false;
				}

				SState top = queue.top();
				queue.pop();

				ForEachSnekMoveRequest(top.GetHeading(snek), [&](Dir dir)
						{
								const Coord head = top.GetHead(snek);
								const Coord newHead = Move(head, dir);
								if (top.IsCollision(board, newHead))
								{
										if (bestClosed < top)
										{
												bestClosed = top;
										}
										return;
								}
								TScore scoreHeadingUpDown = HeadingUpDown(head, targets.Treat, dir) ? ScoreHeadingUpDown : 0;
								TScore scoreAddTreat = HeadingToTarget(head, targets.Treat, dir) ? ScoreHeadingTreat : 0;
								TScore scoreAddEnemy = HeadingToTarget(head, targets.Enemy, dir) ? ScoreHeadingEnemy : 0;
								queue.emplace(SState::NextState(top, scoreHeadingUpDown + scoreAddTreat + scoreAddEnemy, dir, newHead));
						});

				return true;
		}
}

struct SnekAI_TBarak_MJarolimek : public SnekAI
{
		unsigned IterationLimit = 200;

		virtual Team GetTeam() override { return Team::TBarakMJarolimek; };

		void FindBestStep(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost)
		{
				TBarak_MJarolimek::SState bestClosed;
				TBarak_MJarolimek::TStateQueue stateQueue;
				stateQueue.emplace(bestClosed);

				TBarak_MJarolimek::STargets targets;
				targets.Treat = TBarak_MJarolimek::FindClosestTreat(board, bestClosed.GetHead(snek));
				targets.Enemy = TBarak_MJarolimek::FindClosestEnemy(board, bestClosed.GetHead(snek));

				auto limit = IterationLimit;
				while (TBarak_MJarolimek::ExpandQueue(stateQueue, bestClosed, board, snek, targets) && limit--)
				{

				}

				if (!stateQueue.empty() && bestClosed < stateQueue.top())
				{
						bestClosed = stateQueue.top();
				}

				bestClosed.GetFirstAction(moveRequest);
		}

		void Step(const Board& board, const Snek& snek, Dir& moveRequest, bool& boost) override
		{
				FindBestStep(board, snek, moveRequest, boost);
				//DummyStep(board, snek, moveRequest, boost);
		}

		void ReportStepDuration(sf::Int64 durationMicroseconds) override
		{
				if (durationMicroseconds > MicrosecondsLimit)
				{
						const auto ratio = (float)durationMicroseconds / MicrosecondsLimit;
						IterationLimit /= (ratio * 1.1f);
				}
				//std::cout << durationMicroseconds << "ms, limit: " << IterationLimit << std::endl;
		}
};
