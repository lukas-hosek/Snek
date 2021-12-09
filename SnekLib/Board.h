#pragma once

#include <vector>
#include <memory>
#include <set>
#include "Snek.h"
#include "Misc.h"

struct Board
{
		static const unsigned Rows = 60;
		static const unsigned Cols = 106;
		static const unsigned TreatCount = 100;

		std::vector<std::unique_ptr<Snek>> Sneks;
		std::set<Treat> Treats;
		
		bool IsFree(Coord c) const;
		bool IsWithinBounds(Coord c) const;
		Coord GetNext(Coord c) const;

private:
		friend class SnekGame;
		friend struct Snek;
		void Init();
		void Update();
};
