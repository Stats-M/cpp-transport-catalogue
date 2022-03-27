#include "stat_reader.h"

// напишите решение с нуля
// код сохраните в свой git-репозиторий

namespace transport_catalogue::stat_reader
{


void ProcessRequest(TransportCatalogue& tc, std::istream& is)
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


    // Отделяем команду от ее параметров
		auto tmp = detail::Split(line, ' ');
		// Чистим пробелы
		tmp.first = detail::TrimString(tmp.first);
		tmp.second = detail::TrimString(tmp.second);

		// Если tmp.second пуст, то это некорректный ввод, пропускаем цикл
		// TODO в будущем могут появиться команды без параметров, переделать на map<string, func*>
		if (tmp.second.empty())
		{
			continue;
		}

		using namespace std::literals;
		RequestQuery request_query;

		// Обрабатываем тип запроса
		if (tmp.first == "Bus"sv)
		{
			// Запрос на вывод информации о маршруте по его имени (номеру)
			request_query.type = RequestQueryType::GetRouteByName;
		}
		else if (tmp.first == "Stop"sv)
		{
			// Запрос на вывод информации о маршрутах для заданной остановки
			request_query.type = RequestQueryType::GetBusesForStop;
		}
		else
		{
			request_query.type = RequestQueryType::NoOp;
		}

		// В зависимости от команды обрабатываем параметр(ы)
		switch (request_query.type)
		{
		case RequestQueryType::GetRouteByName:
			// Внутри tmp.second хранится имя маршрута, оно может включать пробелы
			// Проверяем, корректно ли отделили номер маршруте
			if (tmp.second.size() != 0)
			{
				// Есть какой-то номер. Формируем запрос
				request_query.query = std::string(tmp.second);
				tc.GetRouteInfo(request_query.query, request_query.reply);
				// Выводм информацию
				std::cout << request_query.reply << std::endl;
			}

			break;

		case RequestQueryType::GetBusesForStop:
			// Внутри tmp.second хранится имя остановки, оно может включать пробелы
			// Проверяем, корректно ли отделили остановку
			if (tmp.second.size() != 0)
			{
				// Есть какой-то номер. Формируем запрос
				request_query.query = std::string(tmp.second);
				tc.GetBusesForStop(request_query.query, request_query.reply);
				// Выводм информацию
				std::cout << request_query.reply << std::endl;
			}

			break;

		case RequestQueryType::NoOp:
			break;
		}
	}
}

}
