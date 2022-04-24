#pragma once
#include "geo.h"           // ��� ������ � ������������ ���������
#include "domain.h"        // ������ �������� ���������, ��������� �������� � ���������

#include <deque>
#include <map>             // ��� ������� ��������� ��������� ����
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <ostream>         // ��� ���������� ��������� ������
#include <sstream>         // ��� stringstream
#include <iomanip>         // ��� ���������� ������� (������������)
#include <unordered_set>
#include <unordered_map>
#include <algorithm>       // ��� uniq(), find()
#include <utility>         // for std::pair<>
#include <cctype>          // for isspace() function

namespace transport_catalogue
{

// ----------- ������ ����� ��� ������� �� �������---------------

// ��������� ������ �� ������� ���� ���������� �� ��������� � �� ���������
struct StopStat
{
	// ����������������� �����������
	explicit StopStat(std::string_view, std::set<std::string_view>&);
	std::string_view name;
	std::set<std::string_view> buses;  // ������ ���� ����������������
};

// ���: ����������� ��������� �� ���������� �� ��������� � �� ���������
using StopStatPtr = const StopStat*;

// ��������� ������ �� ������� ���� ���������� � ��������
struct RouteStat
{	
	// ����������������� �����������
	explicit RouteStat(size_t, size_t, int64_t, double, std::string_view);
	size_t stops_on_route = 0;
	size_t unique_stops = 0;
	int64_t meters_route_length = 0;
	double curvature = 0L;
	std::string name;
};

// ���: ����������� ��������� �� ���������� � ��������
using RouteStatPtr = const RouteStat*;

// ----------- TransportCatalogue ---------------

class TransportCatalogue
{
public:
	TransportCatalogue();
	~TransportCatalogue();

	void AddStop(Stop&&);              // ��������� ��������� � ������� ���� ���������
	void AddRoute(Route&&);            // ��������� ������� � ������� ���� ���������
	void AddDistance(StopPtr, StopPtr, size_t);    // ��������� ���������� ����� ����� ����������� � �������
	size_t GetDistance(StopPtr, StopPtr);          // ���������� ���������� (size_t �����) ����� ����� ����������� � ������������� ����
	size_t GetDistanceDirectly(StopPtr, StopPtr);  // ���������� ���������� (size_t �����) ����� ����� ����������� ��� ������������ ����

	StopPtr GetStopByName(const std::string_view) const;    // ���������� ��������� �� ��������� �� �� ����� �� ������� ���������
	RoutePtr GetRouteByName(const std::string_view) const;  // ���������� ��������� �� ������� �� ��� ����� �� ������� ���������

	RouteStatPtr GetRouteInfo(const std::string_view) const;        // ���������� ��������� �� ��������� ������� � ��������
	StopStatPtr GetBusesForStopInfo(const std::string_view) const;  // ���������� ��������� �� ��������� ������� �� ��������� ��� ��������

	void GetAllRoutes(std::map<const std::string, RendererData>&) const;    // ���������� ������� ��������� � �� �����������

private:
	std::deque<Stop> all_stops_data_;                                     // ��� � ����������� ��� ���� ���������� (�������� ������, �� ���������)
	std::unordered_map<std::string_view, StopPtr> all_stops_map_;         // ������� ��������� (������� � ���-��������)
	std::deque<Route> all_buses_data_;                                    // ��� � ����������� ��� ���� ���������
	std::unordered_map<std::string_view, RoutePtr> all_buses_map_;        // ������� ��������� (���������) (������� � ���-��������)
	std::unordered_map<std::pair<StopPtr, StopPtr>, size_t, PairPointersHasher> distances_map_;    // ������� ���������� ����� �����������

	// ���������� string_view � ������ ��������� �� ��������� �� ��������� ��������� Stop
	std::string_view GetStopName(StopPtr stop_ptr);
	// ���������� string_view � ������ ��������� ��� ���������� ��������� Stop
	std::string_view GetStopName(const Stop stop);

	// ���������� string_view � ������� �������� �� ��������� �� ��������� ��������� Route
	std::string_view GetBusName(RoutePtr route_ptr);
	// ���������� string_view � ������� �������� ��� ���������� ��������� Route
	std::string_view GetBusName(const Route route);
};
}
