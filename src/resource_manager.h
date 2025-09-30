#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "resource.h"
#include "building.h"

class ResourceManager {
public:
    std::unordered_map<std::string, Resource> resources;
    std::vector<std::string> order;

    Resource& ensureResource(const std::string& id) {
        auto [it, inserted] = resources.try_emplace(id);
        if (inserted) {
            it->second.id = id;
            order.push_back(id);
        }
        return it->second;
    }

    Resource& get(const std::string& id) { return ensureResource(id); }

    const Resource* find(const std::string& id) const {
        auto it = resources.find(id);
        return it != resources.end() ? &it->second : nullptr;
    }

    bool canAfford(const std::vector<Cost>& costs) const {
        for (auto& c : costs) {
            auto it = resources.find(c.res);
            if (it == resources.end() || it->second.qty < c.qty) return false;
        }
        return true;
    }

    void pay(const std::vector<Cost>& costs) {
        for (auto& c : costs) {
            auto it = resources.find(c.res);
            if (it != resources.end()) {
                it->second.qty -= c.qty;
                if (it->second.qty < it->second.qmin) it->second.qty = it->second.qmin;
            }
        }
    }

    void add(const std::string& res, double qty) {
        auto& r = ensureResource(res);
        r.qty += qty;
        if (r.qmax > 0 && r.qty > r.qmax) r.qty = r.qmax;
    }
};
