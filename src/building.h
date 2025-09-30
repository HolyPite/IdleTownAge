#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include "resource.h"   // ta classe Resource

struct Cost {
    std::string res;
    double qty;
};

class Building {
public:
    std::string id, name;
    int count = 0;

    std::vector<Cost> base_cost;
    double growth = 1.15;

    std::vector<Cost> inputs;
    std::vector<Cost> outputs;

    Building(std::string i, std::string n,
             std::vector<Cost> cost,
             std::vector<Cost> out,
             std::vector<Cost> in = {});

    std::vector<Cost> nextCost() const;
    bool canAfford(const std::unordered_map<std::string,Resource>& R) const;
    void pay(std::unordered_map<std::string,Resource>& R);
    void build(std::unordered_map<std::string,Resource>& R);
    void produce(std::unordered_map<std::string,Resource>& R,double dt);
};
