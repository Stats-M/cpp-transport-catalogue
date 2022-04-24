/*
 * � ���� ����� ��������� ������/���������, ������� �������� ������ ���������� ������� (domain)
 * ���������� � �� ������� �� ������������� �����������. �������� ���������� �������� � ���������.
 *
 * �� ����� ���� �� ���������� � � transport_catalogue.h, ������ ��������� �� � ���������
 * ������������ ���� ����� ��������� ��������, ����� ���� ����� �� ������������ ����� ���������:
 * ������������ ����� (map_renderer) ����� ����� ������� ����������� �� ������������� �����������.
 */

#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <functional>      // ��� ������� hash<>

namespace transport_catalogue
{

struct Stop;      // forward declaration
struct Route;     // forward declaration

// ���: ����������� ��������� �� ������ �� ��������� � �� ���������
using StopPtr = const Stop*;
// ���: ����������� ��������� �� ������ � �������� � �� ���������
using RoutePtr = const Route*;

// ���������, �������� ���������� �� ��������� � ������������
// ������ ������ � ���
struct Stop
{
	// �������� ����������� ��-��������� ��-�� ������� ������������������ ������������
	Stop() = default;
	// ����������� �� ����� � ���� �����������
	Stop(const std::string_view, const double, const double);
	// ����������� ����������� �� ������ ������������ ��������� (��� �������� ������ ������������)
	Stop(StopPtr);

	std::string name;                   // �������� ���������
	geo::Coordinates coords{ 0L,0L };   // ����������
};

// ���������, �������� ���������� � �������� � ������������
// ������ ������ � ���
struct Route
{
	// �������� ����������� ��-��������� ��-�� ������� ������������������ ������������
	Route() = default;
    // ����������� ����������� �� ������ ������������ ��������� (��� �������� ������ ������������)
	Route(RoutePtr);

	std::string route_name;            // ����� �������� (��������)
	std::vector<StopPtr> stops;        // ��������� ���������� �� ��������� ��������
	size_t unique_stops_qty = 0U;      // ���������� ���������� ��������� �� �������� (��������, �.�. ���������� ������ ��� ������������ ��������)
	double geo_route_length = 0L;      // ����� �������� �� ������ ����� ������������ (��������, �.�. ���������� ������ ��� ������������ ��������)
	size_t meters_route_length = 0U;   // ����� �������� � ������ �������� ���������� ����� ������� (�����) (��������, �.�. ���������� ������ ��� ������������ ��������)
	double curvature = 0L;             // ������������ �������� = meters_route_length / geo_route_length. >1 ��� ������ ��������, ����� ����������
	bool is_circular = false;          // �������� �� ������� ���������
};

// ��������� ��� �������� ��������� � �������� ����
struct RendererData
{
	std::vector<geo::Coordinates> stop_coords;    // ��������� ��������� ���������
	std::vector<std::string_view> stop_names;     // ��������� ���������� �� �������� ���������
	bool is_circular = false;                     // �������� �� ������� ���������
};

// ����� ������ ��� unordered_map � ������ ���� pair<StopPtr, StopPtr>
class PairPointersHasher
{
public:
	std::size_t operator()(const std::pair<StopPtr, StopPtr> pair_of_pointers) const noexcept
	{
		//auto ptr1 = static_cast<const void*>(pair_of_pointers.first);
		//auto ptr2 = static_cast<const void*>(pair_of_pointers.second);
		//return hasher_(ptr1) * 37 + hasher_(ptr2);

		return hasher_(static_cast<const void*>(pair_of_pointers.first)) * 37
			+ hasher_(static_cast<const void*>(pair_of_pointers.second));

		/* ��� ������ ��� ���������� �������� PairPointersHasher<T==Stop>
		// ������� ������������� (������ ��������� ��� ��� ������������ ���������)
		T* ptr1 = const_cast<T*>(pair_of_pointers.first);
		T* ptr2 = const_cast<T*>(pair_of_pointers.second);
		// ���������� ������������� �����
		std::uintptr_t np_ptr1 = reinterpret_cast<std::uintptr_t>(ptr1);
		std::uintptr_t np_ptr2 = reinterpret_cast<std::uintptr_t>(ptr2);
		return hasher_(np_ptr1) * 37 + hasher_(np_ptr2);
		*/
	}

private:
	// ��� - ������� ��� ���������� �������� PairPointersHasher<T == Stop>
	//std::hash<std::uintptr_t> hasher_;

	// ����������� ���-������� ��� const void*
	std::hash<const void*> hasher_;
};

} // namespace transport_catalogue