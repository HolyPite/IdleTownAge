#include "building.h"
#include <utility>

Building::Building(std::string i, std::string n,
                   std::vector<Cost> cost,
                   std::vector<Cost> out,
                   std::vector<Cost> in)
    : id(std::move(i)),
      name(std::move(n)),
      base_cost(std::move(cost)),
      outputs(std::move(out)),
      inputs(std::move(in)) {}

std::vector<Cost> Building::nextCost() const {
    std::vector<Cost> c = base_cost;
    for (auto& x : c) x.qty *= std::pow(growth, count);
    return c;
}

bool Building::canAfford(const std::unordered_map<std::string, Resource>& R) const {
    auto costs = nextCost();
    for (const auto& c : costs) {
        auto it = R.find(c.res);
        if (it == R.end() || it->second.qty < c.qty) return false;
    }
    return true;
}

void Building::pay(std::unordered_map<std::string, Resource>& R) {
    auto costs = nextCost();
    for (const auto& c : costs) {
        auto it = R.find(c.res);
        if (it != R.end()) {
            it->second.qty -= c.qty;
        }
    }
}

void Building::build(std::unordered_map<std::string, Resource>& R) {
    if (canAfford(R)) {
        pay(R);
        ++count;
    }
}

void Building::produce(std::unordered_map<std::string, Resource>& R, double dt) {
    if (count <= 0) return;
    const double multiplier = dt * static_cast<double>(count);

    for (const auto& c : inputs) {
        auto it = R.find(c.res);
        if (it == R.end() || it->second.qty < c.qty * multiplier) return;
    }

    for (const auto& c : inputs) {
        auto it = R.find(c.res);
        if (it != R.end()) {
            it->second.qty -= c.qty * multiplier;
        }
    }

    for (const auto& c : outputs) {
        auto& res = R[c.res];
        if (res.id.empty()) res.id = c.res;
        res.qty += c.qty * multiplier;
    }
}
