/*
 * ����� ����� ���� �� ���������� ��� ����������� �������� � ����, ����������� ������,
 * ������� �� �������� �� �������� �� � transport_catalogue, �� � json reader.
 *
 * � �������� ��������� ��� ���� ���������� ��������� �� ���� ������ ����������� ��������.
 * �� ������ ����������� ��������� �������� ��������, ������� ������� ���.
 *
 * ���� �� ������������� �������, ��� ����� ���� �� ��������� � ���� ����,
 * ������ �������� ��� ������.
 * 
 *    request_handler - ���������("�����) ������������� ����������� (�������� ������)
 *        json_reader - ��������� ������ � ������� ������� json + ���������� ������
 */

#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <unordered_set>    // ��� ���� ������ � RequestHandler
#include <optional>         // ��� ���� ������ � RequestHandler
#include <string_view>      // ��� ���� ������ � RequestHandler
#include <map>              // ��� �������� ������� ���� ���������

// � ������� �������� ����� ������ ����������� ��������� � ��������� ���� ���� utility.h/.cpp
namespace detail
{
// ����������� 1 ������ � 2, ����������� count �� ������� ��������-������������
std::pair<std::string_view, std::string_view> Split(std::string_view, char, int count = 1);
// ������� ������� (�������� �����, ��������� � �.�.) �� ������ ������
std::string_view Lstrip(std::string_view);
// ������� ������� (�������� �����, ��������� � �.�.) � ����� ������
std::string_view Rstrip(std::string_view);
// ������� ������� (�������� �����, ��������� � �.�.) �� ���� ������ ������
std::string_view TrimString(std::string_view);
}

namespace transport_catalogue
{
// ����� RequestHandler ������ ���� ������, ����������� �������������� JSON reader-�
// � ������� ������������ ����������.
// ��. ������� �������������� �����: https://ru.wikipedia.org/wiki/�����_(������_��������������)
class RequestHandler {
public:
    // MapRenderer ����������� � ��������� ����� ��������� �������
    RequestHandler(const TransportCatalogue& tc, map_renderer::MapRenderer& mr) : tc_(tc), mr_(mr)
    {}
    
    // ���������� ���������� � ��������
    const std::optional<RouteStatPtr> GetRouteInfo(const std::string_view& bus_name) const;

    // ���������� ��������, ���������� ����� ���������
    //const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
    const std::optional<StopStatPtr> GetBusesForStop(const std::string_view& stop_name) const;

    // ���������� SVG ��������, �������������� map_renderer
    svg::Document GetMapRender() const;

private:
    // RequestHandler ���������� ��������� �������� "������������ ����������" � "������������ �����"
    const TransportCatalogue& tc_;

    map_renderer::MapRenderer& mr_;
};
}