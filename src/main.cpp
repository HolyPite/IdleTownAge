#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "resource.h"
#include "building.h"

// Barre de ressources
static void draw_bar(SDL_Renderer* r,int x,int y,int w,int h,double v,double vmin,double vmax){
    SDL_Rect bg{ x,y,w,h };
    SDL_SetRenderDrawColor(r,40,38,46,255);
    SDL_RenderFillRect(r,&bg);
    double t=(v-vmin)/(vmax-vmin); if(t<0) t=0; if(t>1) t=1;
    SDL_Rect fg{ x+2,y+2,int((w-4)*t),h-4 };
    SDL_SetRenderDrawColor(r,90,180,120,255);
    SDL_RenderFillRect(r,&fg);
    SDL_SetRenderDrawColor(r,110,108,120,255);
    SDL_RenderDrawRect(r,&bg);
}

// Instance d’un bâtiment placé sur la carte
struct BuildingInstance {
    int type;    // index dans vector<Building>
    int x, y;    // position
};

int main(int argc,char* argv[]){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) return 1;
    SDL_Window* win=SDL_CreateWindow("Medieval Idle",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,0);
    if(!win){ SDL_Quit(); return 2; }
    SDL_Renderer* ren=SDL_CreateRenderer(win,-1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ SDL_DestroyWindow(win); SDL_Quit(); return 3; }

    // --- Ressources
    std::unordered_map<std::string,Resource> R;
    R["wood"] = Resource{ "wood", 50, 0, 200 };
    R["food"] = Resource{ "food", 50, 0, 150 };
    R["gold"] = Resource{ "gold", 70, 0, 0 };
    R["pop"]  = Resource{ "pop", 10, 0, 100 };

    // --- Bâtiments types
    std::vector<Building> buildings;
    buildings.emplace_back(
        "lumber","Cabane de bûcheron",
        std::vector<Cost>{{"wood",20},{"gold",5}},
        std::vector<Cost>{{"wood",0.15}}
    );
    buildings.emplace_back(
        "farm","Ferme",
        std::vector<Cost>{{"wood",15},{"gold",3}},
        std::vector<Cost>{{"food",0.25}}
    );
    buildings.emplace_back(
        "mine","Mine d'or",
        std::vector<Cost>{{"wood",30}},
        std::vector<Cost>{{"gold",0.2}},
        std::vector<Cost>{{"wood",0.05}} // consomme bois
    );
    buildings.emplace_back(
        "nursery","Nurserie",
        std::vector<Cost>{{"wood",25},{"gold",8}},
        std::vector<Cost>{{"pop",0.02}}
    );

    // --- Instances placées
    std::vector<BuildingInstance> placed;

    const double dt=0.1;
    double acc=0, title_acc=0;
    auto prev=std::chrono::high_resolution_clock::now();
    bool run=true;

    while(run){
        // --- Input
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) run=false;
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE) run=false;

            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_a){
                if(buildings[0].canAfford(R)){
                    buildings[0].build(R);
                    int idx = buildings[0].count-1;
                    placed.push_back({0, 500 + idx*70, 200});
                }
            }
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_z){
                if(buildings[1].canAfford(R)){
                    buildings[1].build(R);
                    int idx = buildings[1].count-1;
                    placed.push_back({1, 500 + idx*70, 300});
                }
            }
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_e){
                if(buildings[2].canAfford(R)){
                    buildings[2].build(R);
                    int idx = buildings[2].count-1;
                    placed.push_back({2, 500 + idx*70, 400});
                }
            }
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_r){
                if(buildings[3].canAfford(R)){
                    buildings[3].build(R);
                    int idx = buildings[3].count-1;
                    placed.push_back({3, 500 + idx*70, 500});
                }
            }
        }

        // --- Timing
        auto now=std::chrono::high_resolution_clock::now();
        double frame=std::chrono::duration<double>(now-prev).count();
        prev=now; acc+=frame; title_acc+=frame;

        int ticks=0;
        while(acc>=dt && ticks<10){
            double tsec = SDL_GetTicks()/1000.0;

            // Production
            for(auto& b : buildings){
                b.produce(R, dt);
            }

            // Conso population
            double need = R["pop"].qty * 0.02 * dt;
            if(R["food"].qty >= need) R["food"].qty -= need;

            // Autosell bois
            if(R["wood"].qmax>0 && R["wood"].qty > 0.8*R["wood"].qmax){
                double sell = R["wood"].qty - 0.8*R["wood"].qmax;
                R["gold"].qty += sell * R["wood"].price(tsec);
                R["wood"].qty -= sell;
            }
            ++ticks; acc-=dt;
        }

        // --- Titre fenêtre
        if(title_acc>=0.5){
            char buf[256];
            std::snprintf(buf,sizeof(buf),
                "Medieval Idle | wood=%.1f food=%.1f gold=%.1f pop=%.1f | Cabanes=%d Fermes=%d Mines=%d Nurseries=%d",
                R["wood"].qty, R["food"].qty, R["gold"].qty, R["pop"].qty,
                buildings[0].count, buildings[1].count, buildings[2].count, buildings[3].count);
            SDL_SetWindowTitle(win, buf);
            title_acc=0;
        }

        // --- Rendu
        SDL_SetRenderDrawColor(ren,20,18,24,255);
        SDL_RenderClear(ren);

        // Barres ressources
        int x=40,y=40,w=400,h=24,g=36;
        draw_bar(ren,x,y+0*g,w,h,R["wood"].qty,0,R["wood"].qmax>0?R["wood"].qmax:200);
        draw_bar(ren,x,y+1*g,w,h,R["food"].qty,0,R["food"].qmax>0?R["food"].qmax:150);
        draw_bar(ren,x,y+2*g,w,h,R["gold"].qty,0,200);
        draw_bar(ren,x,y+3*g,w,h,R["pop"].qty ,0,100);

        // Bâtiments placés
        for(auto& inst : placed){
            SDL_Rect rect{inst.x, inst.y, 64, 64};
            if(inst.type == 0) SDL_SetRenderDrawColor(ren,139,69,19,255);    // cabane marron
            if(inst.type == 1) SDL_SetRenderDrawColor(ren,34,139,34,255);    // ferme verte
            if(inst.type == 2) SDL_SetRenderDrawColor(ren,218,165,32,255);   // mine dorée
            if(inst.type == 3) SDL_SetRenderDrawColor(ren,173,216,230,255);  // nurserie bleu clair
            SDL_RenderFillRect(ren,&rect);
            SDL_SetRenderDrawColor(ren,0,0,0,255);
            SDL_RenderDrawRect(ren,&rect);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
