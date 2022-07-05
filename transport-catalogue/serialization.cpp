#include "serialization.h"

namespace serialization
{

// Конструктор
Serializer::Serializer(transport_catalogue::TransportCatalogue& tc,
					   map_renderer::MapRenderer& mr,
					   router::TransportRouter* tr  /*=nullptr*/)
	: tc_(tc), mr_(mr), tr_(tr)
{}

// Public methods

void Serializer::Serialize(const std::string& filename)
{
	std::ofstream out(filename, std::ios::binary);
	proto_all_settings_.Clear();
	// Сохраняем настройки в proto структуру класса
	SerializeStop();
	SerializeDistance();
	SerializeRoute();
	SerealizeRendererSettings();
	SerealizeRouterSettings();
	// Сериализуем прото-структуру в файл
	proto_all_settings_.SerializeToOstream(&out);
}

void Serializer::Deserialize(const std::string& filename)
{
	// Десериализуем настройки из файла в proto структуру класса
	std::ifstream in(filename, std::ios::binary);
	proto_all_settings_.Clear();
	proto_all_settings_.ParseFromIstream(&in);

	// Разбираем настройки из прото-структуры класса
	DeserializeCatalogue();
	DeserializeRenderer();
}

void Serializer::DeserealizeRouter(router::TransportRouter* tr)
{
	if (tr == nullptr)
	{
		throw std::runtime_error("No router object set (nullptr)");
	}

	// Сохраняем указатель на существующий вовне класса объект роутера
	tr_ = tr;

	// Настройки читаем из классовой переменной proto_all_settings_

	// Короткая ссылка на ветвь настроек роутера внутри общей прото-структуры
	const proto_serialization::RouterSettings proto_rt_settings = proto_all_settings_.router_settings();

	router::RouterSettings r_settings;
	r_settings.bus_velocity = proto_rt_settings.bus_velocity();
	r_settings.bus_wait_time = proto_rt_settings.bus_wait_time();
	tr_->ApplyRouterSettings(r_settings);
}

// Private methods

void Serializer::SerializeStop()
{
	for (const auto& stop : tc_.GetAllStopsPtr())
	{
		// stop - элемент вектора типа 
		// std::vector<StopPtr>

		// Вспомогательные переменные
		proto_serialization::Stop proto_stop;
		proto_serialization::Coordinates proto_coords;
		
		// Заполняем временые переменные данными
		proto_coords.set_lat(stop->coords.lat);
		proto_coords.set_lng(stop->coords.lng);
		*proto_stop.mutable_coords() = proto_coords;
		proto_stop.set_name(stop->name);
		// Запоминаем в классовой прото-структуре
		*proto_all_settings_.add_stops() = proto_stop;
	}
}


void Serializer::SerializeDistance()
{
	for (const auto& distance : tc_.GetAllDistances())
	{
		// distance - элемент словаря типа 
		// std::unordered_map<std::pair<StopPtr, StopPtr>, size_t...>

		// Вспомогательные переменные
		proto_serialization::Distance proto_distance;

		// Заполняем временые переменные данными.
		proto_distance.set_from(distance.first.first->name);
		proto_distance.set_to(distance.first.second->name);
		proto_distance.set_distance(distance.second);
		// Запоминаем в классовой прото-структуре
		*proto_all_settings_.add_distances() = proto_distance;
	}
}

void Serializer::SerializeRoute()
{
	for (const auto& route : tc_.GetAllRoutesPtr())
	{
		// stop - элемент дека типа 
		// std::deque<RoutePtr>

		// Вспомогательные переменные
		proto_serialization::Route proto_route;

		// Заполняем временые переменные данными.
		proto_route.set_route_name(route->route_name);
		proto_route.set_is_circular(route->is_circular);

		// ВНИМАНИЕ! Некольцевые маршруты в каталоге достраиваются
		// до кольцевых. Для них сериализовать нужно только 
		// первую половину остановок + 1
		size_t num_stops_to_process = (route->is_circular ? route->stops.size() : route->stops.size() / 2 + 1);
		for (auto stop : route->stops)
		{
			// Проверим, достиг ли счетчик необработанных остановок нуля
			if (num_stops_to_process == 0)
			{
				// Достиг. Выходим из цикла
				break;
			}
			--num_stops_to_process;

			// Вспомогательные переменные вложенного цикла
			proto_serialization::Stop proto_stop;
			proto_serialization::Coordinates proto_coords;

		    // Заполняем временые переменные данными.
			proto_coords.set_lat(stop->coords.lat);
			proto_coords.set_lng(stop->coords.lng);
			proto_stop.set_name(stop->name);
			*proto_stop.mutable_coords() = proto_coords;

			// Запоминаем во временной прото-структуре метода
			*proto_route.add_stops() = proto_stop;
		}

		// Запоминаем в классовой прото-структуре
		*proto_all_settings_.add_routes() = proto_route;
	}
}

proto_serialization::Color Serializer::SerializeColor(const svg::Color& color)
{
	// Вспомогательные переменные
	proto_serialization::Color proto_color;

	if (std::holds_alternative<svg::Rgb>(color))
	{
		// Вспомогательные переменные
		proto_serialization::Rgb proto_rgb;
		svg::Rgb rgb = std::get<svg::Rgb>(color);
		// Заполняем временые переменные данными.
		proto_rgb.set_red(rgb.red);
		proto_rgb.set_green(rgb.green);
		proto_rgb.set_blue(rgb.blue);

		// Запоминаем во временной прото-структуре метода
		proto_color.set_is_rgba(false);
		*proto_color.mutable_rgb() = proto_rgb;
	}
	else if (std::holds_alternative<svg::Rgba>(color))
	{

		// Вспомогательные переменные
		proto_serialization::Rgba proto_rgba;
		svg::Rgba rgba = std::get<svg::Rgba>(color);
		// Заполняем временые переменные данными.
		proto_rgba.set_red(rgba.red);
		proto_rgba.set_green(rgba.green);
		proto_rgba.set_blue(rgba.blue);
		proto_rgba.set_opacity(rgba.opacity);

		// Запоминаем во временной прото-структуре метода
		proto_color.set_is_rgba(true);
		*proto_color.mutable_rgba() = proto_rgba;
	}
	else if (std::holds_alternative<std::string>(color))
	{
		//proto_color.set_name(std::get<std::string>(color));
		*proto_color.mutable_name() = std::get<std::string>(color);
	}

	return proto_color;
}

void Serializer::SerealizeRendererSettings()
{
	// Вспомогательные переменные
	const auto renderer_settings = mr_.GetRendererSettings();
	proto_serialization::RendererSettings proto_renderer_settings;
	proto_serialization::Point proto_point;

	// Заполняем временые переменные данными.
	proto_renderer_settings.set_width(renderer_settings.width);
	proto_renderer_settings.set_height(renderer_settings.height);
	proto_renderer_settings.set_padding(renderer_settings.padding);
	proto_renderer_settings.set_line_width(renderer_settings.line_width);
	proto_renderer_settings.set_stop_radius(renderer_settings.stop_radius);
	proto_renderer_settings.set_bus_label_font_size(renderer_settings.bus_label_font_size);
	proto_renderer_settings.set_stop_label_font_size(renderer_settings.stop_label_font_size);
	proto_renderer_settings.set_underlayer_width(renderer_settings.underlayer_width);

	proto_point.set_x(renderer_settings.bus_label_offset.x);
	proto_point.set_y(renderer_settings.bus_label_offset.y);
	// Запоминаем настройки во временной прото-структуре метода
	*proto_renderer_settings.mutable_bus_label_offset() = proto_point;

	proto_point.Clear();
	proto_point.set_x(renderer_settings.stop_label_offset.x);
	proto_point.set_y(renderer_settings.stop_label_offset.y);
	// Запоминаем настройки во временной прото-структуре метода
	*proto_renderer_settings.mutable_stop_label_offset() = proto_point;

	// Запоминаем настройки во временной прото-структуре метода
	*proto_renderer_settings.mutable_underlayer_color() = SerializeColor(renderer_settings.underlayer_color);

	// Запоминаем цвета палитры во временной прото-структуре метода
	for (const auto& color : renderer_settings.color_palette)
	{
		*proto_renderer_settings.add_color_palette() = SerializeColor(color);
	}

	// Запоминаем настройки в классовой прото-структуре
	*proto_all_settings_.mutable_renderer_settings() = proto_renderer_settings;
}


void Serializer::SerealizeRouterSettings()
{
	// Вспомогательные переменные
	proto_serialization::RouterSettings proto_router_settings;
	// Читаем настройки роутера
	const auto rt_settings = tr_->GetRouterSettings();

	// Заполняем временые переменные данными
	proto_router_settings.set_bus_velocity(rt_settings.bus_velocity);
	proto_router_settings.set_bus_wait_time(rt_settings.bus_wait_time);

	// Запоминаем в классовой прото-структуре
	*proto_all_settings_.mutable_router_settings() = proto_router_settings;
}


void Serializer::DeserializeCatalogue()
{
	// 1. Восстанавливаем данные об остановках
	for (int i = 0; i < proto_all_settings_.stops_size(); ++i)  // .stops().size()
	{
		proto_serialization::Stop proto_stop = proto_all_settings_.stops(i);
		tc_.AddStop(transport_catalogue::Stop(proto_stop.name(), proto_stop.coords().lat(), proto_stop.coords().lng()));
	}

	// 2. Восстанавливаем данные о расстояниях
	for (int i = 0; i < proto_all_settings_.distances_size(); ++i)  // .distances().size()
	{
		transport_catalogue::StopPtr stop_from_ptr = tc_.GetStopByName(proto_all_settings_.distances(i).from());
		transport_catalogue::StopPtr stop_to_ptr = tc_.GetStopByName(proto_all_settings_.distances(i).to());
		size_t distance = proto_all_settings_.distances(i).distance();
		tc_.AddDistance(stop_from_ptr, stop_to_ptr, distance);
	}

	// 3. Восстанавливаем данные о маршрутах
	for (int i = 0; i < proto_all_settings_.routes_size(); ++i)  // .routes().size()
	{
		// Текущий маршрут из классовой прото-структуры
		proto_serialization::Route proto_route = proto_all_settings_.routes(i);
		// Маршрут транспортного каталога
		transport_catalogue::Route route;
		
		route.route_name = proto_route.route_name();
		route.is_circular = proto_route.is_circular();

		// ВНИМАНИЕ! tc_.AddRoute() достраивает не круговой маршрут до кругового.
		// Этот момент учитывается при сериализации, сохраняя лишь 1/2 маршрута.
		// При десериализации никаких действий не требуется, 
		// AddRoute() все сделает автоматически

		// Поостановочно переносим маршрут из proto_route в route
		for (const auto& proto_stop : proto_route.stops())
		{
			// Преобразуем прото-имя в указатель каталога
			route.stops.push_back(tc_.GetStopByName(proto_stop.name()));
		}

		tc_.AddRoute(std::move(route));
	}
}

void Serializer::DeserializeRenderer()
{
	// Считываем настройки рендеринга
	mr_.ApplyRendererSettings(DeserializeRendererSettings(proto_all_settings_.renderer_settings()));
}

map_renderer::RendererSettings Serializer::DeserializeRendererSettings(const proto_serialization::RendererSettings& proto_renderer_settings)
{
	// Вспомогательные переменные
	map_renderer::RendererSettings renderer_settings;
	// Очистим массив цветов палитры от дефолтных элементов
	renderer_settings.color_palette.clear();

	// Считываем настройки из прото-структуры
	renderer_settings.width = proto_renderer_settings.width();
	renderer_settings.height = proto_renderer_settings.height();
	renderer_settings.padding = proto_renderer_settings.padding();
	renderer_settings.line_width = proto_renderer_settings.line_width();
	renderer_settings.stop_radius = proto_renderer_settings.stop_radius();
	renderer_settings.bus_label_font_size = proto_renderer_settings.bus_label_font_size();
	renderer_settings.stop_label_font_size = proto_renderer_settings.stop_label_font_size();
	renderer_settings.underlayer_width = proto_renderer_settings.underlayer_width();

	renderer_settings.bus_label_offset = { proto_renderer_settings.bus_label_offset().x(), proto_renderer_settings.bus_label_offset().y() };
	renderer_settings.stop_label_offset = { proto_renderer_settings.stop_label_offset().x(), proto_renderer_settings.stop_label_offset().y() };

	renderer_settings.underlayer_color = DeserializeColor(proto_renderer_settings.underlayer_color());

    // Заполняем палитру цветами из прото-структуры. Дефолтные цвета были стерты ранее
	for (const auto& proto_color : proto_renderer_settings.color_palette())
	{
		renderer_settings.color_palette.push_back(DeserializeColor(proto_color));
	}

	return renderer_settings;
}

svg::Color Serializer::DeserializeColor(const proto_serialization::Color& proto_color)
{
	if (!proto_color.name().empty())
	{
		// Цвет в формате string
		return svg::Color{ proto_color.name() };
	}
	else if (proto_color.is_rgba())
	{
		// Цвет в формате Rgba
		return svg::Rgba(proto_color.rgba().red(), proto_color.rgba().green(), proto_color.rgba().blue(), proto_color.rgba().opacity());
	}
	// Последний вариант - цвет в формате Rgb
	return svg::Rgb(proto_color.rgb().red(), proto_color.rgb().green(), proto_color.rgb().blue());
}

} // namespace serialization