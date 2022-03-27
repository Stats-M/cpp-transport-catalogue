#include "transport_catalogue.h"

// В будущих проектах после снятия ограничений перенести в отдельный файл типа utility.h/.cpp
namespace detail
{
std::pair<std::string_view, std::string_view> Split(std::string_view line, char by, int count)
{
	size_t pos = line.find(by);
	for (int i = 1; (i < count) && (pos != line.npos); ++i)
	{
		pos = line.find(by, pos + 1);
	}
	// Substr может принимать на вход любые значения сдвига, включая npos (возврат всей строки)
	std::string_view left = line.substr(0, pos);

	// Если символ-разделитель был найден...
	if (pos < line.size() && pos + 1 < line.size())
	{
		// ...возвращаем подстроки без разделителя
		return { left, line.substr(pos + 1) };
	}
	else
	{
		// ...иначе все возвращаем в первой строке и пустую вторую
		return { left, std::string_view() };
	}
}

std::string_view Lstrip(std::string_view line)
{
	// Пока строка не пуста и первый символ не пробел...
	while (!line.empty() && std::isspace(line[0]))
	{
		// ...удаляем первый символ
		line.remove_prefix(1);
	}
	return line;
}

std::string_view Rstrip(std::string_view line)
{
	// Для ускорения запомним длину строки в локальной переменной
	size_t line_size = line.size();
	// Пока строка не пуста и последний символ не пробел...
	while (!line.empty() && std::isspace(line[line_size - 1]))
	{
		// ...удаляем последний символ
		line.remove_suffix(1);
		--line_size;
	}
	return line;
}

std::string_view TrimString(std::string_view line)
{
	return Rstrip(Lstrip(line));
}
}


