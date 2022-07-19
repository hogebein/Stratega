#pragma once
#include <Stratega/Agent/Heuristic/StateHeuristic.h>

namespace SGA
{
	class BasicTBSTechnologyHeuristic : public StateHeuristic
	{
		// encourage greater:
		//   ableGenerateAttackable
		//   selfAttackPower
		//   Resource
		// avoid greater:
		//   enemyAttackPower
		//   

		//enermycityhp: 50%
		//can attack: 30%
		//enough gold for warrior:
		//
	private:
		bool initialized = false, ableToAttack=false, isBuiltBarracks=false,
			isResearchedMining=false, 
			isResearchedDicipline=false, isResearchedPotery=false, isResearchedMathematics=false, 
			isResearchedBronzeWorking=false, isResearchedArchery=false, isResearchedApprenticeship=false, 
			isResearchedMetallurgy=false, isResearchedEngineering=false;
		int self_city_id=-1, enemy_city_id=-1;
		Vector2f self_city, enemy_city;
		double city_max_health=-1.0;

		std::vector<Vector2f> goldVeinPosition;
		int currentTargetGoldVeinIdx=-1;
		double maximumBoardDistance = 1000000.0;
		double totalGold = 0.0;

		const double R_WORKER_P = 0.1, R_DISTANCE_P=0.2, R_GOLDPROPORTION_P=0.7;
		const double R_MINE_P = 0.2, R_TECH_P = 0.8;

	public:
		BasicTBSTechnologyHeuristic(int playerID, GameState initialState);
		//void initialize(int playerID, GameState state);
		void updateStateInfo(GameState& gameState, const int playerID);
		double evaluateGameState( const ForwardModel& forwardModel, GameState& gameState, const int playerID);
		double evaluateGameState( const ForwardModel& forwardModel, GameState& gameState, const int playerID, bool isDebug);

		double getProduction(GameState state);
		double getGold(GameState state);

		double getEntityHealth(GameState state, Vector2f position);

		std::string getName() const override { return "AimToKingHeuristic"; }
	};
}