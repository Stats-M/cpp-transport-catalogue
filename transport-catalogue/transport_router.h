#pragma once

#include "domain.h"    // для доступа к настройкам роутера
#include "transport_catalogue.h"
#include "router.h"
#include <memory>



namespace router
{

// Настройки роутера c дефолтными значениями
struct RouterSettings
{
	int bus_velocity = 40;
	int bus_wait_time = 6;
};

// Элемент маршрута. Из элементов роутер составляет итоговый путь следования
struct RouteItem
{
	std::string edge_name;    // Либо название остановки (если едем), либо имя маршрута (если ждем)
	int span_count = 0;       // Количество остановок без пересадок
	double time = 0.0;        // Время путешествия/ожидания по этому элементу маршрута
	graph::EdgeType type;     // Тип ребра
};

// Ответ роутера на запрос о маршруте, состоящего из вектора ребер графа (элементов маршрута)
struct RouteData
{
	double total_time = 0.0;        // Суммарное время в пути
	std::vector<RouteItem> items;   // Вектор элементов маршрута
	bool founded = false;           // Найден ли маршрут. Нужен для корректного вывода в json
};

// -----------------TransportRouter-------------------------

class TransportRouter
{
public:
	TransportRouter(transport_catalogue::TransportCatalogue&);

	void ApplyRouterSettings(RouterSettings&);
	RouterSettings GetRouterSettings() const;

	// Строит маршрут между двумя остановками
	const RouteData CalculateRoute(const std::string_view, const std::string_view);

private:
	void BuildGraph();    // Создает граф на основе данных транспортного каталога

	RouterSettings settings_;         // Настройки роутера по-умолчанию
	transport_catalogue::TransportCatalogue& tc_;          // Ссылка на каталог для выполнения запросов

	graph::DirectedWeightedGraph<double> dw_graph_;                // Граф с весами типа double (для простоты расчетов)
	std::unique_ptr<graph::Router<double>> router_ = nullptr;      // Указатель на объект роутера на основе графа dw_graph_
	std::unordered_map<std::string_view, size_t> vertexes_wait_;   // Словарь вершин "ожидания", виртуальные двойники реальных остановок
	std::unordered_map<std::string_view, size_t> vertexes_travel_; // Словарь вершин реальных остановок
};

}