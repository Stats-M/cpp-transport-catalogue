/*
 * Назначение модуля: конвертация в/из json исходных данных, данных запросов и ответов
 * справочника на запросы. Вся работа с данными в формате json происходит в json_reader
 * (а также main.cpp - вывод в потоки), вне json_reader'а данные обрабатываются во
 * внутренних форматах транспортного справочника. Это позволяет добавить обработку данных
 * в другом формате (например, XML), не меняя request_handler / transport_catalogue
 *
 *   request_handler - интерфейс ("Фасад") транспортного справочника (двоичные данные)
 *       json_reader - интерфейс работы с данными формата json
 */

#include "json_reader.h"

namespace json_reader
{

/*
enum class ParseMode
{
	UNDEFINED,
	MAKE_BASE,
	PROCESS_REQUESTS,
};
*/

// ---------------Generic I/O-------------------------

void ProcessBaseJSON(transport_catalogue::TransportCatalogue& tc,
				 map_renderer::MapRenderer& mr,
				 std::istream& input)
{
	using namespace std::literals;

	const json::Document j_doc = json::Load(input);
	// Корневой узел JSON документа - словарь
	const json::Dict j_dict = j_doc.GetRoot().AsDict();

	// Находим точку начала секции входных данных в словаре (если есть)
	const auto base_requests_it = j_dict.find("base_requests"s);
	if (base_requests_it != j_dict.cend())
	{
		// Есть входные данные. Формат данных - массив (вектор)
		AddToDB(tc, base_requests_it->second.AsArray());
	}

	// Находим точку начала секции настроек рендерера в словаре (если есть)
	const auto renderer_settings_it = j_dict.find("render_settings"s);
	if (renderer_settings_it != j_dict.cend())
	{
		// Есть секция настроек. Формат данных - словарь
		ReadRendererSettings(mr, renderer_settings_it->second.AsDict());
	}

	// Создаем объект роутера. Создаем здесь, т.к. инициализация графа
	// роутера требует информации о количестве вершин (они же - остановки),
	// информация о которых к этому моменту уже загружена
	router::TransportRouter tr(tc);

	// Находим точку начала секции настроек роутера в словаре (если есть)
	const auto router_settings_it = j_dict.find("routing_settings"s);
	if (router_settings_it != j_dict.cend())
	{
		// Есть секция настроек. Формат данных - словарь
		ReadRouterSettings(tr, router_settings_it->second.AsDict());
	}

	// Проверка секции настроек сериализации.
	const auto serialization_settings_it = j_dict.find("serialization_settings"s);
	if (serialization_settings_it != j_dict.cend())
	{
		// Есть секция настроек сериализации. Формат данных - словарь
		const std::string serialization_filename = ReadSerializationSettings(serialization_settings_it->second.AsDict());
		// Сериализация данных и настроек каталога, рендерера, роутера
		serialization::Serializer serializer(tc, mr, &tr);
		serializer.Serialize(serialization_filename);
	}
}

void ProcessRequestJSON(transport_catalogue::TransportCatalogue& tc,
				 map_renderer::MapRenderer& mr,
				 std::istream& input, std::ostream& output)
{
	using namespace std::literals;

	const json::Document j_doc = json::Load(input);
	// Корневой узел JSON документа - словарь
	const json::Dict j_dict = j_doc.GetRoot().AsDict();

	// Создаем обработчик запросов для справочника
	transport_catalogue::RequestHandler rh(tc, mr);

	// Нельзя создать объект роутера, т.к. справочник пока еще пуст.
	// Создание роутера и обработка запросов будут производиться
	// внутри ветки десериализации

	// Проверка секции настроек сериализации.
	const auto serialization_settings_it = j_dict.find("serialization_settings"s);
	if (serialization_settings_it != j_dict.cend())
	{
		// Есть секция настроек сериализации. Формат данных - словарь
		const std::string serialization_filename = ReadSerializationSettings(serialization_settings_it->second.AsDict());
		
		// Десериализация.
		// Т.к. роутер должен получить готовый справочник, пока передаем nullptr
		serialization::Serializer serializer(tc, mr, nullptr);
		serializer.Deserialize(serialization_filename);

		// Создаем объект роутера на основе уже десериализованного каталога.
		router::TransportRouter tr(tc);

		// Передаем указатель на роутер и завершаем десериализацию
		serializer.DeserializeRouter(&tr);

		// Находим точку начала секции запросов в словаре (если есть)
		const auto stat_requests_it = j_dict.find("stat_requests"s);
		if (stat_requests_it != j_dict.cend())
		{
			// Есть запросы к справочнику. Формат данных - массив (вектор)
			ParseRawJSONQueries(rh, tr, stat_requests_it->second.AsArray(), output);
		}
	}
}

//------------------Process data-------------------

void AddToDB(transport_catalogue::TransportCatalogue& tc, const json::Array& j_arr)
{
	// Каждый элемент массива входных данных j_arr - словарь.

	using namespace std::literals;

	// Вектор значений словаря, обрабатываемых в соответствующем проходе обработки входных данных,
	// т.е. в 0-й проход проверяем на =="Stop", в 1й на =="Stop", во 2й на =="Bus"
	static std::vector<std::string> stages = { "Stop"s, "Stop"s, "Bus"s };

	// Обработка j_arr в несколько проходов в строго определенной последовательности.
	// В каждом проходе обрабатывается своя отдельная группа данных
	for (size_t i = 0; i < stages.size(); ++i)
	{
		for (const auto& element : j_arr)
		{
			// Ищем ключ "type", хранящий тип записи
			const auto request_type = element.AsDict().find("type"s);
			if (request_type != element.AsDict().end())
			{
				// Для каждой пары проверяем соответствие Value i-му значению вектора stages
				// При соответствии вызываем отдельную для каждого прохода функцию обработки
				if (request_type->second.AsString() == stages[i])
				{
					switch (i)
					{
					case 0:
						// Это остановка (тип записи - словарь). Обрабатываем название и координаты 
						AddStopData(tc, element.AsDict());
						break;
					case 1:
						// Это остановка (тип записи - словарь). Обрабатываем расстояния 
						AddStopDistance(tc, element.AsDict());
						break;
					case 2:
						// Это маршрут (тип записи - словарь). Обрабатываем полностью
						AddRouteData(tc, element.AsDict());
						break;
					}
				}
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
		const json::Dict stops = j_dict.at("road_distances"s).AsDict();
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

const svg::Color ConvertJSONColorToSVG(const json::Node& color)
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
	new_settings.underlayer_color = ConvertJSONColorToSVG(j_dict.at("underlayer_color"));
	new_settings.underlayer_width = j_dict.at("underlayer_width").AsDouble();
	new_settings.color_palette.clear();    // Очистим массив от дефолтных элементов
	for (const auto& color : j_dict.at("color_palette").AsArray())
	{
		new_settings.color_palette.emplace_back(ConvertJSONColorToSVG(color));
	}

	// Применяем новые настройки рендера
	mr.ApplyRendererSettings(new_settings);
}

void ReadRouterSettings(router::TransportRouter& tr, const json::Dict& j_dict)
{
	router::RouterSettings new_settings;

	new_settings.bus_velocity = j_dict.at("bus_velocity").AsInt();
	new_settings.bus_wait_time = j_dict.at("bus_wait_time").AsInt();

	// Применяем новые настройки роутера
	tr.ApplyRouterSettings(new_settings);
}

const std::string ReadSerializationSettings(const json::Dict& j_dict)
{
	return j_dict.at("file").AsString();
}


//--------------Processing requests-------------------

void ParseRawJSONQueries(transport_catalogue::RequestHandler& rh, 
						 router::TransportRouter& tr, 
						 const json::Array& j_arr, 
						 std::ostream& output)
{
	using namespace std::literals;

	json::Array processed_queries;  // Вектор json::Node с результатами запросов

	// Для каждого запроса в j_arr получаем ответ в виде json::Node
	for (const auto& query : j_arr)
	{
		const auto request_type = query.AsDict().find("type"s);
		if (request_type != query.AsDict().cend())
		{
			// Есть поле типа запроса "type", обрабатываем
			if (request_type->second.AsString() == "Stop"s)
			{
				processed_queries.emplace_back(ProcessStopQuery(rh, query.AsDict()));
			}
			else if (request_type->second.AsString() == "Bus"s)
			{
				processed_queries.emplace_back(ProcessBusQuery(rh, query.AsDict()));
			}
			else if (request_type->second.AsString() == "Map"s)
			{
				processed_queries.emplace_back(ProcessMapQuery(rh, query.AsDict()));
			}
			else if (request_type->second.AsString() == "Route"s)
			{
				processed_queries.emplace_back(ProcessRouteQuery(tr, query.AsDict()));
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
		/*
		return json::Dict{ {"request_id"s, j_dict.at("id"s).AsInt()},
						  {"error_message"s, "not found"s}};
		*/
		return json::Builder{}
			.StartDict()
			.Key("request_id"s).Value(j_dict.at("id"s).AsInt())
			.Key("error_message"s).Value("not found"s)
			.EndDict()
			.Build();
	}

	// Получаем отсортированный список маршрутов (может быть и 0 маршрутов)
	json::Array routes;
	for (auto& bus : stop_query_ptr.value()->buses)
	{
		// Конвертируем из sv в string
		routes.push_back(std::string(bus));
	}

	// Формируем JSON ответ
	/*
	return json::Dict{ {"buses"s, routes},
					  {"request_id"s, j_dict.at("id"s).AsInt()} };
	*/
	return json::Builder{}
		.StartDict()
		.Key("buses"s).Value(routes)
		.Key("request_id"s).Value(j_dict.at("id"s).AsInt())
		.EndDict()
		.Build();
}

const json::Node ProcessBusQuery(transport_catalogue::RequestHandler& rh, const json::Dict& j_dict)
{
	using namespace std::literals;

	const std::string route_name = j_dict.at("name"s).AsString();
	const auto route_query_ptr = rh.GetRouteInfo(route_name);

	if (route_query_ptr == nullptr)
	{
		// Такого маршрута нет. Генерируем сообщение об ошибке
		/*
		return json::Dict{ {"request_id"s, j_dict.at("id"s).AsInt()},
						  {"error_message"s, "not found"s} };
		*/
		return json::Builder{}
			.StartDict()
			.Key("request_id"s).Value(j_dict.at("id"s).AsInt())
			.Key("error_message"s).Value("not found"s)
			.EndDict()
			.Build();
	}

	// Формируем JSON ответ
	/*
	return json::Dict{ {"curvature"s, route_query_ptr.value()->curvature},
					  {"request_id"s, j_dict.at("id"s).AsInt()},
					  {"route_length"s, static_cast<int>(route_query_ptr.value()->meters_route_length)},
					  {"stop_count"s, static_cast<int>(route_query_ptr.value()->stops_on_route)},
					  {"unique_stop_count"s, static_cast<int>(route_query_ptr.value()->unique_stops)} };
	*/
	return json::Builder{}
		.StartDict()
		.Key("curvature"s).Value(route_query_ptr.value()->curvature)
		.Key("request_id"s).Value(j_dict.at("id"s).AsInt())
		.Key("route_length"s).Value(static_cast<int>(route_query_ptr.value()->meters_route_length))
		.Key("stop_count"s).Value(static_cast<int>(route_query_ptr.value()->stops_on_route))
		.Key("unique_stop_count"s).Value(static_cast<int>(route_query_ptr.value()->unique_stops))
		.EndDict()
		.Build();

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
	/*
	return json::Dict{ {"map"s, os_stream.str()},
					  {"request_id"s, j_dict.at("id"s).AsInt()} };
	*/
	return json::Builder{}
		.StartDict()
		.Key("map"s).Value(os_stream.str())
		.Key("request_id"s).Value(j_dict.at("id"s).AsInt())
		.EndDict()
		.Build();
}


const json::Node ProcessRouteQuery(router::TransportRouter& tr, const json::Dict& j_dict)
{
	using namespace std::literals;

	auto route_data = tr.CalculateRoute(j_dict.at("from").AsString(), j_dict.at("to").AsString());

	// Подходящий маршрут не найден. Генерируем сообщение об ошибке
	if (!route_data.founded)
	{
		return json::Builder{}.StartDict()
			.Key("request_id").Value(j_dict.at("id").AsInt())
			.Key("error_message").Value("not found")
			.EndDict()
			.Build();
	}

	json::Array items;
	// Проходим по элементам маршрута и формируем ответ
	for (const auto& item : route_data.items)
	{
		json::Dict items_map;
		if (item.type == graph::EdgeType::TRAVEL)
		{
			items_map["type"] = "Bus"s;
			items_map["bus"] = item.edge_name;
			items_map["span_count"] = item.span_count;
		}
		else if (item.type == graph::EdgeType::WAIT)
		{
			items_map["type"] = "Wait"s;
			items_map["stop_name"] = item.edge_name;
		}
		items_map["time"] = item.time;
		items.push_back(items_map);
	}
	return json::Builder{}.StartDict()
		.Key("request_id").Value(j_dict.at("id").AsInt())
		.Key("total_time").Value(route_data.total_time)
		.Key("items").Value(items)
		.EndDict()
		.Build();
}



}  // namespace json_reader 
