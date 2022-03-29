#include "input_reader.h"

// напишите решение с нуля
// код сохраните в свой git-репозиторий

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


namespace transport_catalogue::input_reader
{

void ProcessInput(TransportCatalogue& tc, std::istream& is)
{
	// Вектор запросов. Порядок выполнения зависит от категории запроса
	std::vector<InputQuery> queries;

	// Буфер для хранения текущей прочитанной строки
	std::string line;
	std::getline(is, line);

	// Первая строка - количество запросов на добавление данных
	size_t request_num = static_cast<size_t>(std::stoul(line));

	for (size_t i = 0; i < request_num; ++i)
	{
		using namespace std::literals;

		std::getline(is, line, '\n');
		// Формат строки: "Stop Tolstopaltsevo: 55.611087, 37.208290"
		// Формат строки: "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"
		// Формат строки: "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya"

		// Отделяем команду от ее параметров
		auto tmp = detail::Split(line, ' ');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Если tmp.second пуст, то это некорректный ввод, пропускаем
		// TODO в будущем могут появиться команды без параметров
		if (tmp.second.empty())
		{
			continue;
		}

		InputQuery query;
		if (tmp.first == "Stop"sv)
		{
			// Формат строки: "Stop Tolstopaltsevo: 55.611087, 37.208290"
			// Формат строки: "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya"

			query.type = InputQueryType::AddStop;
			// Здесь возможны 2 варианта для tmp.second: либо только координаты, либо координаты + список расстояний

			// Ищем вторую запятую
			tmp = detail::Split(tmp.second, ',', 2);
			// Чистим пробелы
			tmp.first = detail::Rstrip(tmp.first);
			tmp.second = detail::Lstrip(tmp.second);
			if (tmp.second.size() != 0)
			{
				// Есть список расстояний. Разбиваем запрос на 2 отдельных
				//tmp.first содержит "Tolstopaltsevo: 55.611087, 37.208290"
				//tmp.second содержит " 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya"

				// Для корректного сохранения расстояний нужно сохранить наименование остановки для второго запроса.
				auto tmp_stop_name = detail::Split(tmp.first, ':');
				tmp_stop_name.first = detail::TrimString(tmp_stop_name.first);

				// Сначала запрос с координатами
				query.query = std::string(tmp.first);
				queries.push_back(std::move(query));
				// Теперь второй с расстоянием
				query = {};
				query.type = InputQueryType::AddStopsDistance;
				query.query = std::string(tmp_stop_name.first) + ":"s + std::string(tmp.second);
				queries.push_back(std::move(query));
			}
			else
			{
				// В запросе только координаты
				//tmp.first содержит "Tolstopaltsevo: 55.611087, 37.208290"
				//tmp.second содержит ""
				query.query = std::string(tmp.first);
				queries.push_back(std::move(query));
			}
		}
		else if (tmp.first == "Bus"sv)
		{
			// Формат строки: "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"

			query.type = InputQueryType::AddRoute;
			query.query = std::string(tmp.second);
			queries.push_back(std::move(query));
		}
		else
		{
			query.type = InputQueryType::NoOp;
			// Запоминаем текст запроса, даже если его тип NoOp
			query.query = std::string(tmp.second);
			queries.push_back(std::move(query));
		}
	}

	// Разбор ввода и разделение на запросы завершены. Обрабатываем запросы
	ProcessInputQueries(tc, queries);
}

void ProcessInputQueries(TransportCatalogue& tc, std::vector<InputQuery>& queries)
{
	std::vector<InputQuery> delayed;

	// Сначала обрабатываем запросы на добавление остановок
	for (auto& element : queries)
	{
		if (element.type == InputQueryType::AddStop)
		{
			tc.AddStop(ProcessQueryAddStop(element.query));
		}
		else
		{
			delayed.push_back(std::move(element));
		}
	}

	// Т.к. запросы выполняются сразу, то строки и их sv больше не нужны, безопасно делать swap
	queries.swap(delayed);
	delayed.clear();

	// Обрабатываем запросы на добавление расстояний
	for (auto& element : queries)
	{
		if (element.type == InputQueryType::AddStopsDistance)
		{
			// Сложная обработка, для упрощения передаем tc внутрь функции
			ProcessQueryAddStopsDistance(tc, element.query);
		}
		else
		{
			delayed.push_back(std::move(element));
		}
	}

	// Обрабатываем все остальное
	for (auto& element : delayed)
	{
		if (element.type == InputQueryType::AddRoute)
		{
			// Сложная обработка, для упрощения передаем tc внутрь функции
			tc.AddRoute(std::move(ProcessQueryAddRoute(tc, element.query)));
		}

		// Остальные запросы, например, InputQueryType::NoOp, игнорируем
	}
}


Stop ProcessQueryAddStop(std::string& query)
{
	// Формат входной строки "Rasskazovka: 55.632761, 37.333324"
	Stop new_stop;

	// Отделяем название от блока координат
	auto tmp = detail::Split(query, ':');
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

	// Возвращаем подготовленную структуру
	return new_stop;
}


void ProcessQueryAddStopsDistance(TransportCatalogue& tc, std::string& query)
{
	// Формат входной строки "Tolstopaltsevo: 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya"

	// Отделяем название от блока расстояний
	auto tmp = detail::Split(query, ':');
	// Чистим пробелы
	tmp.first = detail::TrimString(tmp.first);
	tmp.second = detail::TrimString(tmp.second);

	// Получаем указатель на остановку для которой сохраняем расстояния
	const Stop* stop_from = tc.GetStopByName(tmp.first);
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
		stop_to = tc.GetStopByName(tmp.first);
		if (stop_to == nullptr)
		{
			// Остановка не существует. Выходим.
			return;
		}

		// Вносим запись в словарь расстояний
		tc.AddDistance(stop_from, stop_to, dist);
	}
}


Route ProcessQueryAddRoute(TransportCatalogue& tc, std::string& query)
{
	// Формат входной строки "750: Tolstopaltsevo - Marushkino - Rasskazovka"
	// Формат входной строки "256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"

	Route new_route;

	// Отделяем название маршрута от блока остановок
	auto tmp = detail::Split(query, ':');
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

	// Переводим string_view с названиями в указатели на остановки из базы и запоминаем
	for (auto& element : stops_list)
	{
		// Получаем либо указатель, либо nullptr
		const Stop* tmp_ptr = tc.GetStopByName(element);
		if (tmp_ptr != nullptr)
		{
			// Добавляем ТОЛЬКО существующие остановки (в данной версии)
			new_route.stops.push_back(tmp_ptr);
		}
	}

	// Возвращаем подготовленную структуру
	return new_route;
}

}
