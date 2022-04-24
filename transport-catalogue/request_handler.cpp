/*
 * «десь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * ≈сли вы затрудн€етесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

 // ‘ункции по обработке string_view
namespace detail
{

std::pair<std::string_view, std::string_view> Split(std::string_view line, char by, int count)
{
	size_t pos = line.find(by);
	for (int i = 1; (i < count) && (pos != line.npos); ++i)
	{
		pos = line.find(by, pos + 1);
	}
	// Substr может принимать на вход любые значени€ сдвига, включа€ npos (возврат всей строки)
	std::string_view left = line.substr(0, pos);

	// ≈сли символ-разделитель был найден...
	if (pos < line.size() && pos + 1 < line.size())
	{
		// ...возвращаем подстроки без разделител€
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
	// ѕока строка не пуста и первый символ не пробел...
	while (!line.empty() && std::isspace(line[0]))
	{
		// ...удал€ем первый символ
		line.remove_prefix(1);
	}
	return line;
}

std::string_view Rstrip(std::string_view line)
{
	// ƒл€ ускорени€ запомним длину строки в локальной переменной
	size_t line_size = line.size();
	// ѕока строка не пуста и последний символ не пробел...
	while (!line.empty() && std::isspace(line[line_size - 1]))
	{
		// ...удал€ем последний символ
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
	// ѕолучаем данные обо всех маршрутах (через std::move)
	tc_.GetAllRoutes(all_routes);
	// ѕередаем ссылку на словарь в рендерер
	return mr_.RenderMap(all_routes);
}

}
