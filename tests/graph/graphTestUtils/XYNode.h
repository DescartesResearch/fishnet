#pragma once
#include <functional>
#include <math.h>
#include <fishnet/ObjectConcepts.hpp>

class XYNode {
private:
    double x;
    double y;
public:
    XYNode(double x, double y):x(x),y(y){}

    bool operator==(const XYNode & other) const {
        return this->x == other.x and this->y == other.y;
    }

    double getX() const {
        return this->x;
    }

    double getY()const {
        return this->y;
    }

    double distanceTo(const XYNode & other) const  {
        double deltaX = this->getX() - other.getX();
        double deltaY =this->getY() - other.getY();
        return sqrt(deltaX*deltaX + deltaY*deltaY);
    }

    std::string toString() const {
        return "("+std::to_string(this->x)+","+std::to_string(this->y)+")";
    }

    size_t hash() const {
        size_t x_hash = std::hash<double>{}(this->x);
        size_t y_hash = std::hash<double>{}(this->y);
        return ((x_hash + y_hash+1)* (x_hash+y_hash)) / 2 + y_hash;
    }
};

static_assert(fishnet::util::Object<XYNode>,"XYNode should be an object");
