#include <SDL2/SDL.h>
#include <SDL2/SDL_filesystem.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>
#include "resource_manager.h"
#include "building_manager.h"

using json = nlohmann::json;

// Simple horizontal resource bar
static void draw_bar(SDL_Renderer* r, int x, int y, int w, int h, double v, double vmin, double vmax) {
    SDL_Rect bg{ x, y, w, h };
    SDL_SetRenderDrawColor(r, 40, 38, 46, 255);
    SDL_RenderFillRect(r, &bg);
    double t = (v - vmin) / (vmax - vmin);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    SDL_Rect fg{ x + 2, y + 2, int((w - 4) * t), h - 4 };
    SDL_SetRenderDrawColor(r, 90, 180, 120, 255);
    SDL_RenderFillRect(r, &fg);
    SDL_SetRenderDrawColor(r, 110, 108, 120, 255);
    SDL_RenderDrawRect(r, &bg);
}

static bool read_json_file(const std::string& path, json& out) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::printf("Erreur: impossible d'ouvrir %s\n", path.c_str());
        return false;
    }
    try {
        f >> out;
    } catch (const std::exception& ex) {
        std::printf("Erreur: lecture JSON %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
    return true;
}

void loadResources(ResourceManager& rm, const std::string& path) {
    json jr;
    if (!read_json_file(path, jr)) return;
    for (auto& r : jr) {
        const std::string id = r.value("id", "");
        if (id.empty()) continue;
        rm.resources[id] = {
            id,
            r.value("qty", 0.0),
            0.0,
            r.value("qmax", 0.0)
        };
    }
}

void loadBuildings(BuildingManager& bm, const std::string& path) {
    json jb;
    if (!read_json_file(path, jb)) return;
    for (auto& b : jb) {
        const std::string id = b.value("id", "");
        const std::string name = b.value("name", id);
        std::vector<Cost> cost, inputs, outputs;
        if (b.contains("cost")) {
            for (auto& c : b["cost"]) {
                cost.push_back({ c.value("res", std::string{}), c.value("qty", 0.0) });
            }
        }
        if (b.contains("inputs")) {
            for (auto& i : b["inputs"]) {
                inputs.push_back({ i.value("res", std::string{}), i.value("qty", 0.0) });
            }
        }
        if (b.contains("outputs")) {
            for (auto& o : b["outputs"]) {
                outputs.push_back({ o.value("res", std::string{}), o.value("qty", 0.0) });
            }
        }
        if (!id.empty()) {
            bm.addPrototype(Building(id, name, cost, outputs, inputs));
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
    SDL_Window* win = SDL_CreateWindow("Medieval Idle",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    if (!win) { SDL_Quit(); return 2; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { SDL_DestroyWindow(win); SDL_Quit(); return 3; }

    ResourceManager rm;
    BuildingManager bm;

    std::filesystem::path dataDir;
    if (char* base = SDL_GetBasePath()) {
        dataDir = std::filesystem::path(base);
        SDL_free(base);
    } else {
        dataDir = std::filesystem::current_path();
    }
    dataDir /= "data";

    loadResources(rm, (dataDir / "resources.json").string());
    loadBuildings(bm, (dataDir / "buildings.json").string());

    if (rm.resources.empty()) {
        std::printf("Avertissement: aucune ressource chargee depuis %s\n", (dataDir / "resources.json").string().c_str());
    }
    if (bm.prototypes.empty()) {
        std::printf("Avertissement: aucun batiment charge depuis %s\n", (dataDir / "buildings.json").string().c_str());
    }

    const double dt = 0.1;
    double acc = 0.0;
    double title_acc = 0.0;
    auto prev = std::chrono::high_resolution_clock::now();
    bool run = true;

    auto triggerBuild = [&](int index, int baseX, int baseY) {
        if (index < 0 || index >= static_cast<int>(bm.prototypes.size())) return;
        int x = baseX + bm.prototypes[index].count * 70;
        bm.tryBuild(index, rm, x, baseY);
    };

    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) run = false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_a: triggerBuild(0, 500, 200); break;
                    case SDLK_z: triggerBuild(1, 500, 300); break;
                    case SDLK_e: triggerBuild(2, 500, 400); break;
                    case SDLK_r: triggerBuild(3, 500, 500); break;
                    default: break;
                }
            }
        }

        auto now = std::chrono::high_resolution_clock::now();
        double frame = std::chrono::duration<double>(now - prev).count();
        prev = now;
        acc += frame;
        title_acc += frame;

        int ticks = 0;
        while (acc >= dt && ticks < 10) {
            bm.produceAll(rm, dt);
            auto popIt = rm.resources.find("pop");
            auto foodIt = rm.resources.find("food");
            if (popIt != rm.resources.end() && foodIt != rm.resources.end()) {
                double need = popIt->second.qty * 0.02 * dt;
                if (foodIt->second.qty >= need) {
                    foodIt->second.qty -= need;
                }
            }
            acc -= dt;
            ++ticks;
        }

        if (title_acc >= 0.5) {
            auto getQty = [&](const char* id) {
                auto it = rm.resources.find(id);
                return it != rm.resources.end() ? it->second.qty : 0.0;
            };
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                "Idle | wood=%.1f food=%.1f gold=%.1f pop=%.1f",
                getQty("wood"), getQty("food"), getQty("gold"), getQty("pop"));
            SDL_SetWindowTitle(win, buf);
            title_acc = 0.0;
        }

        auto drawResource = [&](const char* id, int y, double fallbackMax) {
            auto it = rm.resources.find(id);
            double qty = it != rm.resources.end() ? it->second.qty : 0.0;
            double vmax = fallbackMax;
            if (it != rm.resources.end() && it->second.qmax > 0.0) {
                vmax = it->second.qmax;
            }
            draw_bar(ren, 40, y, 400, 24, qty, 0.0, vmax);
        };

        SDL_SetRenderDrawColor(ren, 20, 18, 24, 255);
        SDL_RenderClear(ren);
        drawResource("wood", 40, 200.0);
        drawResource("food", 76, 150.0);
        drawResource("gold", 112, 200.0);
        drawResource("pop", 148, 100.0);
        bm.render(ren);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
