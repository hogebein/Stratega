#pragma once
#include <Stratega/Representation/TBSGameState.h>
#include <Stratega/Agent/StateAbstraction/AbstractState.h>
#include <string>
#include <iostream>
#include <map>

namespace SGA
{
	struct StateFactoryConfiguration {
		bool insertMap = false;
		bool insertEntityPositions = true;
		//std::map<std::string, bool> insertPlayerParameters;
		std::map<std::string, bool> insertEntityParameters;
		//std::map<std::string, std::map<int, int>> mapParameterNameToUnitTypeParameterID;

		StateFactoryConfiguration() {

		}
	};

	class StateFactory
	{
	public:
		StateFactoryConfiguration config;

	public:
		StateFactory(TBSGameState& tbs, StateFactoryConfiguration _config = StateFactoryConfiguration()) : config(_config)
		{
			for (auto entry : *tbs.parameterIDLookup) {
				if (!config.insertEntityParameters.contains(entry.first))
						config.insertEntityParameters[entry.first] = false;
			}
		}

		AbstractState createAbstractState(TBSGameState& tbsState);

		friend std::ostream& operator<<(std::ostream& os, const StateFactory& dt)
		{
			os << "Map: " << dt.config.insertMap << std::endl;
			os << "Positions: " << dt.config.insertEntityPositions << std::endl;
			for (auto entry : dt.config.insertEntityParameters)
			{
				os << entry.first << ": " << entry.second << std::endl;
			}
			
			return os;
		}
	};
}

