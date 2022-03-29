#include "stat_reader.h"

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::stat_reader
{

std::ostream& operator<<(std::ostream& os, const Stop* stop)
{
	using namespace std::string_literals;
	os << "Stop "s << stop->name << ": "s;
	os << std::fixed << std::setprecision(6);
	os << stop->coords.lat << "s, ";
	os << stop->coords.lng;
	// Не переводим строку. Форматирование - зона ответственности вызывающей функции
	return os;
}


std::ostream& operator<<(std::ostream& os, const Route* route)
{
	using namespace std::string_literals;
	// Какой-либо вывод только при ненулевом указателе.
	// Другие ситуации и вывод для них обрабатываются вызывающей функцией, т.к.
	// у оператора недостаточно данных для формирования ответа в этом случае
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


std::ostream& ProcessRequests(std::ostream& os, TransportCatalogue& tc, std::istream& is)
{
	// Буфер для хранения текущей прочитанной строки
	std::string line;
	std::getline(is, line);

	// Первая строка - количество запросов на добавление данных
	size_t request_num = static_cast<size_t>(std::stoul(line));

	for (size_t i = 0; i < request_num; ++i)
	{
		std::getline(is, line, '\n');
		// Формат строки: "Bus 256"
		// Формат строки: "Stop Samara"

		// Отделяем команду от ее параметров
		auto tmp = detail::Split(line, ' ');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Если tmp.second пуст, то это некорректный ввод, пропускаем цикл
		// TODO в будущем могут появиться команды без параметров, перенести проверку после switch
		if (tmp.second.empty())
		{
			continue;
		}

		using namespace std::literals;
		RequestQuery query;

		// Обрабатываем тип запроса
		if (tmp.first == "Bus"sv)
		{
			// Запрос на вывод информации о маршруте по его имени (номеру)
			query.type = RequestQueryType::GetRouteByName;
		}
		else if (tmp.first == "Stop"sv)
		{
			// Запрос на вывод информации о маршрутах для заданной остановки
			query.type = RequestQueryType::GetBusesForStop;
		}
		else
		{
			query.type = RequestQueryType::NoOp;
		}

		// Запоминаем параметр(ы) команды
		query.params = tmp.second;

		ExecuteRequest(std::cout, tc, query);
	}

	return os;
}


std::ostream& ExecuteRequest(std::ostream& os, TransportCatalogue& tc, RequestQuery& query)
{
	using namespace std::literals;
	
	// В зависимости от команды выполняем запросы
	switch (query.type)
	{
	case RequestQueryType::GetRouteByName:
	{
		// Формируем запрос
		RequestResult result = tc.GetRouteInfo(query.params);
		// Выводим информацию
		if (result.code == RequestResultType::Ok)
		{
			// Маршрут найден.
			// Оператор << обрабатывает случаи когда у маршрута есть остановки или когда их 0 (r_ptr != nullptr)
			std::stringstream ss;
			ss << result.r_ptr;
			os << ss.str() << std::endl;
		}
		else
		{
			// Маршрут не найден
			// Формируем ответ вручную, т.к. в result нет данных о номере маршрута
			// Образец  "Bus 751: not found"
			//std::cout << "Bus "s + std::string(query.params) + ": not found"s << std::endl;
			std::stringstream ss;
			ss << "Bus "s << std::string(query.params) << ": not found"s;
			os << ss.str() << std::endl;
		}
	}
		break;

	case RequestQueryType::GetBusesForStop:
	{
		// Формируем запрос
		RequestResult result = tc.GetBusesForStop(query.params);
		// Выводим информацию в зависимости от кода выполнения запроса
		switch (result.code)
		{
		case RequestResultType::Ok:
		{
			// Остановки найдены. Формируем вывод
			// Формат вывода: "Stop X: buses bus1 bus2 ... busN" (список отсортирован)
			std::stringstream ss;
			for (auto& element : result.vector_str)
			{
				ss << " "s << element;
			}
			std::cout << "Stop "s + std::string(query.params) + ": buses"s + ss.str() << std::endl;
		}
			break;
		case RequestResultType::NoBuses:
			// Для данной остановки ни одного маршрута не зарегистрировано. Формируем ответ
			// Формат вывода: "Stop X: no buses"
			os << "Stop "s + std::string(query.params) + ": no buses"s << std::endl;
			break;
		case RequestResultType::StopNotExists:
			// Такая остановка не найдена.
		    // Формат сообщения: "Stop X: not found"
			os << "Stop "s + std::string(query.params) + ": not found"s << std::endl;
			break;
		}
	}
		break;

	case RequestQueryType::NoOp:
		break;
	}

	return os;
}

}
