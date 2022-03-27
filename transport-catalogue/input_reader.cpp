#include "input_reader.h"

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::input_reader
{

void ProcessInput(TransportCatalogue& tc, std::istream& is)
{

	std::vector<InputQuery> queries;

	// Буфер для хранения текущей прочитанной строки
	std::string line;
	std::getline(is, line);

	/*
	// Эмуляция ввода
	std::cout << line << std::endl;
	*/

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

	// Сначала обрабатываем запросы на добавление остановок
	std::vector<InputQuery> delayed;
	for (auto element : queries)
	{
		if (element.type == InputQueryType::AddStop)
		{
			tc.ProcessInputQuery(element);
		}
		else
		{
			delayed.push_back(std::move(element));
		}
	}

	// Обрабатываем запросы на добавление расстояний
	for (auto element : delayed)
	{
		if (element.type == InputQueryType::AddStopsDistance)
		{
			tc.ProcessInputQuery(element);
		}
	}

	// Обрабатываем все остальное
	for (auto element : delayed)
	{
		tc.ProcessInputQuery(element);
	}
}

}
