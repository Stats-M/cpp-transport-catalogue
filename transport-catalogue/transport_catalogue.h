#pragma once
#include "geo.h"           // для работы с координатами остановок

#include <deque>
#include <string>
#include <string_view>
#include <ostream>         // для перегрузки оператора вывода
#include <sstream>         // для stringstream
#include <iomanip>         // для управления выводом (манипуляторы)
#include <unordered_set>
#include <unordered_map>
#include <algorithm>       // для uniq(), find()
#include <utility>         // for std::pair<>
#include <cctype>          // for isspace() function
#include <functional>      // Для шаблона hash<>


// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue
{

// Тип запросов на добавление информации в базу данных. Определяет парсер входных данных.
enum class InputQueryType
{
	NoOp,            // значение по-умолчанию, нет операции
	AddStop,         // добавить информацию об остановке (имя и координаты) ЭТАП-1
	AddRoute,        // добавить информацию о маршруте (все остановки должны существовать) ЭТАП-3
	AddStopsDistance,// добавить информацию об остановке (задание расстояний до соседних остановок) ЭТАП-2
};

// Структура запроса на добавление информации.
// Внешний парсер входных данных проверяет базовую корректность запроса (тип, наличие тела запроса), т.к. 
// только он знает формат работы источника данных и нужную очередность направления запросов в справочник.
// Парсер запроса самого транспортного справочника производит логический разбор тела запроса.
struct InputQuery
{
	InputQueryType type = InputQueryType::NoOp;
	std::string query;
};

// Тип запросов на добавление информации в базу данных. Определяет парсер входных данных.
enum class RequestQueryType
{
	NoOp,            // значение по-умолчанию, нет операции
	GetRouteByName,  // получить маршрут по имени
	GetBusesForStop, // получить маршруты для остановки
};

// Структура запроса на поиск информации
struct RequestQuery
{
	RequestQueryType type{ RequestQueryType::NoOp};
	std::string_view params;
};

// Код результата выполнение запроса к базе данных.
enum class RequestResultType
{
	Ok,                  // значение по-умолчанию
	NoBuses,             // результат: маршруты не найдены
	StopNotExists,       // результат: остановка не найдена
	RouteNotExists,      // результат: маршруи не найден
};

// Структура, хранящая информацию об остановке и определяющая
// методы работы с ней
struct Stop
{
public:
	std::string name;                   // Название остановки
	//double latitude;                  // Широта
	//double longitude;                 // Долгота
	geo::Coordinates coords{ 0L,0L };   // Координаты
};

// Структура, хранящая информацию о маршруте (автобусе) и определяющая
// методы работы с ней
struct Route
{
	std::string bus_number;            // Номер маршрута (название)
	std::vector<const Stop*> stops;    // Контейнер указателей на остановки маршрута
	size_t unique_stops_qty = 0U;      // Количество уникальных остановок на маршруте (кэшируем, т.к. изменяется только при перестроении маршрута)
	double geo_route_length = 0L;      // Длина маршрута по прямой между координатами (кэшируем, т.к. изменяется только при перестроении маршрута)
	size_t meters_route_length = 0U;   // Длина маршрута с учетом заданных расстояний между точками (метры) (кэшируем, т.к. изменяется только при перестроении маршрута)
	double curvature = 0L;             // Извилистость маршрута = meters_route_length / geo_route_length. >1 для любого маршрута, кроме подземного
};

// Структура для возврата результата запросов stat_reader
struct RequestResult
{
	RequestResultType code = RequestResultType::Ok;   // Код завершения (если это требуется вызывающему методу)
	std::vector<std::string> vector_str;              // Вектор строк. Порядок использования элементов определяется вызывающим методом.
	const Stop* s_ptr = nullptr;               // Указатель на структуру Остановка. Порядок использования определяется вызывающим методом.
	const Route* r_ptr = nullptr;              // Указатель на структуру Маршрут (автобус). Порядок использования определяется вызывающим методом.
};


// Класс хэшера для unordered_map с ключом типа pair<const Stop*, const Stop*>
class PairPointersHasher
{
public:
	std::size_t operator()(const std::pair<const Stop*, const Stop*> pair_of_pointers) const noexcept
	{
		auto ptr1 = static_cast<const void*>(pair_of_pointers.first);
		auto ptr2 = static_cast<const void*>(pair_of_pointers.second);
		return hasher_(ptr1) * 37 + hasher_(ptr2);

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
	//std::hash<std::uintptr_t> hasher_;    Хэш-функция для шаблонного варианта PairPointersHasher<T==Stop>
	std::hash<const void*> hasher_;
};


class TransportCatalogue
{
public:
	TransportCatalogue();
	~TransportCatalogue();

	void AddStop(Stop&&);              // Добавляет остановку в словарь всех остановок
	void AddRoute(Route&&);            // Добавляет маршрут в словарь всех маршрутов
	void AddDistance(const Stop*, const Stop*, size_t);    // Добавляет расстояние между двумя остановками в словарь
	size_t GetDistance(const Stop*, const Stop*);          // Возвращает расстояние (size_t метры) между двумя остановками с перестановкой пары
	size_t GetDistanceDirectly(const Stop*, const Stop*);  // Возвращает расстояние (size_t метры) между двумя остановками без перестановки пары

	const Stop* GetStopByName(std::string_view);    // Возвращает указатель на структуру остановки по ее имени
	Route* GetRouteByName(std::string_view);        // Возвращает указатель на структуру маршрута по его имени

	RequestResult GetRouteInfo(std::string_view);         // Возвращает строку с информацией о маршруте с номером из sv
	RequestResult GetBusesForStop(std::string_view);      // Возвращает строку с информацией об автобусах для останоки из sv

private:
	std::deque<Stop> all_stops_data_;                                     // Дек с информацией обо всех остановках (реальные данные, не указатели)
	std::unordered_map<std::string_view, const Stop*> all_stops_map_;     // Словарь остановок (словарь с хэш-функцией)
	std::deque<Route> all_buses_data_;                                    // Дек с информацией обо всех маршрутах
	std::unordered_map<std::string_view, Route*> all_buses_map_;          // Словарь маршрутов (автобусов) (словарь с хэш-функцией)
	std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, PairPointersHasher> distances_map_;    // Словарь расстояний между остановками

	// Возвращает string_view с именем остановки по указателю на экземпляр структуры Stop
	std::string_view GetStopName(const Stop* stop_ptr);
	// Возвращает string_view с именем остановки для экземпляра структуры Stop
	std::string_view GetStopName(const Stop stop);

	// Возвращает string_view с номером автобуса по указателю на экземпляр структуры Route
	std::string_view GetBusName(const Route* route_ptr);
	// Возвращает string_view с номером автобуса для экземпляра структуры Route
	std::string_view GetBusName(const Route route);
};
}
