/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области
 * (domain) вашего приложения и не зависят от транспортного справочника. Например Автобусные
 * маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 */

#include "domain.h"

namespace transport_catalogue
{

// ---------------- struct Stop ----------------------

Stop::Stop(StopPtr ptr) :
	name(ptr->name), 
	coords(ptr->coords)
{}

Stop::Stop(const std::string_view stop_name, const double lat, const double lng) : 
	name(stop_name), 
	coords(geo::Coordinates{ lat, lng })
{}

// ---------------- struct Route ----------------------

Route::Route(RoutePtr ptr) :
    route_name(ptr->route_name), 
	stops(ptr->stops), 
	unique_stops_qty(ptr->unique_stops_qty), 
	geo_route_length(ptr->geo_route_length), 
	meters_route_length(ptr->meters_route_length), 
	curvature(ptr->curvature), 
	is_circular(ptr->is_circular)
{}

} // namespace transport_catalogue