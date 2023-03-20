#include <Stratega/Agent/SLIAgent/SLIAgent.h>
#include <Stratega/Agent/Heuristic/AimToKingHeuristic.h>
#include <Stratega/Agent/Heuristic/BasicTBSHeuristic.h>
#include <Stratega/Agent/Heuristic/PushThemAllHeuristic.h>
#include <cmath>
#include <random>

namespace SGA {
	void SLIAgent::init(GameState initialState, const ForwardModel& forwardModel, Timer timer) {
		parameters_.PLAYER_ID = getPlayerID();
		actionSequence = std::vector<SGA::Action>();

		if (parameters_.HEURISTIC == "ktk") {
			parameters_.heuristic = std::make_unique< AimToKingHeuristic >(initialState);
		}
		else if (parameters_.HEURISTIC == "pta") {
			parameters_.heuristic = std::make_unique< PushThemAllHeuristic >(getPlayerID(), initialState);
		}
	}

	std::vector< std::vector<SGA::Action> > SLIAgent::generateActionSpace(GameState state, const ForwardModel& forwardModel) {
		std::vector< std::vector<SGA::Action> > nodeActionSpace;

		/* generate action space for each unit */
		auto allUnits = state.getPlayerEntities(parameters_.PLAYER_ID);
		int numUnit = allUnits.size();
		int uid = 0;
		for (auto u : allUnits) {
			auto entityActionSpace =
				forwardModel.generateUnitActions(state, u, parameters_.PLAYER_ID, true);
			if(entityActionSpace.size()==1){
				continue;
			}
			//entityActionSpaceSize[uid] = entityActionSpace.size();
			nodeActionSpace.push_back(std::vector<SGA::Action>{});
			for (int i = 0; i < entityActionSpace.size(); i++) {
				nodeActionSpace[uid].push_back(entityActionSpace[i]);
			}
			uid++;
		}

		return nodeActionSpace;
	}

	bool SLIAgent::rolloutFinished(GameState& rollerState, int depth, SLIParameters& params)
	{
		if (depth >= params.rolloutLength) {
			return true;
		}

		return rollerState.isGameOver();
	}

	void SLIAgent::applyActionToGameState(
		ForwardModel& forwardModel,
		GameState& targetGameState,
		Action& action,
		SLIParameters& params,
		int /*playerID*/) const
	{
		//Roll the game state with our action.
		//params.currentFMCalls += SGA::roll(targetGameState, forwardModel, action, playerID, params);
		int rollReturn = SGA::roll(targetGameState, forwardModel, action, params.PLAYER_ID, params);

		params.currentFMCalls += rollReturn;

		//Continue rolling the state until the game is over, we run out of budget or this agent can play again. 
		while (!targetGameState.canPlay(params.PLAYER_ID) && !params.isBudgetOver() && !targetGameState.isGameOver())
		{
			//Roll actions for the opponent(s).
			params.currentFMCalls += SGA::rollOppOnly(targetGameState, forwardModel, params);
		}
	}

	double SLIAgent::rollOut(GameState gameState, ForwardModel& forwardModel, SLIParameters& params, boost::mt19937& randomGenerator)
	{
		std::cout << "entered rollout\n";
		int thisDepth = 0;
		//Create a copy and mark our depth on the tree.
		auto gsCopy(gameState);

		// [BUGfixed] for FMCALLS budget: selected state is a terminal state that cost no budget
		if (gsCopy.isGameOver() && params.budgetType == Budget::FMCALLS) {
			//std::cout<<"catch terminal state!\n";
			params.currentFMCalls++;
		}

		std::cout << "starting while\n";
		//If we must keep rolling.
		while (!(rolloutFinished(gsCopy, thisDepth, params) || gsCopy.isGameOver())) {

			//Find my action space.
			auto actions = forwardModel.generateActions(gsCopy, params.PLAYER_ID);
			std::cout << "actions.size(): " << actions.size() << "\n";
			if (actions.size() == 0)
				break;

			//Pick one action at random and apply it to the current game state. 
			boost::random::uniform_int_distribution<size_t> randomDistribution(0, actions.size() - 1);
			applyActionToGameState(forwardModel, gsCopy, actions.at(randomDistribution(randomGenerator)), params, params.PLAYER_ID);
			thisDepth++;
		}
		std::cout << "end rollout inner loop\n";
		//We evaluate the state at the end of the rollout using the heuristic specified in the parameter settings. 
		//We then return this reward to the last node expanded in the tree.
		//return normalize(params.heuristic->evaluateGameState(forwardModel, gsCopy, params.PLAYER_ID), 0, 1);
		return params.heuristic->evaluateGameState(forwardModel, gsCopy, params.PLAYER_ID);

	}

