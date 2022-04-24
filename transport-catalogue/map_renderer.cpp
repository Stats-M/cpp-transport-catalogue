/*
 * � ���� ����� �� ������ ���������� ���, ���������� �� ������������ ����� ��������� � ������� SVG.
 * ������������ ��������� ��� ����������� �� ������ ����� ��������� �������.
 * ���� ������ �������� ���� ������.
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

// -----������ �����, ����������� ��������� Drawable----

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
	polyline.SetStrokeColor(stroke_color_);                 // StrokeColor �������� � ������������
	polyline.SetFillColor(svg::NoneColor);                  // FillColor ������ ���� none
	polyline.SetStrokeWidth(renderer_settings_.line_width); // StrokeWidth ������ ���� = ���������line_width
	polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);   // StrokeLineCap ������ ���� Round
	polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND); // StrokeLineJoin ������ ���� Round
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

	// ��������:
	// 1. ��������� ����� ��������� �����
	// 2. �� ��� ������ ��������� ����� ������� ����� (���)
	// 3. ��������� � ��������� � �������� �������

	svg::Text fore_text;   // ����� ��������� �����

	fore_text.SetPosition(label_point_);
	fore_text.SetFontFamily("Verdana"s);
	fore_text.SetData(text_);

	// ��������� ��������� ����� ��������� � ���������
	if (is_stop_)
	{
		// ���������, ���������� ��� ����� ���������
		fore_text.SetOffset(renderer_settings_.stop_label_offset);
		fore_text.SetFontSize(renderer_settings_.stop_label_font_size);
		fore_text.SetFillColor("black"s);
	}
	else
	{
		// ���������, ���������� ��� ����� ��������
		fore_text.SetOffset(renderer_settings_.bus_label_offset);
		fore_text.SetFontSize(renderer_settings_.bus_label_font_size);
		fore_text.SetFontWeight("bold"s);
		fore_text.SetFillColor(fill_fore_color_);
	}

	 // ����� ������� ����� (���) ������� �� ������ ������ ��������� �����
	svg::Text back_text = fore_text;

	// ���������, ���������� ��� �������� ������
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

svg::Document MapRenderer::RenderMap(std::map<const std::string, transport_catalogue::RendererData>& routes_to_render)
{
	using namespace std::literals;

	// 1. ����������� ���������� ��� ���������� � �������� ����� (������ �������� ��������)
	std::unordered_set<geo::Coordinates, geo::CoordinatesHasher> all_coords;                             // ��� ��������� ��� ����������
	//std::unordered_map<geo::Coordinates, std::string_view, geo::CoordinatesHasher> all_unique_stops;   // ��� ���������� ���������
	std::map<std::string_view, geo::Coordinates> all_unique_stops;               // ��� ���������� ���������
	for (const auto& [name, data] : routes_to_render)
	{
		//for (const auto& stop : data.stop_coords)
		//{
		//	all_coords.insert(stop);
		//}
		for (size_t i = 0; i < data.stop_coords.size(); ++i)
		{
			all_coords.insert(data.stop_coords[i]);
			//all_unique_stops.insert(make_pair(data.stop_coords[i], data.stop_names[i]));
			all_unique_stops.insert(make_pair(data.stop_names[i], data.stop_coords[i]));
		}
	}
	// ���������� ����� �������� ��� ���������� � �������� ����������� �����������
	SphereProjector sp{ std::begin(all_coords), std::end(all_coords), 
		settings_.width, settings_.height, settings_.padding};

	// 2. ��������� � ��������� ������� Drawable, ��������� ��������������� ����������
	std::vector<std::unique_ptr<svg::Drawable>> picture_;   // ��������� Drawable-��������
	// 2.1 ����� ���������
	for (const auto& [name, data] : routes_to_render)
	{
		std::vector<svg::Point> points;  // ������ ��������������� ���������
		for (const auto& stop : data.stop_coords)
		{
			points.push_back(sp(stop));
		}
		// ��������� unique_ptr
		picture_.emplace_back(std::make_unique<RouteLine>(RouteLine{ points, GetColorFromPallete() , settings_ }));
	}

	// 2.2 ����� ���������
	ResetPallette();  // ���������� ������� ���� �������, �������� � �������
	for (const auto& [name, data] : routes_to_render)
	{
		// �������� ���� �������� ��������
		svg::Color current_line_color = GetColorFromPallete();
		// ������� ������ ����� �������� (���������)
		picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{sp(data.stop_coords[0]),
														  name, 
														  current_line_color,
														  settings_, 
														  false}));
		// ���� ������� �� ��������� � � ���� ������ 1 ���������, ������� ����� �������� ���������
		if ((!data.is_circular) && (data.stop_coords.size() > 1))
		{
			// ���������, ��� ��� ����������� ��������� ���������� �������� ���� � ������� (�,�,�) � ������ �������
			// �������, ���� ��������� � �������� ����� �� ��������� � <> �
			if (data.stop_coords[0] != data.stop_coords[(data.stop_coords.size() + 1) / 2 - 1])
			{
				// ������� ������ ����� �������� (��������)
				picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{ sp(data.stop_coords[(data.stop_coords.size() + 1) / 2 - 1]),
																  name,
																  current_line_color,
																  settings_,
																  false }));
			}
		}
	}

	// 2.3 ����� ��������� (������ ��, ����� ������� �������� ��������)
	//      (����������� - ������������������ �����������)
	/*
	for (const auto& stop : all_coords)
	{
		// ��������� unique_ptr
		picture_.emplace_back(std::make_unique<StopIcon>(StopIcon{ sp(stop), settings_}));
	}
	*/
	for (const auto& stop : all_unique_stops)
	{
		// ��������� unique_ptr
		picture_.emplace_back(std::make_unique<StopIcon>(StopIcon{ sp(stop.second), settings_ }));
	}


	// 2.4 ����� ��������� (����������� - ������������������ �����������)
	for (const auto& stop : all_unique_stops)
	{
		// ��������� unique_ptr
		picture_.emplace_back(std::make_unique<TextLabel>(TextLabel{ sp(stop.second),
														  std::string(stop.first),
														  "black"s,
														  settings_,
														  true }));
	}

	// �������� Draw() �� ������� ��� ���� �������� ���������� �������� Drawable, 
	// �������� svg::Document
	svg::Document map;
	DrawPicture(picture_, map);
	// ��� ����� �������� svg-��������� ����� � map, ���� ����������

	return map;  // Document.Render() ����� �������� ��� �����-����������
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
