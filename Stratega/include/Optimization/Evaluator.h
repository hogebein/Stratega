#pragma once
#include <string>
#include <iostream>

#include <Optimization/Mutator.h>
#include <Optimization/SearchSpace.h>
#include <Optimization/Evaluator.h>
#include <Optimization/NTupleLandscapeModel.h>

namespace SGA
{
    /// <summary>
    /// Base class to be inherited by other evaluator objects
    /// </summary>
    class Evaluator
    {
    private:
        std::string _name;

    public:
        Evaluator(std::string name) : _name(std::move(name)) {};

        [[nodiscard]] std::string getName() const { return _name; };
        virtual std::vector<float> evaluate(std::vector<int> point, int nSamples);
        virtual void printPoint(const std::vector<int>& point);
    };
}

    
