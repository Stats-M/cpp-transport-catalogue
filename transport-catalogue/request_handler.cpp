/*
 * Назначение модуля: обработчик запросов к базе, содержащего логику,
 * которую не хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 *    request_handler - интерфейс("Фасад) транспортного справочника (двоичные данные)
 *        json_reader - интерфейс работы с данными формата json + добавление данных
 */

#include "request_handler.h"

 // Функции по обработке string_view
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

const std::optional<RouteStatPtr> RequestHandler::GetRouteInfo(const std::string_view& bus_name) const
{
	return tc_.GetRouteInfo(bus_name);
}

const std::optional<StopStatPtr> RequestHandler::GetBusesForStop(const std::string_view& stop_name) const
{
	return tc_.GetBusesForStopInfo(stop_name);
}

svg::Document RequestHandler::GetMapRender() const
{
	std::map<const std::string, transport_catalogue::RendererData> all_routes;
	// Получаем данные обо всех маршрутах (через std::move)
	tc_.GetAllRoutes(all_routes);
	// Передаем ссылку на словарь в рендерер
	return mr_.RenderMap(all_routes);
}

}
