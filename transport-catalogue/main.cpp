#include "request_handler.h"    // "�����" ������������� �����������
#include "json_reader.h"        // ������ � ������� � ������� json
#include "map_renderer.h"

#include <iostream>             // ��� std::cin (isteam) � std::cout (osteam)

// �������� ������������ / ������� / ��������� �������:
// "<input.json 1>stdout.txt"

int main()
{
    /*
     * ��������� ��������� ���������:
     *
     * ������� JSON �� stdin
     * ��������� �� ��� ������ JSON ���� ������ ������������� �����������
     * ��������� ������� � �����������, ����������� � ������� "stat_requests", �������� JSON-������
     * � ��������.
     * ������� � stdout ������ � ���� JSON
     */

    // ������� ����������
    transport_catalogue::TransportCatalogue tc;
    // ������� �������� ����
    map_renderer::MapRenderer mr;
    // ������� ���������� �������� ��� �����������
    transport_catalogue::RequestHandler rh(tc, mr);
    // �������� ���������� JSON � ���������� �����������-��������
    json_reader::ProcessJSON(tc, rh, mr, std::cin, std::cout);

    //TODO ����� �������� �������� RequestHandler ����������� JSON, �.�. ������
    //     ����� ���� RH �� �����.
}