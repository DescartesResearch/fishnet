#pragma once
#include "Constants.hpp"
#include <cmath>
#include <stdexcept>
#include <functional>
#include <fishnet/ObjectConcepts.hpp>

namespace fishnet::math::__impl{
/**
 * @brief Abstract Angle using CRTP
 * 
 * @tparam AngleImpl angle implementation type
 */
template<typename AngleImpl>
class AbstractAngle{
private:
    AngleImpl& impl(){
        return static_cast<AngleImpl &>(*this);
    }

    const AngleImpl & impl() const {
        return static_cast<const AngleImpl &>(*this);
    }

protected:
    double angle;

    double normalizedAngle(const double newAngle)const{
        return impl().normalizedAngle(newAngle);
    }

    void setAngle(const double newAngle){
        [[unlikely]] if (std::isnan(newAngle)) 
            throw std::invalid_argument("NaN cannot be converted to angle!");
        this->angle = normalizedAngle(newAngle);
    }

    AbstractAngle() : angle(0.0) {}

public:

    static AngleImpl asin(double sine){
        return AngleImpl::asin(sine);
    }

    static AngleImpl acos(double cosine){
        return AngleImpl::acos(cosine);
    }

    static AngleImpl atan(double tangent){
        return AngleImpl::atan(tangent);
    }

    static AngleImpl atan2(double y, double x){
        return AngleImpl::atan2(y,x);
    }

    AngleImpl operator - () const {
        return AngleImpl(-angle);
    }

    AngleImpl operator + (const AngleImpl & other) const {
        return AngleImpl(angle + other.angle);
    }

    AngleImpl operator - (const AngleImpl & other) const {
        return AngleImpl(angle - other.angle);
    }

    AngleImpl & operator+=(const AngleImpl & other) {
        setAngle(this->angle + other.angle);
        return impl();
    }

    AngleImpl & operator-=(const AngleImpl & other){
        setAngle(this->angle + other.angle);
        return impl();
    }

    bool operator>(const AngleImpl & other) const {
        if(*this == other) return false;
        return angle > other.angle;
    }

    bool operator>=(const AngleImpl & other) const {
        return *this == other || *this > other;
    }

    bool operator==(const AngleImpl & other) const {
        return fabs(angle-other.angle) < DOUBLE_EPSILON;
    }

    bool operator<=(const AngleImpl & other) const {
        return *this == other || *this < other;
    }

    bool operator< (const AngleImpl & other) const {
        if(*this == other) return false;
        return angle < other.angle;
    }

    double sin() const {
        return impl().sin();
    }

    double cos() const {
        return impl().cos();
    }

    double tan() const {
        return impl().tan();
    }

    double getAngleValue() const {
        return angle;
    } 

    size_t hash() const noexcept {
        return std::hash<double>{}(this->getAngleValue());
    }
};
}
namespace fishnet::math{
// Forward declaration
class Radians;
class Degrees;
}