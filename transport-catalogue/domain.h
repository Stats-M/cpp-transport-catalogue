/*
 * В этом файле размещены классы/структуры, которые являются частью предметной области (domain)
 * приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 */

#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <functional>      // Для шаблона hash<>

namespace transport_catalogue
{

struct Stop;      // forward declaration
struct Route;     // forward declaration

// Тип: константный указатель на запись об остановке в БД остановок
using StopPtr = const Stop*;
// Тип: константный указатель на запись о маршруте в БД маршрутов
using RoutePtr = const Route*;

// Структура, хранящая информацию об остановке и определяющая
// методы работы с ней
struct Stop
{
	// Включаем конструктор по-умолчанию из-за наличия параметризованного конструктора
	Stop() = default;
	// Конструктор по имени и двум координатам
	Stop(const std::string_view, const double, const double);
	// Конструктор копирования на основе константного указателя (для возврата данных обработчикам)
	Stop(StopPtr);

	std::string name;                   // Название остановки
	geo::Coordinates coords{ 0L,0L };   // Координаты
};

// Структура, хранящая информацию о маршруте и определяющая
// методы работы с ней
struct Route
{
	// Включаем конструктор по-умолчанию из-за наличия параметризованного конструктора
	Route() = default;
    // Конструктор копирования на основе константного указателя (для возврата данных обработчикам)
	Route(RoutePtr);

	std::string route_name;            // Номер маршрута (название)
	std::vector<StopPtr> stops;        // Контейнер указателей на остановки маршрута
	size_t unique_stops_qty = 0U;      // Количество уникальных остановок на маршруте (кэшируем, т.к. изменяется только при перестроении маршрута)
	double geo_route_length = 0L;      // Длина маршрута по прямой между координатами (кэшируем, т.к. изменяется только при перестроении маршрута)
	size_t meters_route_length = 0U;   // Длина маршрута с учетом заданных расстояний между точками (метры) (кэшируем, т.к. изменяется только при перестроении маршрута)
	double curvature = 0L;             // Извилистость маршрута = meters_route_length / geo_route_length. >1 для любого маршрута, кроме подземного
	bool is_circular = false;          // Является ли маршрут кольцевым
};

// Структура для передачи координат в рендерер карт
struct RendererData
{
	std::vector<geo::Coordinates> stop_coords;    // Контейнер координат остановок
	std::vector<std::string_view> stop_names;     // Контейнер указателей на названия остановок
	bool is_circular = false;                     // Является ли маршрут кольцевым
};

// Класс хэшера для unordered_map с ключом типа pair<StopPtr, StopPtr>
class PairPointersHasher
{
public:
	std::size_t operator()(const std::pair<StopPtr, StopPtr> pair_of_pointers) const noexcept
	{
		//auto ptr1 = static_cast<const void*>(pair_of_pointers.first);
		//auto ptr2 = static_cast<const void*>(pair_of_pointers.second);
		//return hasher_(ptr1) * 37 + hasher_(ptr2);

		return hasher_(static_cast<const void*>(pair_of_pointers.first)) * 37
			+ hasher_(static_cast<const void*>(pair_of_pointers.second));

		/* Код хэшера для шаблонного варианта PairPointersHasher<T==Stop>
		// Снимаем константность (нельзя приводить тип для константного указателя)
		T* ptr1 = const_cast<T*>(pair_of_pointers.first);
		T* ptr2 = const_cast<T*>(pair_of_pointers.second);
		// Приведение несовместимых типов
		std::uintptr_t np_ptr1 = reinterpret_cast<std::uintptr_t>(ptr1);
		std::uintptr_t np_ptr2 = reinterpret_cast<std::uintptr_t>(ptr2);
		return hasher_(np_ptr1) * 37 + hasher_(np_ptr2);
		*/
	}

private:
	// Хэш - функция для шаблонного варианта PairPointersHasher<T == Stop>
	//std::hash<std::uintptr_t> hasher_;

	// Стандартная хэш-функция для const void*
	std::hash<const void*> hasher_;
};

} // namespace transport_catalogue