#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "resource.h"
#include "building.h"

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

int main(int argc,char* argv[]){
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) return 1;
    SDL_Window* win=SDL_CreateWindow("Medieval Idle",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1280,720,0);
    if(!win){ SDL_Quit(); return 2; }
    SDL_Renderer* ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ SDL_DestroyWindow(win); SDL_Quit(); return 3; }

    // --- Ressources
    std::unordered_map<std::string,Resource> R;
    R["wood"] = Resource{ "wood", 50,   0, 200 };
    R["food"] = Resource{ "food", 50,  0, 150 };
    R["gold"] = Resource{ "gold", 70,   0,   0 };   // qmax=0 => sans limite
    R["pop"]  = Resource{ "pop",  10,  0, 100 };

    // --- Bâtiments
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
                buildings[0].build(R); // Cabane
            }
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_z){
                buildings[1].build(R); // Ferme
            }
        }


        // --- Timing
        auto now=std::chrono::high_resolution_clock::now();
        double frame=std::chrono::duration<double>(now-prev).count();
        prev=now; acc+=frame; title_acc+=frame;

        int ticks=0;
        while(acc>=dt && ticks<10){
            double tsec = SDL_GetTicks()/1000.0;

            // Production par bâtiment
            for(auto& b : buildings){
                b.produce(R, dt);
            }

            // Conso population
            double need = R["pop"].qty * 0.02 * dt;
            if(R["food"].qty >= need) R["food"].qty -= need;

            // Autosell bois si >80% du stock
            if(R["wood"].qmax>0 && R["wood"].qty > 0.8*R["wood"].qmax){
                double sell = R["wood"].qty - 0.8*R["wood"].qmax;
                R["gold"].qty += sell * R["wood"].price(tsec);
                R["wood"].qty -= sell;
            }
            ++ticks; acc-=dt;
        }

        // --- Titre fenêtre
        if(title_acc>=0.5){
            char buf[200];
            std::snprintf(buf,sizeof(buf),
                "Medieval Idle | wood=%.1f food=%.1f gold=%.1f pop=%.1f | Cabanes=%d Fermes=%d",
                R["wood"].qty, R["food"].qty, R["gold"].qty, R["pop"].qty,
                buildings[0].count, buildings[1].count);
            SDL_SetWindowTitle(win, buf);
            title_acc=0;
        }

        // --- Rendu HUD
        SDL_SetRenderDrawColor(ren,20,18,24,255);
        SDL_RenderClear(ren);

        int x=40,y=40,w=400,h=24,g=36;
        draw_bar(ren,x,y+0*g,w,h,R["wood"].qty,0,R["wood"].qmax>0?R["wood"].qmax:200);
        draw_bar(ren,x,y+1*g,w,h,R["food"].qty,0,R["food"].qmax>0?R["food"].qmax:150);
        draw_bar(ren,x,y+2*g,w,h,R["gold"].qty,0,200);
        draw_bar(ren,x,y+3*g,w,h,R["pop"].qty ,0,100);

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
