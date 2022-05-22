/*
 * Назначение модуля: код, отвечающий за визуализацию карты маршрутов в формате SVG.
 */

#include "map_renderer.h"

namespace map_renderer
{

// ---------------SphereProjector-------------------------

bool IsZero(const double value)
{
	return (std::abs(value) < EPSILON);
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const
{
	return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
}

// -----Классы фигур, реализующие интерфейс Drawable----

RouteLine::RouteLine(const std::vector<svg::Point>& stop_points,
					 const svg::Color& stroke_color,
					 const RendererSettings& renderer_settings) :
	stop_points_(stop_points), stroke_color_(stroke_color), renderer_settings_(renderer_settings)
{}

void RouteLine::Draw(svg::ObjectContainer& container) const
{
	svg::Polyline polyline;
	for (const auto& point : stop_points_)
	{
		polyline.AddPoint(point);
	}
	polyline.SetStrokeColor(stroke_color_);                 // StrokeColor задается в конструкторе
	polyline.SetFillColor(svg::NoneColor);                  // FillColor должен быть none
	polyline.SetStrokeWidth(renderer_settings_.line_width); // StrokeWidth должна быть = настройке line_width
	polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);   // StrokeLineCap должна быть Round
	polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND); // StrokeLineJoin должна быть Round
	container.Add(polyline);
}

TextLabel::TextLabel(const svg::Point& label_point,
					 const std::string& text,
					 const svg::Color& fill_fore_color,
					 const RendererSettings& renderer_settings,
					 const bool& is_stop) :
	label_point_(label_point), text_(text), fill_fore_color_(fill_fore_color),
	renderer_settings_(renderer_settings), is_stop_(is_stop)
{}

void TextLabel::Draw(svg::ObjectContainer& container) const
{
	using namespace std::literals;

	// Алгоритм:
	// 1. Формируем текст переднего плана
	// 2. На его основе формируем текст заднего плана (фон)
	// 3. Добавляем в контейнер в обратном порядке

	svg::Text fore_text;   // Текст переднего плана

	fore_text.SetPosition(label_point_);
	fore_text.SetFontFamily("Verdana"s);
	fore_text.SetData(text_);

	// Отдельная обработка меток маршрутов и остановок
	if (is_stop_)
	{
		// Параметры, уникальные для метки остановки
		fore_text.SetOffset(renderer_settings_.stop_label_offset);
		fore_text.SetFontSize(renderer_settings_.stop_label_font_size);
		fore_text.SetFillColor("black"s);
	}
	else
	{
		// Параметры, уникальные для метки маршрута
		fore_text.SetOffset(renderer_settings_.bus_label_offset);
		fore_text.SetFontSize(renderer_settings_.bus_label_font_size);
		fore_text.SetFontWeight("bold"s);
		fore_text.SetFillColor(fill_fore_color_);
	}

	// Текст заднего плана (фон) создаем на основе текста переднего плана
	svg::Text back_text = fore_text;

	// Параметры, уникальные для фонового текста
	back_text.SetFillColor(renderer_settings_.underlayer_color);
	back_text.SetStrokeColor(renderer_settings_.underlayer_color);
	back_text.SetStrokeWidth(renderer_settings_.underlayer_width);
	back_text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
	back_text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

	container.Add(back_text);
	container.Add(fore_text);
}

StopIcon::StopIcon(const svg::Point& label_point, const RendererSettings& renderer_settings) :
	label_point_(label_point), renderer_settings_(renderer_settings)
{}

void StopIcon::Draw(svg::ObjectContainer& container) const
{
	using namespace std::literals;

	svg::Circle icon;
	icon.SetCenter(label_point_);
	icon.SetRadius(renderer_settings_.stop_radius);
	icon.SetFillColor("white"s);

	container.Add(icon);
}


// -----------------MapRenderer-------------------------

void MapRenderer::ApplyRenderSettings(RendererSettings& settings)
{
	settings_ = settings;
}


void MapRenderer::AddRouteLinesToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
										SphereProjector& sp,
										std::map<const std::string, transport_catalogue::RendererData>& routes_to_render)
{
	for (const auto& [name, data] : routes_to_render)
	{
		std::vector<svg::Point> points;  // Вектор нормализованных координат
		for (const auto& stop : data.stop_coords)
		{
			points.push_back(sp(stop));
		}
		// формируем unique_ptr
		picture_.emplace_back(std::make_unique<RouteLine>(RouteLine{ points, GetColorFromPallete() , settings_ }));
	}

}


