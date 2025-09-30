#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "building.h"
#include "resource_manager.h"

struct BuildingInstance {
    int type;
    int x, y;
};

class BuildingManager {
public:
    std::vector<Building> prototypes;
    std::vector<BuildingInstance> placed;

    void addPrototype(const Building& b) { prototypes.push_back(b); }

    void tryBuild(int index, ResourceManager& rm, int x, int y) {
        if(index < 0 || index >= (int)prototypes.size()) return;
        Building& proto = prototypes[index];
        if(proto.canAfford(rm.resources)) {
            proto.build(rm.resources);
            placed.push_back({index, x, y});
        }
    }

    void produceAll(ResourceManager& rm, double dt) {
        for(auto& proto : prototypes) proto.produce(rm.resources, dt);
    }

    void render(SDL_Renderer* ren) {
        for(auto& inst : placed){
            SDL_Rect rect{inst.x, inst.y, 64, 64};
            switch(inst.type){
                case 0: SDL_SetRenderDrawColor(ren,139,69,19,255); break;
                case 1: SDL_SetRenderDrawColor(ren,34,139,34,255); break;
                case 2: SDL_SetRenderDrawColor(ren,218,165,32,255); break;
                case 3: SDL_SetRenderDrawColor(ren,173,216,230,255); break;
                default: SDL_SetRenderDrawColor(ren,200,200,200,255); break;
            }
            SDL_RenderFillRect(ren,&rect);
            SDL_SetRenderDrawColor(ren,0,0,0,255);
            SDL_RenderDrawRect(ren,&rect);
        }
    }
};
