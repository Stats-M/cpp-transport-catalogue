/*
 * Назначение модуля: код, отвечающий за визуализацию карты маршрутов в формате SVG.
 */

#pragma once

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <string>
#include <string_view>
#include <vector>
#include <cmath>
#include <map>            // для входящих данных маршрутов
#include <unordered_set>  // для временного набора координат
//#include <unordered_map>  // альтенатива для временного набора координат
#include <algorithm>      // для std::minmax_element
#include <memory>         // для unique_ptr

namespace map_renderer
{

// Настройки рендеринга c дефолтными значениями
struct RendererSettings
{
    double width = 1200.0;
    double height = 1200.0;
    double padding = 50.0;
    double line_width = 14.0;
    double stop_radius = 5.0;
    int bus_label_font_size = 20;
    svg::Point bus_label_offset{ 7.0, 15.0 };
    int stop_label_font_size = 20;
    svg::Point stop_label_offset{ 7.0, -3.0 };
    svg::Color underlayer_color = svg::Rgba{ 255, 255, 255, 0.85 };
    double underlayer_width = 3.0;
    std::vector<svg::Color> color_palette{ std::string("green"), svg::Rgb{255, 160, 0}, std::string("red") };
};

// ---------------SphereProjector-------------------------

// Допуск проврки вещественных чисел на ноль
inline const double EPSILON = 1e-6;

// Проверяет вещественный аргумент на равенство нулю с установленным допуском
bool IsZero(const double);

// Класс, проецирующий координаты точек на карту (полотно SVG файла)
class SphereProjector
{
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width,
                    double max_height,
                    double padding)
        : padding_(padding)
    {
        // Если контейнер координат пуст - выходим
        if (points_begin == points_end)
        {
            return;
        }

        const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
                                  {
                                      return lhs.lng < rhs.lng;
                                  });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
                                  {
                                      return lhs.lat < rhs.lat;
                                  });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_))
        {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat))
        {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        // Если получены множители и для высоты, и для ширины - выбираем наименьший из них
        if (width_zoom && height_zoom)
        {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom)
        {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom)
        {
            zoom_coeff_ = *height_zoom;
        }
    }

    // Оператор() преобразования Coordinates к Point с учетом нормализации
    svg::Point operator()(geo::Coordinates) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};    //class SphereProjector

// -----Классы фигур, реализующие интерфейс svg::Drawable----

// Фигура: линия маршрута между точками остановок
class RouteLine : public svg::Drawable
{
public:
    RouteLine(const std::vector<svg::Point>&, const svg::Color&, const RendererSettings&);
    void Draw(svg::ObjectContainer&) const override;
private:
    std::vector<svg::Point> stop_points_;        // Вектор нормализованных координат линии
    svg::Color stroke_color_;                    // Цвет контура линии
    const RendererSettings& renderer_settings_;  // Настройки рендерера для параметров по-умолчанию
};

// Фигура: текстовая метка с тенью (двухслойный шрифт)
class TextLabel : public svg::Drawable
{
public:
    TextLabel(const svg::Point&, const std::string& text, const svg::Color&, const RendererSettings&, const bool& is_stop);
    void Draw(svg::ObjectContainer&) const override;
private:
    svg::Point label_point_;        // Нормализованные координаты метки
    std::string font_family_;       // Название шрифта
    std::string font_weight_;       // Толщина шрифта
    std::string text_;              // Текст метки
    svg::Color fill_fore_color_;    // Цвет текста переднего плана
    const RendererSettings& renderer_settings_;  // Настройки рендерера для параметров по-умолчанию
    bool is_stop_;                  // Флаг обработки метки остановки (при false - метка маршрута)
};

// Фигура: линия маршрута между точками остановок
class StopIcon : public svg::Drawable
{
public:
    StopIcon(const svg::Point&, const RendererSettings&);
    void Draw(svg::ObjectContainer&) const override;
private:
    svg::Point label_point_;                     // Нормализованные координаты иконки
    const RendererSettings& renderer_settings_;  // Настройки рендерера для параметров по-умолчанию
};

// -----------------MapRenderer-------------------------

// Алгоритм работы:
// 1. Создаем контейнер объектов svg::Drawable, например, vector<unique_ptr<svg::Drawable>>
// 2. Добавляем в него указатели на объекты-наследники Drawable
// 3. Вызываем DrawPicture() для отображения объектов через примитивы в svg::ObjectContainer, 
//    например, svg::Document
// 4. Можно добавить напрямую к Document дополнительные svg-примитивы (круг, полилиния, текст)
// ---- В текущей архитектуре на этом шаге Document возвращается в RequestHandler
// ---- и далее передается обратно в вызывавший метод в json_reader, который и выполняет шаг 5.
// 5. Вызываем Document.Render(outstream) для вывода текста документа в поток / string
class MapRenderer
{
public:
    // Метод применяет указанные настройки рендеринга карты
    void ApplyRendererSettings(RendererSettings);
    // SERIALIZER. Возвращает настройки рендеринга карты
    RendererSettings GetRendererSettings() const;

    // Метод добавляет данные линий маршрутов в SVG-файл
    void AddRouteLinesToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
                               SphereProjector& sp,
                               std::map<const std::string, transport_catalogue::RendererData>& routes_to_render);
    // Метод добавляет названия маршрутов в SVG-файл
    void AddRouteLabelsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
                                SphereProjector& sp,
                                std::map<const std::string, transport_catalogue::RendererData>& routes_to_render);
    // Метод добавляет названия остановок в SVG-файл
    void AddStopLabelsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
                               SphereProjector& sp,
                               std::map<std::string_view, geo::Coordinates> all_unique_stops);
    // Метод добавляет метки остановок в SVG-файл
    void AddStopIconsToRender(std::vector<std::unique_ptr<svg::Drawable>>& picture_,
                              SphereProjector& sp,
                              std::map<std::string_view, geo::Coordinates> all_unique_stops);

    // Метод формирует итоговый SVG-файл, вызывая методы рендера всех составных частей документа
    svg::Document RenderMap(std::map<const std::string, transport_catalogue::RendererData>&);


    // Производит отрисовку всех Drawable объектов в контейнере по итераторам
    template <typename DrawableIterator>
    void DrawPicture(DrawableIterator begin, DrawableIterator end, svg::ObjectContainer& target)
    {
        for (auto it = begin; it != end; ++it)
        {
            (*it)->Draw(target);
        }
    }

    // Производит отрисовку контейнера Drawable объектов в объект ObjectContainer, например, svg::Document
    template <typename Container>
    void DrawPicture(const Container& container, svg::ObjectContainer& target)
    {
        DrawPicture(begin(container), end(container), target);
    }

private:
    RendererSettings settings_;    // Настройки рендерера по-умолчанию
    size_t pallette_item_ = 0;     // Текущий элемент из палитры цветов

    // Возвращает следующий цвет из цветовой палитры
    const svg::Color GetColorFromPallete();
    // Сбрасывает счетчик элементов палитры
    void ResetPallette();

};    // class MapRenderer

}
