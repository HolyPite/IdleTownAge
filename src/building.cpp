#include "building.h"

Building::Building(std::string i, std::string n,
                   std::vector<Cost> cost,
                   std::vector<Cost> out,
                   std::vector<Cost> in)
    : id(i), name(n), base_cost(cost), outputs(out), inputs(in) {}

std::vector<Cost> Building::nextCost() const {
    std::vector<Cost> c = base_cost;
    for(auto& x : c) x.qty *= std::pow(growth, count);
    return c;
}

bool Building::canAfford(const std::unordered_map<std::string,Resource>& R) const {
    for(auto& c : nextCost())
        if(R.at(c.res).qty < c.qty) return false;
    return true;
}

void Building::pay(std::unordered_map<std::string,Resource>& R) {
    for(auto& c : nextCost()) R[c.res].qty -= c.qty;
}

void Building::build(std::unordered_map<std::string,Resource>& R) {
    if(canAfford(R)) { pay(R); ++count; }
}

void Building::produce(std::unordered_map<std::string,Resource>& R,double dt) {
    for(auto& c : inputs) if(R.at(c.res).qty < c.qty*dt) return;
    for(auto& c : inputs) R[c.res].qty -= c.qty*dt;
    for(auto& c : outputs) R[c.res].qty += c.qty*dt*count;
}
