#define _USE_MATH_DEFINES  // Для константы Пи

#include "geo.h"

#include <cmath>

namespace geo
{

bool Coordinates::operator==(const Coordinates& other) const
{
    return lat == other.lat && lng == other.lng;
}

bool Coordinates::operator!=(const Coordinates& other) const
{
    return !(*this == other);
}

std::size_t CoordinatesHasher::operator()(const Coordinates& coords) const
{
    return static_cast<std::size_t>(coords.lat + 37 * coords.lng);
}

double ComputeDistance(Coordinates from, Coordinates to)
{
    using namespace std;
    if (from == to)
    {
        return 0;
    }
    static const double dr = M_PI / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr)) 
        * EARTH_RADIUS;
}

}
