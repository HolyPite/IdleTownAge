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
        if (index < 0 || index >= (int)prototypes.size()) return;
        Building& proto = prototypes[index];
        if (proto.canAfford(rm.resources)) {
            proto.build(rm.resources);
            placed.push_back({ index, x, y });
        }
    }

    void produceAll(ResourceManager& rm, double dt) {
        for (auto& proto : prototypes) proto.produce(rm.resources, dt);
    }

    void render(SDL_Renderer* ren) {
        static const SDL_Color palette[] = {
            { 139, 69, 19, 255 },
            { 34, 139, 34, 255 },
            { 218, 165, 32, 255 },
            { 173, 216, 230, 255 },
            { 205, 92, 92, 255 },
            { 123, 104, 238, 255 },
            { 240, 128, 128, 255 },
            { 95, 158, 160, 255 }
        };
        const int paletteSize = static_cast<int>(sizeof(palette) / sizeof(palette[0]));

        for (auto& inst : placed) {
            SDL_Rect rect{ inst.x, inst.y, 64, 64 };
            SDL_Color color = palette[paletteSize > 0 ? inst.type % paletteSize : 0];
            SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(ren, &rect);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderDrawRect(ren, &rect);
        }
    }
};
