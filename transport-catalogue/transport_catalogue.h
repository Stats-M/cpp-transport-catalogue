#pragma once
#include "geo.h"           // для работы с координатами остановок
#include "domain.h"        // классы основных сущностей, описывают автобусы и остановки

#include <deque>
#include <map>             // для словаря координат рендерера карт
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <ostream>         // для перегрузки оператора вывода
#include <sstream>         // для stringstream
#include <iomanip>         // для управления выводом (манипуляторы)
#include <unordered_set>
#include <unordered_map>
#include <algorithm>       // для uniq(), find()
#include <utility>         // for std::pair<>
#include <cctype>          // for isspace() function

namespace transport_catalogue
{

// ----------- Секция типов для ответов на запросы---------------

// Структура ответа на запросы типа Информация об остановке и ее маршрутах
struct StopStat
{
	// Параметризованный конструктор
	explicit StopStat(std::string_view, std::set<std::string_view>&);
	std::string_view name;
	std::set<std::string_view> buses;  // Должны быть отсортированными
};

// Тип: константный указатель на статистику об остановке и ее маршрутах
using StopStatPtr = const StopStat*;

// Структура ответа на запросы типа Информация о маршруте
struct RouteStat
{
	// Параметризованный конструктор
	explicit RouteStat(size_t, size_t, int64_t, double, std::string_view);
	size_t stops_on_route = 0;
	size_t unique_stops = 0;
	int64_t meters_route_length = 0;
	double curvature = 0L;
	std::string name;
};

// Тип: константный указатель на статистику о маршруте
using RouteStatPtr = const RouteStat*;

// ----------- TransportCatalogue ---------------

class TransportCatalogue
{
public:
	TransportCatalogue();
	~TransportCatalogue();

	void AddStop(Stop&&);              // Добавляет остановку в словарь всех остановок
	void AddRoute(Route&&);            // Добавляет маршрут в словарь всех маршрутов
	void AddDistance(StopPtr, StopPtr, size_t);    // Добавляет расстояние между двумя остановками в словарь
	size_t GetDistance(StopPtr, StopPtr);          // Возвращает расстояние (size_t метры) между двумя остановками с перестановкой пары
	size_t GetDistanceDirectly(StopPtr, StopPtr);  // Возвращает расстояние (size_t метры) между двумя остановками без перестановки пары

	StopPtr GetStopByName(const std::string_view) const;    // Возвращает указатель на остановку по ее имени из словаря остановок
	RoutePtr GetRouteByName(const std::string_view) const;  // Возвращает указатель на маршрут по его имени из словаря маршрутов

	RouteStatPtr GetRouteInfo(const std::string_view) const;        // Возвращает указатель на результат запроса о маршруте
	StopStatPtr GetBusesForStopInfo(const std::string_view) const;  // Возвращает указатель на результат запроса об автобусах для останоки

	void GetAllRoutes(std::map<const std::string, RendererData>&) const;    // Возвращает словарь маршрутов с их остановками

	size_t GetAllStopsCount() const;                     // ROUTER. Возвращает количество уникальных остановок в базе
	const std::vector<StopPtr> GetAllStopsPtr() const;   // ROUTER. Возвращает вектор указателей на остановки
	const std::deque<RoutePtr> GetAllRoutesPtr() const;  // ROUTER. Возвращает вектор указателей на маршруты

	// SERIALIZER. Возвращает read-only словарь расстояний между всеми остановками
	const std::unordered_map<std::pair<StopPtr, StopPtr>, size_t, PairPointersHasher>& GetAllDistances() const;


private:
	std::deque<Stop> all_stops_data_;                                     // Дек с информацией обо всех остановках (реальные данные, не указатели)
	std::unordered_map<std::string_view, StopPtr> all_stops_map_;         // Словарь остановок (словарь с хэш-функцией)
	std::deque<Route> all_buses_data_;                                    // Дек с информацией обо всех маршрутах
	std::unordered_map<std::string_view, RoutePtr> all_buses_map_;        // Словарь маршрутов (автобусов) (словарь с хэш-функцией)
	std::unordered_map<std::pair<StopPtr, StopPtr>, size_t, PairPointersHasher> distances_map_;    // Словарь расстояний между остановками

	// Возвращает string_view с именем остановки по указателю на экземпляр структуры Stop
	std::string_view GetStopName(StopPtr stop_ptr);
	// Возвращает string_view с именем остановки для экземпляра структуры Stop
	std::string_view GetStopName(const Stop stop);

	// Возвращает string_view с номером автобуса по указателю на экземпляр структуры Route
	std::string_view GetBusName(RoutePtr route_ptr);
	// Возвращает string_view с номером автобуса для экземпляра структуры Route
	std::string_view GetBusName(const Route route);
};
}
