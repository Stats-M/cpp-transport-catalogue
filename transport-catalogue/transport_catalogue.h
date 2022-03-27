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

// В будущих проектах после снятия ограничений перенести в отдельный файл типа utility.h/.cpp
namespace detail
{
	// Преобразует 1 строку в 2, разделенные count по порядку символом-разделителем
	std::pair<std::string_view, std::string_view> Split(std::string_view, char, int count = 1);
	// Удаляет пробелы (переводы строк, табуляцию и т.п.) из начала строки
	std::string_view Lstrip(std::string_view);
	// Удаляет пробелы (переводы строк, табуляцию и т.п.) с конца строки
	std::string_view Rstrip(std::string_view);
	// Удаляет пробелы (переводы строк, табуляцию и т.п.) со всех сторон строки
	std::string_view TrimString(std::string_view);
}

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
	RequestQueryType type;
	std::string query;
	std::string reply;
};


// Структура, хранящая информацию об остановке и определяющая
// методы работы с ней
struct Stop
{
public:
	std::string name;          // Название остановки
	//double latitude;           // Широта
	//double longitude;          // Долгота
	geo::Coordinates coords{ 0L,0L };   // Координаты

	friend std::ostream& operator<<(std::ostream& out, const Stop& stop);    // friend потому что оператор << должен иметь доступ
																			 // к внутреннему устройству класса/структуры
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

	friend std::ostream& operator<<(std::ostream& os, const Route* route);    // friend потому что оператор << должен иметь доступ
																			  // к внутреннему устройству класса/структуры
};


// Класс хэшера для unordered_map с ключом типа pair<const T*, const T*>
template <typename T>
class PairPointersHasher
{
public:
	std::size_t operator()(const std::pair<const T*, const T*> pair_of_pointers) const noexcept
	{
		// Снимаем константность (нельзя приводить тип для константного указателя)
		T* ptr1 = const_cast<T*>(pair_of_pointers.first);
		T* ptr2 = const_cast<T*>(pair_of_pointers.second);
		// Приведение несовместимых типов
		std::uintptr_t np_ptr1 = reinterpret_cast<std::uintptr_t>(ptr1);
		std::uintptr_t np_ptr2 = reinterpret_cast<std::uintptr_t>(ptr2);

		return hasher_(np_ptr1) * 37 + hasher_(np_ptr2);
	}

private:
	std::hash<std::uintptr_t> hasher_;
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

	void GetRouteInfo(std::string_view, std::string&);    // Возвращает строку с информацией о маршруте с номером из sv
	void GetBusesForStop(std::string_view, std::string&); // Возвращает строку с информацией об автобусах для останоки из sv

	void ProcessInputQuery(InputQuery&);       // Парсер входящих запросов, для которых модулем ввода данных
											   // определен их тип (т.е. заполнено поле InputQuery.InputQueryType)

private:
	std::deque<Stop> all_stops_data_;                                     // Дек с информацией обо всех остановках (реальные данные, не указатели)
	std::unordered_map<std::string_view, const Stop*> all_stops_map_;     // Словарь остановок (словарь с хэш-функцией)
	std::deque<Route> all_buses_data_;                                    // Дек с информацией обо всех маршрутах
	std::unordered_map<std::string_view, Route*> all_buses_map_;          // Словарь маршрутов (автобусов) (словарь с хэш-функцией)
	std::unordered_map<std::pair<const Stop*, const Stop*>, size_t, PairPointersHasher<Stop>> distances_map_;    // Словарь расстояний между остановками

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
