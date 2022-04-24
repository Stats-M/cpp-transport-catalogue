/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 *
 * Назначение модуля: конвертация в/из json исходных данных, данных запросов и ответов
 * справочника на запросы. Вся работа с данными в формате json происходит в json_reader
 * (а также main.cpp - вывод в потоки), вне json_reader'а данные обрабатываются во
 * внутренних форматах транспортного справочника. Это позволяет добавить обработку данных
 * в другом формате (напримео, XML), не меняя request_handler / transport_catalogue
 *
 *   request_handler - интерфейс ("Фасад) транспортного справочника (двоичные данные)
 *       json_reader - интерфейс работы с данными формата json
 */

#include "json_reader.h"

namespace json_reader
{

// ---------------Generic I/O-------------------------

void ProcessJSON(transport_catalogue::TransportCatalogue& tc, 
				 transport_catalogue::RequestHandler& rh,
				 map_renderer::MapRenderer& mr,
				 std::istream& input, std::ostream& output)
{
	// Временно отключено. Нужно делать static документ или класс чтобы можно было разделить
	//LoadAsJSON(tc, input);     // Загружаем исходные данные в static переменную
	//QueryAsJSON(rh, output);  // Отправляем запросы к справочнику и выводим результат

	using namespace std::literals;

	const json::Document j_doc = json::Load(input);
	// Корневой узел JSON документа - словарь
	const json::Dict j_dict = j_doc.GetRoot().AsMap();
	// Находим точку начала секции входных данных в словаре
	const auto base_requests_it = j_dict.find("base_requests"s);
	if (base_requests_it != j_dict.cend())
	{
		// Есть входные данные. Формат данных - массив (вектор)
		AddToDB(tc, base_requests_it->second.AsArray());
	}

	// Находим точку начала секции настроек рендерера в словаре
	const auto renderer_settings_it = j_dict.find("render_settings"s);
	if (renderer_settings_it != j_dict.cend())
	{
		// Есть секция настроек. Формат данных - словарь
		ReadRendererSettings(mr, renderer_settings_it->second.AsMap());
	}

	// Находим точку начала секции запросов в словаре
	const auto stat_requests_it = j_dict.find("stat_requests"s);
	if (stat_requests_it != j_dict.cend())
	{
		// Есть запросы к справочнику. Формат данных - массив (вектор)
		//QueryAsJSON(stat_requests_it->second.AsArray(), rh, output);
		ProcessQueriesJSON(rh, stat_requests_it->second.AsArray(), output);
	}
}

/* Временно отключено. Нужно делать static документ или менять архитектуру модуля 
*  чтобы можно было разделить обработку наполнения справочника и запросов к нему.
void LoadAsJSON(transport_catalogue::TransportCatalogue& tc, std::istream& input)
{
	const json::Document j_doc = json::Load(input);
	// Корневой узел JSON документа - словарь
	const json::Dict j_dict = j_doc.GetRoot().AsMap();
	// Находим точку начала секции входных данных в словаре
	const auto base_requests_it = j_dict.find("base_requests");
	if (base_requests_it != j_dict.cend())
	{
		// Есть входные данные. Формат данных - массив (вектор)
		AddToDB(tc, base_requests_it->second.AsArray());
	}

	// Находим точку начала секции запросов в словаре
	const auto stat_requests_it = j_dict.find("stat_requests");
	if (stat_requests_it != j_dict.cend())
	{
		// Есть запросы к справочнику. Формат данных - массив (вектор)
		//ProcessAsJSON(stat_requests_it->second.AsArray(), rh, output);
	}

}

void QueryAsJSON(const json::Array& j_arr, transport_catalogue::RequestHandler& rh, std::ostream& output)
{

}
*/

//------------------Process data-------------------

void AddToDB(transport_catalogue::TransportCatalogue& tc, const json::Array& j_arr)
{
	// Каждый элемент массива входных данных j_arr - словарь.

	using namespace std::literals;

	// Проход 1. Заносим данные о названиях и координатах остановок
	for (const auto& element : j_arr)
	{
		// Ищем ключ "type", хранящий тип записи
		const auto request_type = element.AsMap().find("type"s);
		if (request_type != element.AsMap().end())
		{
			if (request_type->second.AsString() == "Stop"s)
			{
				// Это остановка (тип записи - словарь). Обрабатываем название и координаты 
				AddStopData(tc, element.AsMap());
			}
		}
	}

	// Проход 2. Заносим данные о расстояниях между остановками
	for (const auto& element : j_arr)
	{
		// Ищем ключ "type", хранящий тип записи
		const auto request_type = element.AsMap().find("type"s);
		if (request_type != element.AsMap().end())
		{
			if (request_type->second.AsString() == "Stop"s)
			{
				// Это остановка (тип записи - словарь). Обрабатываем расстояния 
				AddStopDistance(tc, element.AsMap());
			}
		}
	}

	// Проход 3. Заносим данные о маршрутах
	for (const auto& element : j_arr)
	{
		// Ищем ключ "type", хранящий тип записи
		const auto request_type = element.AsMap().find("type"s);
		if (request_type != element.AsMap().end())
		{
			if (request_type->second.AsString() == "Bus"s)
			{
				// Это маршрут (тип записи - словарь). Обрабатываем полностью
				AddRouteData(tc, element.AsMap());
			}
		}
	}
}

void AddStopData(transport_catalogue::TransportCatalogue& tc, const json::Dict& j_dict)
{
	using namespace std::literals;

	const std::string stop_name = j_dict.at("name"s).AsString();
	const double latitude = j_dict.at("latitude"s).AsDouble();
	const double longitude = j_dict.at("longitude"s).AsDouble();

	tc.AddStop(transport_catalogue::Stop{ stop_name, latitude, longitude });
}

void AddStopDistance(transport_catalogue::TransportCatalogue& tc, const json::Dict& j_dict)
{
	using namespace std::literals;

	const std::string from_stop_name = j_dict.at("name"s).AsString();
	transport_catalogue::StopPtr from_ptr = tc.GetStopByName(from_stop_name);
	// Продолжаем обработку только если остановка отправления есть в справочнике
	if (from_ptr != nullptr)
	{
		const json::Dict stops = j_dict.at("road_distances"s).AsMap();
		for (const auto& [to_stop_name, distance] : stops)
		{
			tc.AddDistance(from_ptr, tc.GetStopByName(to_stop_name), static_cast<size_t>(distance.AsInt()));
		}
	}
}

void AddRouteData(transport_catalogue::TransportCatalogue& tc, const json::Dict& j_dict)
{
	// Алгоритм:
	// 1. Создаем локальную переменную типа Route
	// 2. Заполняем имя маршрута, тип и указатели на все остановки (nullptr отбрасываем)
	// 3. Передаем Route через move в TC, который производит заполнение расчетных 
	//    полей и перемещает итоговый объект в хранилище.

	using namespace std::literals;

	transport_catalogue::Route new_route;
	new_route.route_name = j_dict.at("name"s).AsString();
	new_route.is_circular = j_dict.at("is_roundtrip"s).AsBool();

	// Формируем вектор указателей на остановки
	for (auto& element : j_dict.at("stops"s).AsArray())
	{
		// Получаем либо указатель, либо nullptr
		transport_catalogue::StopPtr tmp_ptr = tc.GetStopByName(element.AsString());
		if (tmp_ptr != nullptr)
		{
			// Добавляем ТОЛЬКО существующие остановки (в данной версии)
			new_route.stops.push_back(tmp_ptr);
		}
	}
	// Добавляем маршрут. Он может иметь и 0 (ноль) остановок, это валидный случай
	tc.AddRoute(std::move(new_route));
}

//------------------Process settings-------------------

const svg::Color ConvertColor_JSON2SVG(const json::Node& color)
{
	if (color.IsString())
	{
		// Цвет в формате std::string
		return svg::Color{ color.AsString() };
	}
	else if (color.IsArray())
	{
		if (color.AsArray().size() == 3)
		{
			// Цвет в формате RGB, значения цветности 0...255
			return svg::Rgb
			{
				static_cast<uint8_t>(color.AsArray()[0].AsInt()),
				static_cast<uint8_t>(color.AsArray()[1].AsInt()),
				static_cast<uint8_t>(color.AsArray()[2].AsInt())
			};
		}
		else if (color.AsArray().size() == 4)
		{
			// Цвет в формате RGBA, значения цветности 0...255
			return svg::Rgba
			{
				static_cast<uint8_t>(color.AsArray()[0].AsInt()),
				static_cast<uint8_t>(color.AsArray()[1].AsInt()),
				static_cast<uint8_t>(color.AsArray()[2].AsInt()),
				color.AsArray()[3].AsDouble()
			};
		}
	}

	// Возвращаем monostate если никакой вариант не подошел
	return svg::Color();
}


void ReadRendererSettings(map_renderer::MapRenderer& mr, const json::Dict& j_dict)
{
	map_renderer::RendererSettings new_settings;

	new_settings.width = j_dict.at("width").AsDouble();
	new_settings.height = j_dict.at("height").AsDouble();
	new_settings.padding = j_dict.at("padding").AsDouble();
	new_settings.line_width = j_dict.at("line_width").AsDouble();
	new_settings.stop_radius = j_dict.at("stop_radius").AsDouble();
	new_settings.bus_label_font_size = j_dict.at("bus_label_font_size").AsInt();
	new_settings.bus_label_offset = { j_dict.at("bus_label_offset").AsArray()[0].AsDouble(), j_dict.at("bus_label_offset").AsArray()[1].AsDouble() };
	new_settings.stop_label_font_size = j_dict.at("stop_label_font_size").AsInt();
	new_settings.stop_label_offset = { j_dict.at("stop_label_offset").AsArray()[0].AsDouble(), j_dict.at("stop_label_offset").AsArray()[1].AsDouble() };
	new_settings.underlayer_color = ConvertColor_JSON2SVG(j_dict.at("underlayer_color"));
	new_settings.underlayer_width = j_dict.at("underlayer_width").AsDouble();
	new_settings.color_palette.clear();    // Очистим массив от дефолтных элементов
	for (const auto& color : j_dict.at("color_palette").AsArray())
	{
		new_settings.color_palette.emplace_back(ConvertColor_JSON2SVG(color));
	}

	// Применяем новые настройки рендера
	mr.ApplyRenderSettings(new_settings);
}

//--------------Processing requests-------------------

void ProcessQueriesJSON(transport_catalogue::RequestHandler& rh, const json::Array& j_arr, std::ostream& output)
{
	using namespace std::literals;

	json::Array processed_queries;  // Вектор json::Node с результатами запросов

	// Для каждого запроса в j_arr получаем ответ в виде json::Node
	for (const auto& query : j_arr)
	{
		const auto request_type = query.AsMap().find("type"s);
		if (request_type != query.AsMap().cend())
		{
			// Есть поле типа запроса "type", обрабатываем
			if (request_type->second.AsString() == "Stop"s)
			{
				processed_queries.emplace_back(ProcessStopQuery(rh, query.AsMap()));
			}
			else if (request_type->second.AsString() == "Bus"s)
			{
				processed_queries.emplace_back(ProcessRouteQuery(rh, query.AsMap()));
			}
			else if (request_type->second.AsString() == "Map"s)
			{
				processed_queries.emplace_back(ProcessMapQuery(rh, query.AsMap()));
			}
		}
	}
	json::Print(json::Document{ processed_queries }, output);
}

const json::Node ProcessStopQuery(transport_catalogue::RequestHandler& rh, const json::Dict& j_dict)
{
	using namespace std::literals;

	const std::string stop_name = j_dict.at("name"s).AsString();
	const auto stop_query_ptr = rh.GetBusesForStop(stop_name);

	if (stop_query_ptr == nullptr)
	{
		// Такой остановки нет. Генерируем сообщение об ошибке
		return json::Dict{ {"request_id"s, j_dict.at("id"s).AsInt()}, 
						  {"error_message"s, "not found"s}};
	}

	// Получаем отсортированный список маршрутов (может быть и 0 маршрутов)
	json::Array routes;
	for (auto& bus : stop_query_ptr.value()->buses)
	{
		// Конвертируем из sv в string
		routes.push_back(std::string(bus));
	}

	// Формируем JSON ответ
	return json::Dict{ {"buses"s, routes},
					  {"request_id"s, j_dict.at("id"s).AsInt()} };
}

const json::Node ProcessRouteQuery(transport_catalogue::RequestHandler& rh, const json::Dict& j_dict)
{
	using namespace std::literals;
	
	const std::string route_name = j_dict.at("name"s).AsString();
	const auto route_query_ptr = rh.GetRouteInfo(route_name);

	if (route_query_ptr == nullptr)
	{
		// Такого маршрута нет. Генерируем сообщение об ошибке
		return json::Dict{ {"request_id"s, j_dict.at("id"s).AsInt()},
						  {"error_message"s, "not found"s} };
	}

	// Формируем JSON ответ
	return json::Dict{ {"curvature"s, route_query_ptr.value()->curvature},
					  {"request_id"s, j_dict.at("id"s).AsInt()},
					  {"route_length"s, static_cast<int>(route_query_ptr.value()->meters_route_length)},
					  {"stop_count"s, static_cast<int>(route_query_ptr.value()->stops_on_route)},
					  {"unique_stop_count"s, static_cast<int>(route_query_ptr.value()->unique_stops)} };
}


const json::Node ProcessMapQuery(transport_catalogue::RequestHandler& rh, const json::Dict& j_dict)
{
	using namespace std::literals;

	// Запрашиваем SVG документ через RequestHandler
	svg::Document svg_map = rh.GetMapRender();
	// Выводим его в текстовый поток через svg::Document.Render(stream)
	std::ostringstream os_stream;
	svg_map.Render(os_stream);

	// Формируем JSON ответ, получая std:string из содержимого текстового потока
	return json::Dict{ {"map"s, os_stream.str()},
					  {"request_id"s, j_dict.at("id"s).AsInt()} };
}

}  // namespace json_reader 
