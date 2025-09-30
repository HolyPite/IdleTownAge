#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "resource.h"
#include "building.h"

class ResourceManager {
public:
    std::unordered_map<std::string, Resource> resources;

    Resource& get(const std::string& id) { return resources[id]; }

    bool canAfford(const std::vector<Cost>& costs) const {
        for(auto& c : costs) {
            auto it = resources.find(c.res);
            if(it == resources.end() || it->second.qty < c.qty) return false;
        }
        return true;
    }

    void pay(const std::vector<Cost>& costs) {
        for(auto& c : costs) {
            resources[c.res].qty -= c.qty;
        }
    }

    void add(const std::string& res, double qty) {
        auto& r = resources[res];
        r.qty += qty;
        if(r.qmax>0 && r.qty > r.qmax) r.qty = r.qmax;
    }
};
