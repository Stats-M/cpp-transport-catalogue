#include "transport_catalogue.h"


namespace transport_catalogue
{

TransportCatalogue::TransportCatalogue()
{}

TransportCatalogue::~TransportCatalogue()
{}

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

void TransportCatalogue::AddRoute(Route&& route)
{
	// Отсутствует ли этот маршрут (автобус) route в базе?
	if (all_buses_map_.count(route.bus_number) == 0)
	{
		// Такого атобуса (он же маршрут) в базе нет, добавлям

		/* Проверка отключена: в route передается vector<const Stop*>, то что они не 
		* ==nullptr пока будет проверять вызывающая функция, т.к. быстрее не добавить
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
		all_buses_map_.insert({ std::string_view(ref.bus_number), &ref });

		// 4. Подсчитываем уникальные остановки на маршруте
		// Копируем вектор указателей
		std::vector<const Stop*> tmp = ref.stops;
		std::sort(tmp.begin(), tmp.end());
		auto last = std::unique(tmp.begin(), tmp.end());
		// Сохраняем количество уникальных остановок на маршруте
		ref.unique_stops_qty = (last != tmp.end() ? std::distance(tmp.begin(), last) : tmp.size());

		// 5. Подсчитываем длину маршрута
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

		/* СТАРЫЙ РАСЧЕТ ДИСТАНЦИИ (ТОЛЬКО ПО КООРДИНАТАМ)
		int stops_num = static_cast<int>(ref.stops.size());
		if (stops_num > 1)
		{
			ref.geo_route_length = 0L;
			for (int i = 0; i < stops_num - 1; ++i)
			{
				ref.geo_route_length += ComputeDistance(ref.stops[i]->coords, ref.stops[i + 1]->coords);
			}
		}
		else
		{
			// У маршрута 0 или 1 остановка. Длина == 0
			ref.geo_route_length = 0L;
		}
		*/
	}
}

void TransportCatalogue::AddDistance(const Stop* stop_from, const Stop* stop_to, size_t dist)
{
	if (stop_from != nullptr && stop_to != nullptr)
	{
		// Вносим запись в словарь расстояний
		distances_map_.insert({ { stop_from, stop_to }, dist });
	}
}

size_t TransportCatalogue::GetDistance(const Stop* stop_from, const Stop* stop_to)
{
	size_t result = GetDistanceDirectly(stop_from, stop_to);
	// Если прямого расстояния в словаре нет, возвращаем обратное расстояние, даже если оно 0
	return (result > 0 ? result : GetDistanceDirectly(stop_to, stop_from));
}

size_t TransportCatalogue::GetDistanceDirectly(const Stop* stop_from, const Stop* stop_to)
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


std::string_view TransportCatalogue::GetStopName(const Stop* stop_ptr)
{
	return std::string_view(stop_ptr->name);
}

std::string_view TransportCatalogue::GetStopName(const Stop stop)
{
	return std::string_view(stop.name);
}

std::string_view TransportCatalogue::GetBusName(const Route* route_ptr)
{
	return std::string_view(route_ptr->bus_number);
}

std::string_view TransportCatalogue::GetBusName(const Route route)
{
	return std::string_view(route.bus_number);
}


const Stop* TransportCatalogue::GetStopByName(std::string_view stop_name)
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

Route* TransportCatalogue::GetRouteByName(std::string_view bus_name)
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


RequestResult TransportCatalogue::GetRouteInfo(std::string_view bus_name)
{
	RequestResult result;
	result.r_ptr = GetRouteByName(bus_name);
	
	if (result.r_ptr != nullptr)
	{
		// Маршрут с таким именем существует
		result.code = RequestResultType::Ok;
	}
	else
	{
		// Маршрут с таким именем НЕ существует
		result.code = RequestResultType::RouteNotExists;
	}

	return result;
}


RequestResult TransportCatalogue::GetBusesForStop(std::string_view stop_name)
{
	RequestResult result;
	result.s_ptr = GetStopByName(stop_name);

	if (result.s_ptr != nullptr)
	{
		// TODO Улучшить метод, добавив обратный словарь stops_to_buses_map_
		std::vector<std::string_view> found_buses_sv;   // временный вектор найденных результатов
		for (auto& bus : all_buses_map_)
		{
			// Ищем в векторе остановок для текущего элемента цикла bus хоть 1 совпадение с заданной остановкой
			auto tmp = std::find_if(bus.second->stops.begin(), bus.second->stops.end(),
									[stop_name](const Stop* curr_stop)
									{
										return (curr_stop->name == stop_name);
									});
			if (tmp != bus.second->stops.end())
			{
				// Что-то найдено. Добавляем в вывод
				found_buses_sv.push_back(bus.second->bus_number);
			}
		}

		// Нашли хоть что-нибудь?
		if (found_buses_sv.size() > 0)
		{
			// Да, есть найденные маршруты. Формируем ответ
			result.code = RequestResultType::Ok;
			std::sort(found_buses_sv.begin(), found_buses_sv.end());
			for (auto& element : found_buses_sv)
			{
				// Переводим в строки
				result.vector_str.emplace_back(std::string(element));
			}
		}
		else
		{
			// Для данной остановки ни одного маршрута не зарегистрировано.
			result.code = RequestResultType::NoBuses;
		}
	}
	else
	{
		// Такая остановка не найдена.
		result.code = RequestResultType::StopNotExists;
	}

	return result;
}
}
