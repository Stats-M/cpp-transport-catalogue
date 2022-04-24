#pragma once

// Проверка валидности изображений SVG: https://www.svgviewer.dev/

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <string_view>
#include <algorithm>
#include <optional>
#include <variant>

namespace svg
{

// Цвет в формате RGB
class Rgb
{
public:
    Rgb() = default;

    Rgb(uint8_t red, uint8_t green, uint8_t blue) :
        red(red), green(green), blue(blue)
    {}

    ~Rgb() = default;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

// Цвет в формате RGBA
class Rgba
{
public:
    Rgba() = default;

    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity) :
        red(red), green(green), blue(blue), opacity(opacity)
    {}

    ~Rgba() = default;

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0L;  // G-умолчанию 100% непрозрачность
};


using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;

// Структура для вывода Color (тип Variant) через std::visit()
struct OstreamSolutionPrinter
{
    std::ostream& out;

    void operator()(std::monostate) const
    {}
    void operator()(std::string str) const
    {
        out << str;
    }
    void operator()(Rgb rgb) const
    {
        using namespace std::literals;
        out << "rgb("s << std::to_string(rgb.red) << ","s << std::to_string(rgb.green) << ","s << std::to_string(rgb.blue) << ")"s;
    }
    void operator()(Rgba rgba) const
    {
        using namespace std::literals;
        out << "rgba("s << std::to_string(rgba.red) << ","s << std::to_string(rgba.green) << ","s << std::to_string(rgba.blue) << ","s << rgba.opacity << ")"s;
    }
};

// Объявив в заголовочном файле константу со спецификатором inline,
// мы сделаем так, что она будет одной на все единицы трансляции,
// которые подключают этот заголовок.
// В противном случае каждая единица трансляции будет использовать свою копию этой константы
inline const Color NoneColor{ "none" };

// тип формы конца линии
enum class StrokeLineCap
{
    BUTT,
    ROUND,
    SQUARE,
};

// тип формы соединения линий
enum class StrokeLineJoin
{
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

// Оператор выводит текстовое представление формы конца линии в поток
std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap);

// Оператор выводит текстовое представление формы соединения линии в поток
std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join);

// Класс PathProps должен принимать шаблонный параметр Owner, который задаёт 
// тип класса, владеющего этими свойствами. Это дает поддержку method chaining, 
// возвращая ссылку на текущий экземпляр правильного типа. 
// Например, при вызове PathProps::SetFillColor у класса Circle должна возвращаться ссылка Circle&.
// Curiously Recurring Template Pattern (CRTP)
template <typename Owner>
class PathProps
{
public:
    // Задаёт значение свойства fill — цвет заливки. 
    // По умолчанию свойство не выводится.
    Owner& SetFillColor(Color color)
    {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    // Задаёт значение свойства stroke — цвет контура. 
    // По умолчанию свойство не выводится.
    Owner& SetStrokeColor(Color color)
    {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    // Задаёт значение свойства stroke-width — толщину линии. 
    // По умолчанию свойство не выводится.
    Owner& SetStrokeWidth(double width)
    {
        width_ = width;
        return AsOwner();
    }
    // Задаёт значение свойства stroke-linecap — тип формы конца линии.
    // По умолчанию свойство не выводится.
    Owner& SetStrokeLineCap(StrokeLineCap line_cap)
    {
        line_cap_ = line_cap;
        return AsOwner();
    }
    // Задаёт значение свойства stroke-linejoin — тип формы соединения линий. 
    // По умолчанию свойство не выводится.
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join)
    {
        line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const
    {
        using namespace std::literals;

        if (fill_color_)
        {
            // out << " fill=\""sv << *fill_color_ << "\""sv;  // Obsolete
            out << " fill=\""sv;
            visit(OstreamSolutionPrinter{ out }, *fill_color_);
            out << "\""sv;
        }
        if (stroke_color_)
        {
            // out << " stroke=\""sv << *stroke_color_ << "\""sv;  // Obsolete
            out << " stroke=\""sv;
            visit(OstreamSolutionPrinter{ out }, *stroke_color_);
            out << "\""sv;
        }
        if (width_)
        {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_)
        {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_)
        {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner()
    {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};


struct Point
{
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y)
    {}
    double x = 0.0;
    double y = 0.0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext
{
    RenderContext(std::ostream& out)
        : out(out)
    {}

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent)
    {}

    RenderContext Indented() const
    {
        return { out, indent_step, indent + indent_step };
    }

    void RenderIndent() const
    {
        for (int i = 0; i < indent * indent_step; ++i)
        {
            out.put(' ');
        }
    }

    std::ostream& out;
    //int indent_step = 4;
    int indent_step = 1;
    int indent = 0;
};


/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object
{
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};


// ---------- IObjectContainer ------------------

// ObjectContainer задаёт интерфейс для доступа к контейнеру SVG-объектов. 
// Через этот интерфейс Drawable-объекты могут визуализировать себя, добавляя 
// в контейнер SVG-примитивы. 
// Строго говоря, ObjectContainer — это абстрактный класс, а не интерфейс, так как 
// шаблонный метод Add, принимающий наследников Object по значению, не получится 
// сделать виртуальным: в C++ шаблонные методы не могут быть виртуальными.
class ObjectContainer
{
public:
    // Реализован на основе чисто виртуального метода ObjectContainer::AddPtr, 
    // принимающего unique_ptr<Object>&& .
    template <typename T>
    void Add(T obj)
    {
        AddPtr(std::make_unique<T>(obj));
    }

protected:
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

// ---------- IDrawable ------------------

// Интерфейс Drawable унифицирует работу с объектами, которые можно нарисовать, 
// подключив SVG-библиотеку. Для этого в нём есть метод Draw, принимающий ссылку 
// на интерфейс ObjectContainer.
class Drawable
{
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};


/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle>
{
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline>
{
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

    /*
     * Прочие методы и данные, необходимые для реализации элемента <polyline>
     */
private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text>
{
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

    // Прочие данные и методы, необходимые для реализации элемента <text>
private:
    std::string Encode(std::string data) const;
    void RenderObject(const RenderContext& context) const override;
    Point position_;
    Point offset_;
    uint32_t size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class Document : public ObjectContainer
{
public:

    Document() = default;

    /*
     Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
     Пример использования:
     Document doc;
     doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
    */
    template <typename Obj>
    void Add(Obj obj)
    {
        objects_.emplace_back(std::make_unique<Obj>(std::move(obj)));
    }

    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override
    {
        objects_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

    // Прочие методы и данные, необходимые для реализации класса Document
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg