#include "transport_catalogue.h"
#include <algorithm>   // для sort

namespace transport_catalogue
{

// ----------- Секция типов для ответов на запросы---------------

StopStat::StopStat(std::string_view stop_name, std::set<std::string_view>& buses) :
	name(stop_name), buses(buses)
{}

RouteStat::RouteStat(size_t stops, size_t unique_stops, int64_t meters_length, double curvature, std::string_view name) :
	stops_on_route(stops),
	unique_stops(unique_stops),
	meters_route_length(meters_length),
	curvature(curvature),
	name(name)
{}

// ----------- TransportCatalogue ---------------

TransportCatalogue::TransportCatalogue()
{}

TransportCatalogue::~TransportCatalogue()
{}

// Добавляет остановку в словарь всех остановок
void TransportCatalogue::AddStop(Stop&& stop)
{
	if (all_stops_map_.count(GetStopName(&stop)) == 0)
	{
		// Таких остановок в базе нет, добавлям

		// 1. Добавляем остановку в дек-хранилище, перемещая stop
		auto& ref = all_stops_data_.emplace_back(std::move(stop));
		// 2. Добавляем остановку в словарь остановок
		all_stops_map_.insert({ std::string_view(ref.name), &ref });
	}
}

// Добавляет маршрут в словарь всех маршрутов
void TransportCatalogue::AddRoute(Route&& route)
{
	// Отсутствует ли этот маршрут route в базе?
	if (all_buses_map_.count(route.route_name) == 0)
	{
		// Такого маршрута в базе нет, добавлям

		/* Проверка отключена: в route передается vector<StopPtr>, то что они не
		* ==nullptr будет проверять вызывающая функция, т.к. быстрее не добавить
		* пустой указатель, чем потом проходить весь вектор в поисках пустых записей
		// 1 Проверяем существование остановок, несуществующие удаляем из route
		route.stops.erase(std::remove_if(route.stops.begin(), route.stops.end(),
							[&](auto& x)
							{
								return (all_stops_map_.count(x->name) == 0);
							}),
						  route.stops.end());
		*/

		// 2. Добавляем маршрут (автобус) в дек-хранилище, перемещая route
		auto& ref = all_buses_data_.emplace_back(std::move(route));

		// 3. Добавляем указатель на автобус (маршрут) из хранилища в словарь маршрутов
		all_buses_map_.insert({ std::string_view(ref.route_name), &ref });

		// 4. Подсчитываем уникальные остановки на маршруте
		// Копируем вектор указателей
		std::vector<StopPtr> tmp = ref.stops;
		std::sort(tmp.begin(), tmp.end());
		auto last = std::unique(tmp.begin(), tmp.end());
		// Сохраняем количество уникальных остановок на маршруте
		ref.unique_stops_qty = (last != tmp.end() ? std::distance(tmp.begin(), last) : tmp.size());

		// 5. Если маршрут НЕ кольцевой, достраиваем обратный маршрут
		if (!ref.is_circular)
		{
			// Достраиваем обратный маршрут для некольцевого маршрута
			for (int i = ref.stops.size() - 2; i >= 0; --i)
			{
				// Добавляем в конец вектора остановки в обратном направлении, кроме конечной stops_list[size() - 1]
				ref.stops.push_back(ref.stops[i]);
			}
		}

		// 6. Подсчитываем длину маршрута
		int stops_num = static_cast<int>(ref.stops.size());
		if (stops_num > 1)
		{
			ref.geo_route_length = 0L;
			ref.meters_route_length = 0U;
			for (int i = 0; i < stops_num - 1; ++i)
			{
				ref.geo_route_length += ComputeDistance(ref.stops[i]->coords, ref.stops[i + 1]->coords);
				ref.meters_route_length += GetDistance(ref.stops[i], ref.stops[i + 1]);
			}
			// Рассчитываем кривизну маршрута
			ref.curvature = ref.meters_route_length / ref.geo_route_length;
		}
		else
		{
			// У маршрута 0 или 1 остановка. Длина == 0
			ref.geo_route_length = 0L;
			ref.meters_route_length = 0U;
			ref.curvature = 1L;
		}
	}
}

// Добавляет расстояние между двумя остановками в словарь
void TransportCatalogue::AddDistance(StopPtr stop_from, StopPtr stop_to, size_t dist)
{
	if (stop_from != nullptr && stop_to != nullptr)
	{
		// Вносим запись в словарь расстояний
		distances_map_.insert({ { stop_from, stop_to }, dist });
	}
}

// Возвращает расстояние (size_t метры) между двумя остановками с перестановкой пары
size_t TransportCatalogue::GetDistance(StopPtr stop_from, StopPtr stop_to)
{
	size_t result = GetDistanceDirectly(stop_from, stop_to);
	// Если прямого расстояния в словаре нет, возвращаем обратное расстояние, даже если оно 0
	return (result > 0 ? result : GetDistanceDirectly(stop_to, stop_from));
}

// Возвращает расстояние (size_t метры) между двумя остановками без перестановки пары
size_t TransportCatalogue::GetDistanceDirectly(StopPtr stop_from, StopPtr stop_to)
{
	if (distances_map_.count({ stop_from, stop_to }) > 0)
	{
		return distances_map_.at({ stop_from, stop_to });
	}
	else
	{
		return 0U;
	}
}


std::string_view TransportCatalogue::GetStopName(StopPtr stop_ptr)
{
	return std::string_view(stop_ptr->name);
}

std::string_view TransportCatalogue::GetStopName(const Stop stop)
{
	return std::string_view(stop.name);
}

std::string_view TransportCatalogue::GetBusName(RoutePtr route_ptr)
{
	return std::string_view(route_ptr->route_name);
}

std::string_view TransportCatalogue::GetBusName(const Route route)
{
	return std::string_view(route.route_name);
}


// Возвращает указатель на остановку по ее имени из словаря остановок
StopPtr TransportCatalogue::GetStopByName(const std::string_view stop_name) const
{
	if (all_stops_map_.count(stop_name) == 0)
	{
		// Таких остановок в базе нет
		return nullptr;
	}
	else
	{
		return all_stops_map_.at(stop_name);
	}
}


// Возвращает указатель на маршрут по его имени из словаря маршрутов
RoutePtr TransportCatalogue::GetRouteByName(const std::string_view bus_name) const
{
	if (all_buses_map_.count(bus_name) == 0)
	{
		// Таких остановок в базе нет
		return nullptr;
	}
	else
	{
		return all_buses_map_.at(bus_name);
	}
}


// Возвращает указатель на результат запроса о маршруте
RouteStatPtr TransportCatalogue::GetRouteInfo(const std::string_view route_name) const
{
	RoutePtr ptr = GetRouteByName(route_name);

	if (ptr == nullptr)
	{
		// Маршрут с таким именем НЕ существует
		return nullptr;
	}

	// Маршрут с таким именем существует
	return new RouteStat(ptr->stops.size(),
						 ptr->unique_stops_qty,
						 ptr->meters_route_length,
						 ptr->curvature,
						 ptr->route_name);
}


// Возвращает указатель на результат запроса об автобусах для останоки
StopStatPtr TransportCatalogue::GetBusesForStopInfo(const std::string_view stop_name) const
{
	StopPtr ptr = GetStopByName(stop_name);

	if (ptr == nullptr)
	{
		// Такая остановка не найдена.
		return nullptr;
	}

	// TODO Улучшить метод, добавив обратный словарь stops_to_buses_map_ (если потребуется)

	std::set<std::string_view> found_buses_sv;   // временное пустое множество найденных результатов (лексикографическая сортировка)
	for (const auto& bus : all_buses_map_)
	{
		// Ищем в векторе остановок для текущего элемента цикла bus хоть 1 совпадение с заданной остановкой
		auto tmp = std::find_if(bus.second->stops.begin(), bus.second->stops.end(),
								[stop_name](StopPtr curr_stop)
								{
									return (curr_stop->name == stop_name);
								});
		if (tmp != bus.second->stops.end())
		{
			// Что-то найдено. Добавляем в вывод
			found_buses_sv.insert(bus.second->route_name);
		}
	}

	// Возвращаем результат, даже если множество пустое, это валидный результат
	return new StopStat(stop_name, found_buses_sv);
}


void TransportCatalogue::GetAllRoutes(std::map<const std::string, RendererData>& all_routes) const
{

	for (const auto& route : all_buses_data_)
	{
		if (route.stops.size() > 0)
		{
			// Только для непустых маршрутов (пустые не участвуют в нормализации координат)
			RendererData item;
			for (StopPtr stop : route.stops)
			{
				item.stop_coords.push_back(stop->coords);
				item.stop_names.push_back(stop->name);
			}
			item.is_circular = route.is_circular;

			all_routes.emplace(make_pair(route.route_name, item));
		}
	}

	return;
}


size_t TransportCatalogue::GetAllStopsCount() const
{
	return all_stops_data_.size();
}


const std::vector<StopPtr> TransportCatalogue::GetAllStopsPtr() const
{
	std::vector<StopPtr> stop_ptrs;
	for (const auto& [stop_name, stop_ptr] : all_stops_map_)
	{
		stop_ptrs.push_back(stop_ptr);
	}
	return stop_ptrs;
}


const std::deque<RoutePtr> TransportCatalogue::GetAllRoutesPtr() const
{
	std::deque<RoutePtr> route_ptrs;
	for (const auto& route : all_buses_data_)
	{
		route_ptrs.emplace_back(&route);
	}
	std::sort(route_ptrs.begin(), route_ptrs.end(), [](RoutePtr lhs, RoutePtr rhs)
			  {
				  return lhs->route_name <= rhs->route_name;
			  });
	return route_ptrs;
}

}