namespace transport_catalogue
{


std::ostream& operator<<(std::ostream& os, const Stop& stop)
{
	using namespace std::string_literals;
	os << "Stop "s << stop.name << ": "s;
	os << std::fixed << std::setprecision(6);
	os << stop.coords.lat << "s, ";
	os << stop.coords.lng << std::endl;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Route* route)
{
	using namespace std::string_literals;
	// Какой-либо вывод только при ненулевом указателе.
	// Другие ситуации и вывод для них обрабатываются вызывающей функцией
	if (route != nullptr)
	{
		os << "Bus "s << route->bus_number << ": "s;
		if (route->stops.size())
		{
			// Есть остановки на маршруте, выводим их
			os << route->stops.size() << " stops on route, "s;
			os << route->unique_stops_qty << " unique stops, "s;
			os << route->meters_route_length << " route length, "s;
			os << std::setprecision(6);
			os << route->curvature << " curvature"s;
			// Не переводим строку. Форматирование - зона ответственности вызывающей функции

			/* ВЫВОД ДЛЯ ВЕРСИЙ 1 и 2
			os << route->stops.size() << " stops on route, "s;
			os << route->unique_stops_qty << " unique stops, "s;
			//os << std::fixed << std::setprecision(6);
			os << std::setprecision(6);
			os << route->geo_route_length << " route length"s;
			// Не переводим строку. Форматирование - зона ответственности вызывающей функции
			*/
		}
		else
		{
			// Остановок нет
			os << "no stops"s;
			// Не переводим строку. Форматирование - зона ответственности вызывающей функции
		}
	}
	return os;
}

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
	if (all_buses_map_.count(GetBusName(&route)) == 0)
	{
		// Таких атобусов (они же маршруты) в базе нет, добавлям

		// 1. Добавляем автобус в дек-хранилище, перемещая route
		auto& ref = all_buses_data_.emplace_back(std::move(route));
		// 2. Добавляем автобус в словарь маршрутов
		all_buses_map_.insert({ std::string_view(ref.bus_number), &ref });

		// Подсчитываем уникальные остановки на маршруте
		// Копируем вектор указателей
		std::vector<const Stop*> tmp = ref.stops;
		std::sort(tmp.begin(), tmp.end());
		auto last = std::unique(tmp.begin(), tmp.end());
		// Сохраняем количество уникальных остановок на маршруте
		ref.unique_stops_qty = (last != tmp.end() ? std::distance(tmp.begin(), last) : tmp.size());

		// Подсчитываем длину маршрута
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

void TransportCatalogue::ProcessInputQuery(InputQuery& input_query)
{
	// В зависимости от типа запроса обрабатываем строку
	switch (input_query.type)
	{
	case InputQueryType::AddStop:
	{
		// if a declaration statement is encountered inside the statement, it has to be scoped in its own compound statement

		// Формат входной строки "Rasskazovka: 55.632761, 37.333324"
		Stop new_stop;

		// Отделяем название от блока координат
		auto tmp = detail::Split(input_query.query, ':');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Сохраняем string с названием
		new_stop.name = std::string(tmp.first);

		// Разделяем координаты
		tmp = detail::Split(tmp.second, ' ');
		// Чистим пробелы с учетом ранее очищенных боков общей строки
		tmp.first = detail::Rstrip(tmp.first);
		tmp.second = detail::Lstrip(tmp.second);

		// Конвертируем и сохраняем координаты
		new_stop.coords.lat = std::stod(std::string(tmp.first));
		new_stop.coords.lng = std::stod(std::string(tmp.second));

		// Сохраняем остановку в транспортном каталоге
		AddStop(std::move(new_stop));
		break;

	}
	case InputQueryType::AddRoute:
	{
		// if a declaration statement is encountered inside the statement, it has to be scoped in its own compound statement

		// Формат входной строки "750: Tolstopaltsevo - Marushkino - Rasskazovka"
		// Формат входной строки "256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"

		Route new_route;

		// Отделяем название маршрута от блока остановок
		auto tmp = detail::Split(input_query.query, ':');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Сохраняем string с названием
		new_route.bus_number = std::string(tmp.first);

		// Если в строке нет разделителя > (кольцевой маршрут), то устанавливаем по-умолчанию разделитель '-'
		char delim = (tmp.second.find('>') == tmp.second.npos ? '-' : '>');

		std::vector<std::string_view> stops_list;

		// Пока tmp.second не пусто (пробелы на конце удалены ранее и тут либо данные, либо ничего нет)
		while (tmp.second.size() != 0)
		{
			// нарезаем строку на остановки

			// Отделяем очерезную остановку в tmp.first
			tmp = detail::Split(tmp.second, delim);
			// Чистим пробелы с учетом ранее очищенных боков общей строки
			tmp.first = detail::Rstrip(tmp.first);
			tmp.second = detail::Lstrip(tmp.second);

			// Запоминаем название
			stops_list.push_back(tmp.first);
		}

		if ((delim == '-') && (stops_list.size() > 1))
		{
			// Достраиваем обратный маршрут для некольцевого маршрута
			for (int i = stops_list.size() - 2; i >= 0; --i)
			{
				// Добавляем в конец вектора остановки в обратном направлении, кроме конечной stops_list[size() - 1]
				stops_list.push_back(stops_list[i]);
			}
		}

		// Запоминаем в структуре маршрута остановки
		for (auto element : stops_list)
		{
			// Проверка существования остановки
			if (all_stops_map_.count(element) > 0)
			{
				// Такая остановка существует. Получаем указатель на структуру с данными об остановке
				new_route.stops.push_back(all_stops_map_.at(element));
			}
			// Не существующие остановки пока НЕ добавляем (в данной версии)
		}

		// Добавляем остановку в транспортный каталог
		AddRoute(std::move(new_route));

	}
	break;

	case InputQueryType::AddStopsDistance:
	{
		// Формат входной строки "Tolstopaltsevo: 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya"

		// Отделяем название от блока расстояний
		auto tmp = detail::Split(input_query.query, ':');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Получаем указатель на остановку для которой сохраняем расстояния
		const Stop* stop_from = GetStopByName(tmp.first);
		if (stop_from == nullptr)
		{
			// Остановка не существует. Выходим.
			return;
		}

		size_t dist = 0U;
		const Stop* stop_to = nullptr;

		while (tmp.second.size() != 0)
		{
			// Выделяем расстояние
			tmp = detail::Split(tmp.second, 'm');
			tmp.first = detail::TrimString(tmp.first);
			tmp.second = detail::Lstrip(tmp.second);
			dist = std::stoul(std::string(tmp.first));

			// Выделяем название второй остановки
			tmp = detail::Split(tmp.second, ' ');
			tmp = detail::Split(tmp.second, ',');
			tmp.first = detail::TrimString(tmp.first);
			tmp.second = detail::Lstrip(tmp.second);
			stop_to = GetStopByName(tmp.first);
			if (stop_to == nullptr)
			{
				// Остановка не существует. Выходим.
				return;
			}

			// Вносим запись в словарь расстояний
			AddDistance(stop_from, stop_to, dist);
		}
	}
	break;

	case InputQueryType::NoOp:
		break;

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


void TransportCatalogue::GetRouteInfo(std::string_view bus_name, std::string& result)
{
	std::stringstream ss;
	Route* r_ptr = GetRouteByName(bus_name);
	if (r_ptr != nullptr)
	{
		ss << r_ptr;
		result = ss.str();
		// Не переводим строку. Форматирование - зона ответственности вызывающей функции
	}
	else
	{
		using namespace std::string_literals;

		//Образец  "Bus 751: not found"
		result = "Bus "s + std::string(bus_name) + ": not found"s;
		// Не переводим строку. Форматирование - зона ответственности вызывающей функции
	}
}

void TransportCatalogue::GetBusesForStop(std::string_view stop_name, std::string& result)
{
	const Stop* s_ptr = GetStopByName(stop_name);
	if (s_ptr != nullptr)
	{
		// TODO Улучшить метод, добавив обратный словарь stops_to_buses_map_
		std::vector<std::string_view> found_buses_sv;   // вектор найденных результатов
		for (auto bus : all_buses_map_)
		{
			// Ищем в векторе остановок для текущего маршрута цикла bus хоть 1 совпадение с заданной остановкой
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
			std::sort(found_buses_sv.begin(), found_buses_sv.end());
			// Формат сообщения: "Stop X: buses bus1 bus2 ... busN" (список отсортирован)
			std::stringstream ss;
			using namespace std::string_literals;
			for (auto element : found_buses_sv)
			{
				ss << " "s << std::string(element);
			}
			result = "Stop "s + std::string(stop_name) + ": buses"s + ss.str();
		}
		else
		{
			// Для данной остановки ни одного маршрута не зарегистрировано. Формируем ответ
			// Формат сообщения: "Stop X: no buses"
			using namespace std::string_literals;
			result = "Stop "s + std::string(stop_name) + ": no buses"s;
		}

	}
	else
	{
		// Такая остановка не найдена.
		// Формат сообщения: "Stop X: not found"
		using namespace std::string_literals;
		result = "Stop "s + std::string(stop_name) + ": not found"s;
	}
}
}
