#pragma once
#include <Stratega/ntbea/Evaluators/Evaluator.h>
#include <Stratega/ntbea/VectorSearchSpace.h>
#include <Stratega/Agent/UnitMCTSAgent/UnitMCTSAgent.h>

#include <Stratega/ntbea/Evaluators/Arena.h>

#include <Stratega/Configuration/GameConfig.h>

namespace SGA
{
	/// <summary>
    /// Base class to be inherited by other evaluator objects
    /// </summary>
    class MCTSuEvaluator : public Evaluator
    {
    private:
        std::vector<float> k_values;
        std::vector<int> rollout_length;
        std::vector<int> opponent_script;
        
        std::vector<int> currentPoint;

        SGA::GameConfig& config;
        std::mt19937 rngEngine = std::mt19937(0);
        const std::vector<std::unique_ptr<Agent>> agents;

        std::unique_ptr<Arena> arena = nullptr;

    public:
        MCTSuEvaluator(
            std::vector<float> k_values,
            std::vector<int> rollout_length,
//            std::vector<int> opponent_script,
            SGA::GameConfig& config
        );
    
        std::vector<float> evaluate(std::vector<int> point, int nSamples) override;
        //float evaluateGame(std::vector<int> point, int opponentID, bool playFirst);
    	
        void printPoint(const std::vector<int>& point) override;

        std::vector<std::unique_ptr<Agent>> generateAgents();
    };

    
}