	ActionAssignment SLIAgent::computeAction(GameState state, const ForwardModel& forwardModel, Timer timer) {
		parameters_.currentFMCalls = 0;
		// BUG: numUnit should be movable unit rather than real number of units
		std::cout << "START NEW  SEARCH!\n";

		///*
		std::cout << "\nActionSpace: \n";

		auto actionSpace = forwardModel.generateActions(state, getPlayerID());
		for(auto action: actionSpace){
			state.printActionInfo(action);
		}
		//return ActionAssignment::fromSingleAction(actionSpace[0]);
		//*/
		/*To modify position of these variables below
		*/
		int nSample = 100;
		const auto processedForwardModel = parameters_.preprocessForwardModel(forwardModel);
		int N_SAMPLE_FOR_EVALUATE = 20;
		int EVALUATE_CONSTANT = 100;

		boost::mt19937& randomGenerator = getRNGEngine();

		if (actionSequence.size() > 0) {
			//std::cout << "entering loop\n";
			auto action = actionSequence[0];
			actionSequence.erase(actionSequence.begin());
			while (!action.validate(state) && actionSequence.size() > 0) {
				action = actionSequence[0];
				actionSequence.erase(actionSequence.begin());
			}

			if (action.validate(state)) {
				/*
				std::cout << "Action Took: \n";
				state.printActionInfo(action);
				std::cout << "\n";
				//*/
				return ActionAssignment::fromSingleAction(action);
			}
		}


		std::vector< std::vector<SGA::Action> > nodeActionSpace = generateActionSpace(state, forwardModel);  
		if(nodeActionSpace.size() == 0){
			Action endAction = Action::createEndAction(getPlayerID());
			return ActionAssignment::fromSingleAction(endAction);
		}
		/*
		for (int i = 0; i < nodeActionSpace.size(); i++) {
			std::cout << "i: " << i << " , actionSpace size: " << nodeActionSpace[i].size() << "\n";
		}
		*/
		int NUM_UNITS = nodeActionSpace.size();

		/*whetherSelected: flags for whether this unit action is selected in the generate stage
		* it later is used to generate a action space for side information
		*/
		std::vector< std::vector<bool> > whetherSelected;
		std::vector< std::vector<double> > unitActionRewards = {};
		std::vector< std::vector<int> > unitActionCounts = {};
		for(int i = 0; i < nodeActionSpace.size(); i++){
			std::vector<bool> tmp = std::vector<bool> (nodeActionSpace[i].size(), false);
			whetherSelected.push_back(tmp);
			std::vector<double> tmp_r = std::vector<double>(nodeActionSpace[i].size(), 0.0);
			unitActionRewards.push_back(tmp_r);
			std::vector<int> tmp_count = std::vector<int>(nodeActionSpace[i].size(), 0);
			unitActionCounts.push_back(tmp_count);
		}

		std::cout << "a\n";
		
		int numUnit = nodeActionSpace.size();
		int which = 0;
		std::vector< std::vector<int> > actionCombinations = {};
		for (int j = 0; j < nSample; j++) {
			
			std::vector<int> actionCombination = {};
			for (int i = 0; i < numUnit; i++) {
				boost::random::uniform_int_distribution< size_t > distrib(0, nodeActionSpace[i].size() - 1);
				which = static_cast<int>(distrib(randomGenerator));
				actionCombination.push_back(which);
				//std::cout << "i: " << i << " , i size: " << whetherSelected[i].size() << " which: " << which << "\n";
				//i: 0 , i size: 0 which: 0
				whetherSelected[i][which] = true;
			}
			actionCombinations.push_back(actionCombination);
		}

		//std::cout << "a1\n";
		for (int j = 0; j < nSample; j++) {
			for (int i = 0; i < numUnit; i++) {
				//std::cout << "i: " << i << "\n";
				/*for each unit action sampled, randomly generate its complement, evaluate this combination and evaluate it*/
				auto uAction = nodeActionSpace[i][actionCombinations[j][i]];
				//std::cout << 1 << "\n";
				auto gsCopy(state);
				if (uAction.validate(gsCopy)) {
					applyActionToGameState(*processedForwardModel, gsCopy, uAction, parameters_, parameters_.PLAYER_ID);
				}
				// generate action for other units... and evaluate them with a rollout
				std::vector<int> combination_to_roll = {};
				//std::cout << 2 << "\n";
				for (int k = 0; k < numUnit; k++) {
					if (k == i)
						continue;
					boost::random::uniform_int_distribution< size_t > distrib(0, nodeActionSpace[k].size() - 1);
					which = static_cast<int>(distrib(randomGenerator));
					combination_to_roll.push_back(which);

					auto a_tmp = nodeActionSpace[k][which];
					
					if (a_tmp.validate(gsCopy)) {
						applyActionToGameState(*processedForwardModel, gsCopy, a_tmp, parameters_, parameters_.PLAYER_ID);
					}

					whetherSelected[i][which] = true;
				}
				//std::cout << 3 << "\n";
				// execute them all
				double r = rollOut(gsCopy, *processedForwardModel, parameters_, randomGenerator);
				//std::cout << 4 << "\n";
				//std::cout << "unitActionRewards[i].size: " << unitActionRewards[i].size() << " , actionCombinations[j][i]: " << actionCombinations[j][i] << "\n";
				unitActionRewards[i][actionCombinations[j][i]] += r;
				unitActionCounts[i][actionCombinations[j][i]] += 1;
			}
		}
		//std::cout << "a2\n";
		// generate a new action space since there might be repeated unit action in the samples
		std::vector<std::vector<Action> > filteredActionSpace = {};
		std::vector<std::vector<double> > filteredActionValue = {};
		std::vector<double> expSum = {};
		for (int i = 0; i < nodeActionSpace.size(); i++) {
			filteredActionSpace.push_back(std::vector<Action>());
			filteredActionValue.push_back(std::vector<double>());

			double unitExpSum = 0.0;
			for(int j = 0; j < nodeActionSpace[i].size(); j++){
				if(whetherSelected[i][j]){
					filteredActionSpace[i].push_back(nodeActionSpace[i][j]);
					// also transfer the reward/count
					double unitActionValue = exp(unitActionRewards[i][j] / unitActionCounts[i][j]);
					filteredActionValue[i].push_back(unitActionValue);
					unitExpSum += unitActionValue;
				}
			}
			expSum.push_back(unitExpSum);
		}
		// return ActionAssignment::fromSingleAction(actionSpace[0]); // this works

		//sideInfo() -> above
		//uniformly sample N action combinations -> A
		// m  = int(budget / N)
		// for atomic action a in A
		// uniformly sample other unit actions to form a combination that contains a, get score and update
		// return R (reward_for_each_unit_action)
		//std::cout << "b\n";
		/*softmax*/
		/*
		for (int i = 0; i < nodeActionSpace.size(); i++) {
			for (int j = 0; j < nodeActionSpace[i].size(); j++){
				filteredActionValue[i][j] /= expSum[i];
			}
		}
		*/
		for (int i = 0; i < filteredActionSpace.size(); i++) {
			for (int j = 0; j < filteredActionSpace[i].size(); j++) {
				filteredActionValue[i][j] /= expSum[i];
			}
		}
		/*entropy*/
		/*
		std::vector<double> entropy = {};
		for (int i = 0; i < nodeActionSpace.size(); i++) {
			double unitEntropy = 0.0;
			for (int j = 0; j < nodeActionSpace[i].size(); j++) {
				double p = filteredActionValue[i][j];
				unitEntropy += p * log(p);
			}
			entropy.push_back(unitEntropy);
		}
		*/
		std::vector<double> entropy = {};
		for (int i = 0; i < filteredActionSpace.size(); i++) {
			double unitEntropy = 0.0;
			for (int j = 0; j < filteredActionSpace[i].size(); j++) {
				double p = filteredActionValue[i][j];
				unitEntropy += p * log(p);
			}
			entropy.push_back(unitEntropy);
		}
		//generate-entropy
		//reward_for_each_unit_action = sideInfo()
		//std::cout << 2 << "\n";
		//return ActionAssignment::fromSingleAction(actionSpace[0]); //this does not work, works after the above change


		std::vector< std::vector<int> > actionCombinationsToEvaluate = {};

		std::vector<double> sortedEntropy(entropy);
		std::sort(sortedEntropy.begin(), sortedEntropy.end(), std::greater<>());

		/*index is the order, value is unit id (not the id in the engine)*/
		std::vector<int> unitOrder = std::vector<int>(entropy.size(), 0);
		for(int i = 0; i < sortedEntropy.size(); i++){
			auto iter = std::lower_bound(sortedEntropy.begin(), sortedEntropy.end(), entropy[i]);
			int order = int(iter - sortedEntropy.begin());
			unitOrder[order] = i;
		}
		std::cout << 3 << " actionCombinationsToEvaluate.size(): "<< actionCombinationsToEvaluate.size()<<"\n";

		std::vector< std::discrete_distribution<std::size_t> > d_vector = {};
		for (int i = 0; i < NUM_UNITS; i++) {
			int unit_id = unitOrder[i];
			d_vector.push_back(std::discrete_distribution<std::size_t>(filteredActionValue[unit_id].begin(), filteredActionValue[unit_id].end()));
		}
		for(int kk = 0; kk < N_SAMPLE_FOR_EVALUATE; kk++){
			actionCombinationsToEvaluate.push_back(std::vector<int>(NUM_UNITS, 0));
			std::cout << "kk: " << kk << "\n";


			for (int i = 0; i < NUM_UNITS; i++) {
				/*sample unit action for unitOrder[i]*/
				int unit_id = unitOrder[i];
				//std::discrete_distribution<std::size_t> d{ filteredActionValue[i].begin(), filteredActionValue[i].end() };
				std::cout << "filteredActionValue.size(): " << filteredActionValue.size() << " unit_id: " << unit_id << "filteredActionValue[unit_id].size() " << filteredActionValue[unit_id].size() <<"\n";
				
				for(int j = 0; j < filteredActionValue[unit_id].size(); j++){
					std::cout << filteredActionValue[unit_id][j] << "\t";
				}
				std::cout << "\n";
				//std::discrete_distribution<std::size_t> d{ filteredActionValue[unit_id].begin(), filteredActionValue[unit_id].end() };
				//std::vector<double> tmp_d = std::vector<double>(filteredActionValue[unit_id].size(), 1.0);
				//std::discrete_distribution<std::size_t> d( filteredActionValue[unit_id].begin(), filteredActionValue[unit_id].end() );
				std::cout << "3.3\n";
				//int unit_action_id = d(randomGenerator);
				int unit_action_id = d_vector[i](randomGenerator);
				std::cout << "kk: " << kk << " unit_id: " << unit_id <<" actionCombinationsToEvaluate[kk].size(): "<< actionCombinationsToEvaluate[kk].size() << "\n";
				actionCombinationsToEvaluate[kk][unit_id] = unit_action_id;
			}
		}

		//return ActionAssignment::fromSingleAction(actionSpace[0]); // works

		/*for debugging*/
		/*
		for(int i = 0; i < actionCombinationsToEvaluate.size(); i++){
			std::cout << "actionCombinationsToEvaluate[i].size(): " << actionCombinationsToEvaluate[i].size() << "\n";
		}
		*/

		std::cout << 4 << "\n";
		// calculate entropy of distribution for each unit
		// sample unit action according to unit-order decided by its entropy of reward distribution
		// return a set of action combinations
		int n_evaluate = int(log(N_SAMPLE_FOR_EVALUATE));
		/*
		std::cout << "n_evaluate: " << n_evaluate << "\n";
		for(int i = 0; i < filteredActionSpace.size(); i++){
			std::cout << "filteredActionSpace " << i << " size: " << filteredActionSpace[i].size() <<"\n";
		}
		for (int i = 0; i < filteredActionValue.size(); i++) {
			std::cout << "filteredActionValue " << i << " size: " << filteredActionValue[i].size() << "\n";
		}
		//*/

		int sample_remained = N_SAMPLE_FOR_EVALUATE;
		std::vector<double> reward_evaluated = std::vector<double>(sample_remained, 0.0);
		for(int i = 0; i < n_evaluate; i++){
			std::cout << "The Evaluate Loop: " << i << "\n";
			int m = int(EVALUATE_CONSTANT / (sample_remained * int(log(EVALUATE_CONSTANT))));

			/*for each action combination, evaluate them, get half of them for the next turn*/
			for(int j = 0; j < sample_remained; j++){
				double r_this = 0.0;
				auto gsCopy(state);
				std::cout << 4.01 << "\n";

				/*execute this action combination*/
				for (int k = 0; k < numUnit; k++) {
					//auto a_tmp = nodeActionSpace[k][actionCombinationsToEvaluate[j][k]];
					std::cout << "actionCombinationsToEvaluate[j].size(): " << actionCombinationsToEvaluate[j].size() << "\n";
					std::cout << "k: " << k << ", actionCombinationsToEvaluate[j][k]: " << actionCombinationsToEvaluate[j][k];
					std::cout << ", filteredActionSpace[k].size(): " << filteredActionSpace[k].size() << " numUnit: "<< numUnit<< "\n";
					auto a_tmp = filteredActionSpace[k][actionCombinationsToEvaluate[j][k]];

					if (a_tmp.validate(gsCopy)) {
						applyActionToGameState(*processedForwardModel, gsCopy, a_tmp, parameters_, parameters_.PLAYER_ID);
					}

				}
				std::cout << 4.02 << "\n";
				for(int k = 0; k < m; k++){
					// execute them all
					std::cout << "rollout k: " << k << " m: " << m <<" isEnd? "<< gsCopy.isGameOver() << "\n";
					double r = rollOut(gsCopy, *processedForwardModel, parameters_, randomGenerator);
					r_this += r;
				}
				
				r_this /= m;
				reward_evaluated[j] = r_this;
				std::cout << 4.03 << "\n";

			}
			std::cout << 4.1 << "\n";

			// update sample_remained
			std::vector<double> sortedReward(reward_evaluated);

			std::sort(sortedReward.begin(), sortedReward.end(), std::greater<>());

			/*index is the order, value is unit id (not the id in the engine)*/
			std::vector<int> banditOrder = std::vector<int>(reward_evaluated.size(), 0);
			
			for (int j = 0; j < sortedReward.size(); j++) {
				//auto iter = std::lower_bound(sortedReward.begin(), sortedReward.end(), reward_evaluated[j]);
				auto iter = std::find(sortedReward.begin(), sortedReward.end(), reward_evaluated[j]);
				int order = int(iter - sortedReward.begin());
				banditOrder[order] = j;
			}
			std::cout << 4.3 << "\n";
			/*if (i == 1) { // doesn't work, works now
				for(int j = 0; j < banditOrder.size(); j++){
					std::cout << "bandit " << j << " : " << banditOrder[j] << "\n";
				}
				std::cout << "4.3.1\n";
				//return ActionAssignment::fromSingleAction(actionSpace[0]);
			}*/
			

			std::vector<std::vector<int> > tmp = {};
			for(int j = 0; j < int(sample_remained/2); j++){
				if(banditOrder[j] >= actionCombinationsToEvaluate.size()){
					std::cout << "ERROR!!#*(#@*#(\n";
				}
				tmp.push_back(std::vector<int>());
				for(int k = 0; k < actionCombinationsToEvaluate[banditOrder[j]].size(); k++){
					tmp[j].push_back(actionCombinationsToEvaluate[banditOrder[j]][k]);
				}
				//tmp.push_back(actionCombinationsToEvaluate[banditOrder[j]]);
			}
			std::cout << 4.4 << "\n";
			//actionCombinationsToEvaluate.erase(actionCombinationsToEvaluate.begin(), actionCombinationsToEvaluate.end());
			/*for(int j = 0; j < actionCombinationsToEvaluate.size(); j++){
				std::cout << "j: " << j << ", actionCombinationsToEvaluate.size(): "<< actionCombinationsToEvaluate.size() <<"\n";
				actionCombinationsToEvaluate[j].clear();
			}*/
			actionCombinationsToEvaluate = tmp; // std::vector<std::vector<int> >();
			//actionCombinationsToEvaluate.clear();
			std::cout << 4.5 << "\n";
			/*for (int j = 0; j < tmp.size(); j++) {
				actionCombinationsToEvaluate.push_back(std::vector<int>());
				for(int k = 0; k < tmp[j].size(); k++){
					actionCombinationsToEvaluate[j].push_back(tmp[j][k]);
				}
				//actionCombinationsToEvaluate.push_back(tmp[j]);
			}*/
			std::cout << "remaining action combinations:\n";
			for (int j = 0; j < actionCombinationsToEvaluate.size(); j++) {
				std::cout << "actionCombinationsToEvaluate[j].size(): " << actionCombinationsToEvaluate[j].size() << "\n";
			}

			//actionCombinationsToEvaluate = tmp;
			sample_remained = actionCombinationsToEvaluate.size();

			std::cout << "sample_remained: " << sample_remained << "\n";
			if(sample_remained == 1){
				break;
			}
			reward_evaluated = std::vector<double>(sample_remained, 0.0);
		}
		/*for debbuging, check the output of the evaluation*/
		//return ActionAssignment::fromSingleAction(actionSpace[0]); works

		std::cout << 5 << " actionCombinationsToEvaluate[0].size(): " << actionCombinationsToEvaluate.size() << "\n";
		std::vector<int> finalAction = actionCombinationsToEvaluate[0];
		for(int i = 0; i < finalAction.size(); i++){
			actionSequence.push_back(filteredActionSpace[i][finalAction[i]]);
		}
		std::cout << 6 << " actionSequence.size(): "<< actionSequence.size() << "\n";
		auto a_to_return = actionSequence[0];
		//state.printActionInfo(actionSequence[0]);
		actionSequence.erase(actionSequence.begin());
		
		std::cout << "c\n";
		//return ActionAssignment::fromSingleAction(actionSpace[0]);
		//std::cout << "remaining forward model calls" << parameters_.currentFMCalls<< "\n";
		if(!a_to_return.validate(state)){
			std::cout << "action not valid\n";
		}
		std::cout << "d\n";
		return ActionAssignment::fromSingleAction(a_to_return);
		//evaluate
		// while True
		// for each action combination
		// evaluate N times and get scores

		// eliminate K action combinations

		// return the only action combination
	}
}