void MapRenderer::AddRouteLabelsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
										 SphereProjector& sp,
										 std::map<const std::string, transport_catalogue::RendererData>& routes_to_render)
{
	ResetPallette();  // Сбрасываем текущий цвет палитры, начинаем с первого
	for (const auto& [name, data] : routes_to_render)
	{
		// Получаем цвет текущего маршрута
		svg::Color current_line_color = GetColorFromPallete();
		// Выводим первую метку маршрута (стартовую)
		picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{ sp(data.stop_coords[0]),
														  name,
														  current_line_color,
														  settings_,
														  false }));
		// Если маршрут НЕ кольцевой и у него больше 1 остановки, выводим метку конечной остановки
		if ((!data.is_circular) && (data.stop_coords.size() > 1))
		{
			// Учитываем, что для некольцевых маршрутов координаты хранятся туда и обратно (А,В,А) и всегда нечетны
			// Выводим, если начальная и конечная точки не совпадают А <> В
			if (data.stop_coords[0] != data.stop_coords[(data.stop_coords.size() + 1) / 2 - 1])
			{
				// Выводим вторую метку маршрута (конечную)
				picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{ sp(data.stop_coords[(data.stop_coords.size() + 1) / 2 - 1]),
																  name,
																  current_line_color,
																  settings_,
																  false }));
			}
		}
	}
}


void MapRenderer::AddStopLabelsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
										SphereProjector& sp,
										std::map<std::string_view, geo::Coordinates> all_unique_stops)
{
	for (const auto& stop : all_unique_stops)
	{
		// формируем unique_ptr
		picture_.emplace_back(std::make_unique<StopIcon>(StopIcon{ sp(stop.second), settings_ }));
	}
}


void MapRenderer::AddStopIconsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
									   SphereProjector& sp,
									   std::map<std::string_view, geo::Coordinates> all_unique_stops)
{
	using namespace std::literals;

	for (const auto& stop : all_unique_stops)
	{
		// формируем unique_ptr
		picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{ sp(stop.second),
														  std::string(stop.first),
														  "black"s,
														  settings_,
														  true }));
	}
}


svg::Document MapRenderer::RenderMap(std::map<const std::string, transport_catalogue::RendererData>& routes_to_render)
{
	std::unordered_set<geo::Coordinates, geo::CoordinatesHasher> all_coords;    // Кэш координат для калибровки
	std::map<std::string_view, geo::Coordinates> all_unique_stops;              // Кэш уникальных остановок (сортировка по имени)

	// 1. Нормализуем координаты для переданных в рендерер точек (только непустые маршруты)
	for (const auto& [name, data] : routes_to_render)
	{
		for (size_t i = 0; i < data.stop_coords.size(); ++i)
		{
			all_coords.insert(data.stop_coords[i]);
			all_unique_stops.insert(make_pair(data.stop_names[i], data.stop_coords[i]));
		}
	}
	// Пропускаем через проектор ВСЕ координаты и получаем поправочный коэффициент
	SphereProjector sp{ std::begin(all_coords), std::end(all_coords),
		settings_.width, settings_.height, settings_.padding };

	// 2. Добавляем в контейнер Drawable-объекты, используя нормализованные координаты
	std::vector<std::unique_ptr<svg::Drawable>> picture_;   // Контейнер Drawable-объектов
	// 2.1 Линии маршрутов
	AddRouteLinesToRender(picture_, sp, routes_to_render);
	// 2.2 Метки маршрутов
	AddRouteLabelsToRender(picture_, sp, routes_to_render);
	// 2.3 Метки остановок (только те, через которые проходят маршруты)
	//      (очередность - лексикографическое возрастание)
	AddStopLabelsToRender(picture_, sp, all_unique_stops);
	// 2.4 Метки остановок (очередность - лексикографическое возрастание)
	AddStopIconsToRender(picture_, sp, all_unique_stops);

	// 3. Вызываем Draw() по очереди для всех элеметов контейнера, формируя svg::Document
	//    из svg-примитивов
	svg::Document map;
	DrawPicture(picture_, map);

	// Тут можно вручную добавить svg-примитивы прямо к map, если необходимо

	return map;  // Document.Render() будет вызывать уже метод-получатель
}

const svg::Color MapRenderer::GetColorFromPallete()
{
	if (pallette_item_ == settings_.color_palette.size())
	{
		pallette_item_ = 0;
	}
	return settings_.color_palette[pallette_item_++];
}

void MapRenderer::ResetPallette()
{
	pallette_item_ = 0;
}


}
