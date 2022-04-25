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

Stop::Stop(StopPtr other_stop_ptr) :
	name(other_stop_ptr->name),
	coords(other_stop_ptr->coords)
{}

Stop::Stop(const std::string_view stop_name, const double lat, const double lng) : 
	name(stop_name), 
	coords(geo::Coordinates{ lat, lng })
{}

// ---------------- struct Route ----------------------

Route::Route(RoutePtr other_stop_ptr) :
    route_name(other_stop_ptr->route_name),
	stops(other_stop_ptr->stops),
	unique_stops_qty(other_stop_ptr->unique_stops_qty),
	geo_route_length(other_stop_ptr->geo_route_length),
	meters_route_length(other_stop_ptr->meters_route_length),
	curvature(other_stop_ptr->curvature),
	is_circular(other_stop_ptr->is_circular)
{}

// ----------- class PairPointersHasher --------------

std::size_t  PairPointersHasher::operator()(const std::pair<StopPtr, StopPtr> pair_of_pointers) const noexcept
{
	//auto ptr1 = static_cast<const void*>(pair_of_pointers.first);
	//auto ptr2 = static_cast<const void*>(pair_of_pointers.second);
	//return hasher_(ptr1) * 37 + hasher_(ptr2);

	return hasher_(static_cast<const void*>(pair_of_pointers.first)) * 37
		+ hasher_(static_cast<const void*>(pair_of_pointers.second));

	/* Код хэшера для шаблонного варианта PairPointersHasher<T==Stop>
	// Снимаем константность (нельзя приводить тип для константного указателя)
	T* ptr1 = const_cast<T*>(pair_of_pointers.first);
	T* ptr2 = const_cast<T*>(pair_of_pointers.second);
	// Приведение несовместимых типов
	std::uintptr_t np_ptr1 = reinterpret_cast<std::uintptr_t>(ptr1);
	std::uintptr_t np_ptr2 = reinterpret_cast<std::uintptr_t>(ptr2);
	return hasher_(np_ptr1) * 37 + hasher_(np_ptr2);
	*/
}


} // namespace transport_catalogue
