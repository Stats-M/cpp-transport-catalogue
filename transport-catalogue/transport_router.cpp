#include "transport_router.h"

namespace router
{

// -----------------TransportRouter-------------------------

TransportRouter::TransportRouter(transport_catalogue::TransportCatalogue& tc)
	: tc_(tc), dw_graph_(tc.GetAllStopsCount() * 2)
{
	// Маршрутизатор — класс TransportRouter — требует квадратичного относительно 
	// количества вершин объёма памяти, не считая памяти, требуемой для хранения кэша маршрутов.
	// (справедливо для графа с двойными вершинами (пара - ребро ожидания на одной остановке))
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

	// Если граф еще не построен (объект роутера не существует), строим
	if (!router_)
	{
		BuildGraph();
	}

	RouteData result;    // Локальная переменная для NRVO
	auto calculated_route = router_->BuildRoute(vertexes_wait_.at(from), vertexes_wait_.at(to));

	// Есть ли в optional рассчитанный роутером путь?
	if (calculated_route)
	{
		// Устанавливаем флаг для обработчика json
		result.founded = true;
		// Проходим по ребрам найденного пути и составляем список его элементов
		for (const auto& element_id : calculated_route->edges)
		{
			auto edge_details = dw_graph_.GetEdge(element_id);
			// Суммируем время ребра с общим временем пути
			result.total_time += edge_details.weight;
			// Из общей информации о ребре делаем выборку только необходимой для RouteItem
			result.items.emplace_back(RouteItem{
				edge_details.edge_name,
				(edge_details.type == graph::EdgeType::TRAVEL) ? edge_details.span_count : 0,
				edge_details.weight,
				edge_details.type });
		}
	}
	return result;
}


// Строим граф с двойными вершинами (с ребрами ожидания)
void TransportRouter::BuildGraph()
{
	int vertex_id = 0;

	// 1. Ребра ожидания. Проходим по вектору указателей на все остановки
	for (const auto& stop : tc_.GetAllStopsPtr())
	{
		// Добавляем вершину в словарь вершин ожидания
		vertexes_wait_.insert({ stop->name, vertex_id });
		// Добавляем ее же в словарь обычных вершин
		vertexes_travel_.insert({ stop->name, ++vertex_id });
		// Создаем ребро ожидания
		dw_graph_.AddEdge({
				vertexes_wait_.at(stop->name),    // id
				vertexes_travel_.at(stop->name),  // id
				settings_.bus_wait_time * 1.0,    // вес == времени ожидания (double)
				stop->name,                       // наименование ребра == имени остановки
				graph::EdgeType::WAIT,            // тип ребра
				0                                 // span == 0 для ребра ожидания
					   });
		++vertex_id;
	}

	// 2. Ребра передвижения. Проходим по указателям на все маршруты
	for (const auto& route : tc_.GetAllRoutesPtr())
	{
		// Проходим по всем остановкам (кросе последней) каждого маршрута и для каждой текущей...
		for (size_t it_from = 0; it_from < route->stops.size() - 1; ++it_from)
		{
			int span_count = 0;
			// ...до каждой из оставшихся остановок маршрута
			for (size_t it_to = it_from + 1; it_to < route->stops.size(); ++it_to)
			{
				double road_distance = 0.0;
				// Считаем дистанцию
				for (size_t it = it_from + 1; it <= it_to; ++it)
				{
					road_distance += static_cast<double>(tc_.GetDistance(route->stops[it - 1], route->stops[it]));
				}
				// Создаем ребро передвижения с весом, равным времени на проезд от вершины передвижения
				// исходной остановки до вершины ожидания другой остановки
				dw_graph_.AddEdge({
						vertexes_travel_.at(route->stops[it_from]->name),
						vertexes_wait_.at(route->stops[it_to]->name),
						road_distance / (settings_.bus_velocity * 1000.0 / 60.0),    // вес (== времени движения)
						route->route_name,
						graph::EdgeType::TRAVEL,
						++span_count     // Счетчик остановок в ребре
							   });
			}
		}

		// Т.к. архитектурно у нас для некольцевого маршрута хранится последовательность остановок туда и обратно,
		// добавлять графы для обратного маршрута не требуется, они уже добавлены при первом проходе

		// Кольцевые маршруты заканчиваются в месте старта и пассажиру нужно выйти из автобуса и дождаться
		// следующего. Поэтому для кольцевого маршрута нет графа проезда "между кольцами" и он ничем в 
		// таком случае не отличается от графа обычного маршрута "туда-обратно"
	}

	// Создаем объект роутера на основе построенного графа
	router_ = std::make_unique<graph::Router<double>>(dw_graph_);
}

}