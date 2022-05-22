#include "transport_router.h"

namespace router
{

// -----------------TransportRouter-------------------------

TransportRouter::TransportRouter(transport_catalogue::TransportCatalogue& tc)
	: tc_(tc), dw_graph_(tc.GetAllStopsCount() * 2)
{
	// ������������� � ����� TransportRouter � ������� ������������� ������������ 
	// ���������� ������ ������ ������, �� ������ ������, ��������� ��� �������� ���� ���������.
	// (����������� ��� ����� � �������� ��������� (���� - ����� �������� �� ����� ���������))
}


void TransportRouter::ApplyRouterSettings(RouterSettings& settings)
{
    settings_ = std::move(settings);
}


const RouterSettings& TransportRouter::GetRouterSettings() const
{
	return settings_;
}


const RouteData TransportRouter::CalculateRoute(const std::string_view from, const std::string_view to)
{

	// ���� ���� ��� �� �������� (������ ������� �� ����������), ������
	if (!router_)
	{
		BuildGraph();
	}

	RouteData result;    // ��������� ���������� ��� NRVO
	auto calculated_route = router_->BuildRoute(vertexes_wait_.at(from), vertexes_wait_.at(to));

	// ���� �� � optional ������������ �������� ����?
	if (calculated_route)
	{
		// ������������� ���� ��� ����������� json
		result.founded = true;
		// �������� �� ������ ���������� ���� � ���������� ������ ��� ���������
		for (const auto& element_id : calculated_route->edges)
		{
			auto edge_details = dw_graph_.GetEdge(element_id);
			// ��������� ����� ����� � ����� �������� ����
			result.total_time += edge_details.weight;
			// �� ����� ���������� � ����� ������ ������� ������ ����������� ��� RouteItem
			result.items.emplace_back(RouteItem{
				edge_details.edge_name,
				(edge_details.type == graph::EdgeType::TRAVEL) ? edge_details.span_count : 0,
				edge_details.weight,
				edge_details.type });
		}
	}
	return result;
}


// ������ ���� � �������� ��������� (� ������� ��������)
void TransportRouter::BuildGraph()
{
	int vertex_id = 0;

	// 1. ����� ��������. �������� �� ������� ���������� �� ��� ���������
	for (const auto& stop : tc_.GetAllStopsPtr())
	{
		// ��������� ������� � ������� ������ ��������
		vertexes_wait_.insert({ stop->name, vertex_id });
		// ��������� �� �� � ������� ������� ������
		vertexes_travel_.insert({ stop->name, ++vertex_id });
		// ������� ����� ��������
		dw_graph_.AddEdge({
				vertexes_wait_.at(stop->name),    // id
				vertexes_travel_.at(stop->name),  // id
				settings_.bus_wait_time * 1.0,    // ��� == ������� �������� (double)
				stop->name,                       // ������������ ����� == ����� ���������
				graph::EdgeType::WAIT,            // ��� �����
				0                                 // span == 0 ��� ����� ��������
					   });
		++vertex_id;
	}

	// 2. ����� ������������. �������� �� ���������� �� ��� ��������
	for (const auto& route : tc_.GetAllRoutesPtr())
	{
		// �������� �� ���� ���������� (����� ���������) ������� �������� � ��� ������ �������...
		for (size_t it_from = 0; it_from < route->stops.size() - 1; ++it_from)
		{
			int span_count = 0;
			// ...�� ������ �� ���������� ��������� ��������
			for (size_t it_to = it_from + 1; it_to < route->stops.size(); ++it_to)
			{
				double road_distance = 0.0;
				// ������� ���������
				for (size_t it = it_from + 1; it <= it_to; ++it)
				{
					road_distance += static_cast<double>(tc_.GetDistance(route->stops[it - 1], route->stops[it]));
				}
				// ������� ����� ������������ � �����, ������ ������� �� ������ �� ������� ������������
				// �������� ��������� �� ������� �������� ������ ���������
				dw_graph_.AddEdge({
						vertexes_travel_.at(route->stops[it_from]->name),
						vertexes_wait_.at(route->stops[it_to]->name),
						road_distance / (settings_.bus_velocity * 1000.0 / 60.0),    // ��� (== ������� ��������)
						route->route_name,
						graph::EdgeType::TRAVEL,
						++span_count     // ������� ��������� � �����
							   });
			}
		}

		// �.�. ������������ � ��� ��� ������������ �������� �������� ������������������ ��������� ���� � �������,
		// ��������� ����� ��� ��������� �������� �� ���������, ��� ��� ��������� ��� ������ �������

		// ��������� �������� ������������� � ����� ������ � ��������� ����� ����� �� �������� � ���������
		// ����������. ������� ��� ���������� �������� ��� ����� ������� "����� ��������" � �� ����� � 
		// ����� ������ �� ���������� �� ����� �������� �������� "����-�������"
	}

	// ������� ������ ������� �� ������ ������������ �����
	router_ = std::make_unique<graph::Router<double>>(dw_graph_);
}

}