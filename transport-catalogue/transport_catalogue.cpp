#include "transport_catalogue.h"

namespace transport_catalogue
{

// ----------- ������ ����� ��� ������� �� �������---------------

StopStat::StopStat(std::string_view stop_name, std::set<std::string_view>& buses) :
	name(stop_name), buses(buses)
{}

RouteStat::RouteStat(size_t stops, size_t unique_stops, int64_t meters_length, double curvature, std::string_view name) :
	stops_on_route(stops), 
	unique_stops(unique_stops),
	meters_route_length(meters_length), 
	curvature(curvature), 
	name(name)
{}

// ----------- TransportCatalogue ---------------

TransportCatalogue::TransportCatalogue()
{}

TransportCatalogue::~TransportCatalogue()
{}

// ��������� ��������� � ������� ���� ���������
void TransportCatalogue::AddStop(Stop&& stop)
{
	if (all_stops_map_.count(GetStopName(&stop)) == 0)
	{
		// ����� ��������� � ���� ���, ��������

		// 1. ��������� ��������� � ���-���������, ��������� stop
		auto& ref = all_stops_data_.emplace_back(std::move(stop));
		// 2. ��������� ��������� � ������� ���������
		all_stops_map_.insert({ std::string_view(ref.name), &ref });
	}
}

// ��������� ������� � ������� ���� ���������
void TransportCatalogue::AddRoute(Route&& route)
{
	// ����������� �� ���� ������� route � ����?
	if (all_buses_map_.count(route.route_name) == 0)
	{
		// ������ �������� � ���� ���, ��������

		/* �������� ���������: � route ���������� vector<StopPtr>, �� ��� ��� �� 
		* ==nullptr ����� ��������� ���������� �������, �.�. ������� �� ��������
		* ������ ���������, ��� ����� ��������� ���� ������ � ������� ������ �������
		// 1 ��������� ������������� ���������, �������������� ������� �� route
		route.stops.erase(std::remove_if(route.stops.begin(), route.stops.end(),
							[&](auto& x)
							{
								return (all_stops_map_.count(x->name) == 0);
							}),
						  route.stops.end());
		*/

		// 2. ��������� ������� (�������) � ���-���������, ��������� route
		auto& ref = all_buses_data_.emplace_back(std::move(route));

		// 3. ��������� ��������� �� ������� (�������) �� ��������� � ������� ���������
		all_buses_map_.insert({ std::string_view(ref.route_name), &ref });

		// 4. ������������ ���������� ��������� �� ��������
		// �������� ������ ����������
		std::vector<StopPtr> tmp = ref.stops;
		std::sort(tmp.begin(), tmp.end());
		auto last = std::unique(tmp.begin(), tmp.end());
		// ��������� ���������� ���������� ��������� �� ��������
		ref.unique_stops_qty = (last != tmp.end() ? std::distance(tmp.begin(), last) : tmp.size());

		// 5. ���� ������� �� ���������, ����������� �������� �������
		if (!ref.is_circular)
		{
			// ����������� �������� ������� ��� ������������ ��������
			for (int i = ref.stops.size() - 2; i >= 0; --i)
			{
				// ��������� � ����� ������� ��������� � �������� �����������, ����� �������� stops_list[size() - 1]
				ref.stops.push_back(ref.stops[i]);
			}
		}

		// 6. ������������ ����� ��������
		int stops_num = static_cast<int>(ref.stops.size());
		if (stops_num > 1)
		{
			ref.geo_route_length = 0L;
			ref.meters_route_length = 0U;
			for (int i = 0; i < stops_num - 1; ++i)
			{
				ref.geo_route_length += ComputeDistance(ref.stops[i]->coords, ref.stops[i + 1]->coords);
				ref.meters_route_length += GetDistance(ref.stops[i], ref.stops[i + 1]);
			}
			// ������������ �������� ��������
			ref.curvature = ref.meters_route_length / ref.geo_route_length;
		}
		else
		{
			// � �������� 0 ��� 1 ���������. ����� == 0
			ref.geo_route_length = 0L;
			ref.meters_route_length = 0U;
			ref.curvature = 1L;
		}
	}
}

// ��������� ���������� ����� ����� ����������� � �������
void TransportCatalogue::AddDistance(StopPtr stop_from, StopPtr stop_to, size_t dist)
{
	if (stop_from != nullptr && stop_to != nullptr)
	{
		// ������ ������ � ������� ����������
		distances_map_.insert({ { stop_from, stop_to }, dist });
	}
}

// ���������� ���������� (size_t �����) ����� ����� ����������� � ������������� ����
size_t TransportCatalogue::GetDistance(StopPtr stop_from, StopPtr stop_to)
{
	size_t result = GetDistanceDirectly(stop_from, stop_to);
	// ���� ������� ���������� � ������� ���, ���������� �������� ����������, ���� ���� ��� 0
	return (result > 0 ? result : GetDistanceDirectly(stop_to, stop_from));
}

// ���������� ���������� (size_t �����) ����� ����� ����������� ��� ������������ ����
size_t TransportCatalogue::GetDistanceDirectly(StopPtr stop_from, StopPtr stop_to)
{
	if (distances_map_.count({ stop_from, stop_to }) > 0)
	{
		return distances_map_.at({ stop_from, stop_to });
	}
	else
	{
		return 0U;
	}
}


std::string_view TransportCatalogue::GetStopName(StopPtr stop_ptr)
{
	return std::string_view(stop_ptr->name);
}

std::string_view TransportCatalogue::GetStopName(const Stop stop)
{
	return std::string_view(stop.name);
}

std::string_view TransportCatalogue::GetBusName(RoutePtr route_ptr)
{
	return std::string_view(route_ptr->route_name);
}

std::string_view TransportCatalogue::GetBusName(const Route route)
{
	return std::string_view(route.route_name);
}


// ���������� ��������� �� ��������� �� �� ����� �� ������� ���������
StopPtr TransportCatalogue::GetStopByName(const std::string_view stop_name) const
{
	if (all_stops_map_.count(stop_name) == 0)
	{
		// ����� ��������� � ���� ���
		return nullptr;
	}
	else
	{
		return all_stops_map_.at(stop_name);
	}
}


// ���������� ��������� �� ������� �� ��� ����� �� ������� ���������
RoutePtr TransportCatalogue::GetRouteByName(const std::string_view bus_name) const
{
	if (all_buses_map_.count(bus_name) == 0)
	{
		// ����� ��������� � ���� ���
		return nullptr;
	}
	else
	{
		return all_buses_map_.at(bus_name);
	}
}


// ���������� ��������� �� ��������� ������� � ��������
RouteStatPtr TransportCatalogue::GetRouteInfo(const std::string_view route_name) const
{
	RoutePtr ptr = GetRouteByName(route_name);

	if (ptr == nullptr)
	{
		// ������� � ����� ������ �� ����������
		return nullptr;
	}

	// ������� � ����� ������ ����������
	return new RouteStat(ptr->stops.size(), 
						 ptr->unique_stops_qty, 
						 ptr->meters_route_length,
						 ptr->curvature, 
						 ptr->route_name);
}


// ���������� ��������� �� ��������� ������� �� ��������� ��� ��������
StopStatPtr TransportCatalogue::GetBusesForStopInfo(const std::string_view stop_name) const
{
	StopPtr ptr = GetStopByName(stop_name);

	if (ptr == nullptr)
	{
		// ����� ��������� �� �������.
		return nullptr;
	}

	// TODO �������� �����, ������� �������� ������� stops_to_buses_map_

	//std::vector<std::string_view> found_buses_sv;   // ��������� ������ ��������� �����������
	std::set<std::string_view> found_buses_sv;   // ��������� ������ ��������� ��������� �����������
	for (const auto& bus : all_buses_map_)
	{
		// ���� � ������� ��������� ��� �������� �������� ����� bus ���� 1 ���������� � �������� ����������
		auto tmp = std::find_if(bus.second->stops.begin(), bus.second->stops.end(),
								[stop_name](StopPtr curr_stop)
								{
									return (curr_stop->name == stop_name);
								});
		if (tmp != bus.second->stops.end())
		{
			// ���-�� �������. ��������� � �����
			found_buses_sv.insert(bus.second->route_name);
		}
	}

	// ���������� ���������, ���� ���� ��������� ������, ��� �������� ���������
	return new StopStat(stop_name, found_buses_sv);
}


void TransportCatalogue::GetAllRoutes(std::map<const std::string, RendererData>& all_routes) const
{

	for (const auto& route : all_buses_data_)
	{
		if (route.stops.size() > 0)
		{
			// ������ ��� �������� ��������� (������ �� ��������� � ������������ ���������)
			RendererData item;
			for (StopPtr stop : route.stops)
			{
				item.stop_coords.push_back(stop->coords);
				item.stop_names.push_back(stop->name);
			}
			item.is_circular = route.is_circular;
			
			all_routes.emplace(make_pair(route.route_name, item));
		}
	}

	return;
}

}