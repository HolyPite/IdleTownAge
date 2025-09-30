#pragma once
#include <string>
#include <functional>

struct Resource {
    std::string id;
    double qty=0, qmin=0, qmax=0;
    std::function<double(double)> rps;
    std::function<double(double,const Resource&)> price_fn;

    void tick(double t, double dt){
        if(rps) qty += rps(t)*dt;
        if(qty<qmin) qty=qmin;
        if(qmax>0 && qty>qmax) qty=qmax;
    }
    double price(double t) const { return price_fn ? price_fn(t,*this) : 0.0; }
};
