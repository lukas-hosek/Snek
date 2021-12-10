#include "SnekGame.h"

#include "View.h"
#include "SnekAI_Dummy.h"
#include "SnekAI_Human.h"

#include "AIProvider.h"
#include "BatchUpdater.h"


enum class Mode
{
		ViewAI,
		ViewHuman,
		BatchModeCustomTeams,
		MonteCarlo
};


int main()
{
		Mode mode = Mode::ViewAI;

		std::vector<Team> teams = {
				Team::GMolnarASporka,
				Team::JFormanekVPetrov,
				Team::MMarkoLHosek,
				Team::MLabutRKrenek,
				Team::PManMSourek,
				Team::PNohejlMMatous,
				Team::PSmrcekLSuk,
				Team::TBarakMJarolimek,
				Team::TGrunwaldDMikes,
				Team::TJanakMKlima,
				Team::TVahalikRSevcik
		};

		//teams = AllTeams;

		if (mode == Mode::ViewAI || mode == Mode::ViewHuman)
		{
				SnekGame TheGame;
				if (mode == Mode::ViewHuman)
				{
						teams.push_back(Team::Human);
				}
				TheGame.SetTeams(teams);
				View view;
				view.Init(TheGame);
				view.Update(TheGame);
		}
		else if (mode == Mode::BatchModeCustomTeams)
		{
				BatchUpdater batchUpdater;
				batchUpdater.Teams = teams;
				batchUpdater.Runs = 10;
				batchUpdater.PrintInfo = false;
				auto res = batchUpdater.Run(); 
				batchUpdater.PrintResults(res);
		}
		else if (mode == Mode::MonteCarlo)
		{
				BatchUpdater::FindBestOfMT(AllTeams, 3);
				//BatchUpdater::FindBestOf(AllTeams, 3);
		}
}