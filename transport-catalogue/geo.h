#pragma once

// Этот include нужно включать если компилятор ругается на необъявленный тип size_t
// Обычно size_t определяется в заголовках с контейнерами STL, но при их отсутствии 
// может вызвать ошибку. В этом случае нужно добавить #include <cstddef>
#include <cstddef>

namespace geo
{

// Радиус Земли, метров
const int EARTH_RADIUS = 6371000;

// Структура, определяющая положение точки в пространстве
// и методы работы с ней
struct Coordinates
{
    double lat;  // Широта
    double lng;  // Долгота
    bool operator==(const Coordinates&) const;
    bool operator!=(const Coordinates&) const;
};

class CoordinatesHasher
{
public:
    std::size_t operator()(const Coordinates&) const;
};

// Функция рассчитывает расстояние между остановками по поверхности
// земного шара
double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
