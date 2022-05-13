#include "json_builder.h"

namespace json
{

//Builder::Builder()
//{}

/*
При определении словаря задаёт строковое значение ключа для очередной пары
ключ-значение. Следующий вызов метода обязательно должен задавать
соответствующее этому ключу значение с помощью метода Value или начинать
его определение с помощью StartDict или StartArray.
*/
KeyContext Builder::Key(std::string key)
{
    // Если это первый элемент в документе (root еще пуст)
    if (root_ == nullptr)
    {
        throw std::logic_error("Key() called for empty document.");
    }
    else if (nodes_stack_.empty())
    {
        // Ошибочный случай Key() при пустом стеке
        throw std::logic_error("Key() called outside of any container element.");
    }

    // Это не первый элемент документа.
    // Проверяем какого типа текущий контейнер
    Node* parent_container = nodes_stack_.back();
    if (parent_container->IsDict())
    {
        if (!key_opened_)
        {
            // Запоминаем ключ
            key_.swap(key);

            // Добавляем новый ключ с пустым Value в словарь
            const_cast<Dict&>(parent_container->AsDict())[key_] = Node();
            key_opened_ = true;
        }
        else
        {
            // Ошибочный случай Key().Key()...
            throw std::logic_error("Key() called for a Dict with already setted Key. Should call Value()");
        }
    }
    else
    {
        // Ошибочный случай Key() для НЕ словаря
        throw std::logic_error("Key() called for a container other than Dict.");
    }

    return KeyContext{ *this };
}


/*
Задаёт значение, соответствующее ключу при определении словаря, очередной
элемент массива или, если вызвать сразу после конструктора json::Builder,
всё содержимое конструируемого JSON-объекта.
Может принимать как простой объект — число или строку — так и целый массив
или словарь.
Здесь Node::Value — это синоним для базового класса Node,
шаблона variant с набором возможных типов-значений.
*/
BaseContext Builder::Value(Node::Value val)
{

    // Если это первый элемент в документе (root еще пуст)...
    if (root_ == nullptr)
    {
        // ...то контейнеров в стеке вызовов нет и val содержит все содержимое JSON-объекта.
        root_.GetValue() = std::move(val);  // variant сам знает что записать из другого variant
        return BaseContext{ *this };
    }

    // Проверяем ошибочный случай Builder{}.Value().Value()... (root_ не nullptr и пустой стек)
    if (nodes_stack_.empty())
    {
        throw std::logic_error("Value() called at wrong order (possibly, several Value() calls in a row for non-container).");
    }

    // Если мы здесь, то это не первый элемент документа.

    // Проверяем какого типа текущий контейнер
    Node* current_container = nodes_stack_.back();
    if (current_container->IsArray())
    {
        // Текущий контейнер - массив

        // Добавляем значение в текущий массив
        const_cast<Array&>(current_container->AsArray()).push_back(std::move(val));
    }
    else if (current_container->IsDict())
    {
        // Текущий контейнер - словарь
        if (key_opened_)
        {
            // Родительский контейнер - словарь и ключ не пуст. Текущее значение val - значение пары словаряя

            // Добавляем val в пару Key-Value текущего словаря для текущего ключа key_
            const_cast<Dict&>(current_container->AsDict())[key_] = std::move(val);
            // Ключ получил соответствующее ему значение.
            key_.clear();
            key_opened_ = false;
        }
        else
        {
            throw std::logic_error("Value() called for Dict' Key field, not for Value field as intended.");
        }
    }
    else
    {
        throw std::logic_error("Value() called for unknown parent container.");
    }

    return BaseContext(*this);
}

/*
Начинает определение сложного значения-словаря.
Вызывается в тех же контекстах, что и Value.
Следующим вызовом обязательно должен быть Key или EndDict.
*/
DictItemContext Builder::StartDict()
{

    // Если это первый элемент в документе (root еще пуст)
    if (root_ == nullptr)
    {
        root_ = Dict();               // Создаем пустой массив в root_
        nodes_stack_.emplace_back(&root_);  // Запомнаем &root_ в качестве начала стека
        return DictItemContext{ *this };
    }

    // Возможная ошибка - UB при пустом стеке и ненулевом root_
    // Это возможно при ошибочном вызове Start*() после Value()
    if (nodes_stack_.empty())
    {
        throw std::logic_error("StartDict() called at wrong order.");
    }

    // Это не первый элемент документа.
    // Проверяем каким был предыдущий (родительский) контейнер
    Node* parent_container = nodes_stack_.back();
    if (parent_container->IsArray())
    {
        // Родительский контейнер - массив. У нас случай нового словаря внутри массива

        // Добавляем новый пустой словарь в родительский массив
        const_cast<Array&>(parent_container->AsArray()).push_back(Dict());
        // Получаем указатель на добавленный новый узел
        Node* node = &const_cast<Array&>(parent_container->AsArray()).back();
        // Сохраняем указатель в стеке
        nodes_stack_.push_back(node);
    }
    else if (parent_container->IsDict())
    {
        if (key_opened_)
        {
            // Родительский контейнер - словарь и ключ не пуст. У нас случай нового словаря в качестве значения пары родительского словаря

            // Добавляем новый пустой словарь в родительский словарь для ключа key_
            const_cast<Dict&>(parent_container->AsDict())[key_] = Dict();
            // Получаем указатель на добавленный новый узел
            Node* node = &const_cast<Dict&>(parent_container->AsDict()).at(key_);
            // Сохраняем указатель в стеке
            nodes_stack_.push_back(node);
            // Ключ получил соответствующее ему значение.
            key_.clear();
            key_opened_ = false;
        }
        else
        {
            throw std::logic_error("StartDict() called for Key element, not for Value as intended.");
        }
    }
    else
    {
        throw std::logic_error("StartDict() called not for empty document, nor for Array element / Dict Value element.");
    }


    return DictItemContext{ *this };
}

/*
 Начинает определение сложного значения-массива. Вызывается в тех же контекстах,
 что и Value. Следующим вызовом обязательно должен быть EndArray или любой,
 задающий новое значение: Value, StartDict или StartArray.
*/
ArrayItemContext Builder::StartArray()
{
    /*
    Если StartArray, то варианта 3:
    1. Этот массив первый и корневой элемент, тогда нужно в root_ положить ноду с
       пустым массивом и адрес этой ноды добавить в стек.
    2. Предыдущий элемент - массив (nodes_stack_.back()->IsArray()). Тогда нужно
       получить ссылку на последний элемент nodes_stack_ (например, через get<Array>
       и неконстантный GetValue()), добавить в массив по этой ссылке ноду с пустым
       массивом и положить в стек адрес только что добавленной ноды.
    3. Предыдущий элемент - указатель на значение словаря (проверка nodes_stack_back()->IsNull()).
       Тогда просто в последний элемент стека записываем ноду с пустым массивом.
    */

    // Если это первый элемент в документе (root еще пуст)
    if (root_ == nullptr)
    {
        root_ = Array();                     // Создаем пустой массив в root_
        nodes_stack_.emplace_back(&root_);   // Запомнаем &root_ в качестве начала стека
        return ArrayItemContext{ *this };
    }

    // Возможная ошибка - UB при пустом стеке и ненулевом root_
    // Это возможно при ошибочном вызове Start*() после Value()
    if (nodes_stack_.empty())
    {
        throw std::logic_error("StartArray() called at wrong order.");
    }

    // Это не первый элемент документа.
    // Проверяем каким был предыдущий (родительский) контейнер
    Node* parent_container = nodes_stack_.back();
    if (parent_container->IsArray())
    {
        // Родительский контейнер - массив. У нас случай нового массива внутри массива

        // Добавляем новый пустой массив в родительский массив
        const_cast<Array&>(parent_container->AsArray()).push_back(Array());
        // Получаем указатель на добавленный новый узел
        Node* node = &const_cast<Array&>(parent_container->AsArray()).back();
        // Сохраняем указатель в стеке
        nodes_stack_.push_back(node);
    }
    else if (parent_container->IsDict())
    {
        if (key_opened_)
        {
            // Родительский контейнер - словарь и ключ не пуст. У нас случай нового массива в качестве значения пары родительского словаря

            // Добавляем новый пустой массив в родительский словарь для ключа key_
            const_cast<Dict&>(parent_container->AsDict())[key_] = Array();
            // Получаем указатель на добавленный новый узел
            Node* node = &const_cast<Dict&>(parent_container->AsDict()).at(key_);
            // Сохраняем указатель в стеке
            nodes_stack_.push_back(node);
            // Ключ получил соответствующее ему значение.
            key_.clear();
            key_opened_ = false;
        }
        else
        {
            throw std::logic_error("StartArray() called for Key element, not for Value.");
        }
    }
    else
    {
        throw std::logic_error("StartArray() called at wrong order.");
    }

    return ArrayItemContext{ *this };
}

/*
Завершает определение сложного значения-словаря.
Последним незавершённым вызовом Start* должен быть StartDict.
*/
BaseContext Builder::EndDict()
{

    // Стек должен быть НЕ пуст && текущий контейнер - словарь && ключ словаря пуст (пара была создана)
    if ((!nodes_stack_.empty()) && (nodes_stack_.back()->IsDict()) && (!key_opened_))
    {
        // Удаляем последний элемент стека, -1 уровень вложенности
        nodes_stack_.pop_back();
    }
    else
    {
        // Все остальные случаи невалидны
        throw std::logic_error("EndDict() called at wrong order.");
    }

    return BaseContext{ *this };
}

/*
Завершает определение сложного значения-массива.
Последним незавершённым вызовом Start* должен быть StartArray
*/
BaseContext Builder::EndArray()
{

    // Стек должен быть НЕ пуст && текущий контейнер - массив
    if ((!nodes_stack_.empty()) && (nodes_stack_.back()->IsArray()))
    {
        // Удаляем последний элемент стека, -1 уровень вложенности
        nodes_stack_.pop_back();
    }
    else
    {
        // Все остальные случаи невалидны
        throw std::logic_error("EndArray() called at wrong order.");
    }

    return BaseContext{ *this };
}

json::Node Builder::Build()
{

    if (root_ == nullptr)
    {
        // Документ пуст
        throw std::logic_error("Build() called for empty document.");
    }
    else if (!nodes_stack_.empty())
    {
        // Стек еще не пуст (документ не построен до конца)
        throw std::logic_error("Build() called for not finished document.");
    }

    return std::move(root_);
}



// КОНТЕКСТЫ ДЛЯ ВОЗВРАТА ИЗ МЕТОДОВ

KeyContext BaseContext::Key(std::string str)
{
    return builder_.Key(std::move(str));
}
BaseContext BaseContext::Value(Node::Value val)
{
    return builder_.Value(val);
}
DictItemContext BaseContext::StartDict()
{
    return builder_.StartDict();
}
ArrayItemContext BaseContext::StartArray()
{
    return builder_.StartArray();
}
BaseContext BaseContext::EndDict()
{
    return builder_.EndDict();
}
BaseContext BaseContext::EndArray()
{
    return builder_.EndArray();
}
json::Node BaseContext::Build()
{
    return builder_.Build();
}


KeyValueContext KeyContext::Value(Node::Value val)
{
    return BaseContext::Value(std::move(val));
}
ArrayItemContext KeyContext::StartArray()
{
    return BaseContext::StartArray();
}
DictItemContext KeyContext::StartDict()
{
    return BaseContext::StartDict();
}



ArrayValueContext ArrayItemContext::Value(Node::Value val)
{
    return BaseContext::Value(std::move(val));
}
DictItemContext ArrayItemContext::StartDict()
{
    return BaseContext::StartDict();
}
ArrayItemContext ArrayItemContext::StartArray()
{
    return BaseContext::StartArray();
}



KeyContext DictItemContext::Key(std::string key)
{
    return BaseContext::Key(key);
}


KeyContext KeyValueContext::Key(std::string key)
{
    return BaseContext::Key(key);
}


ArrayValueContext ArrayValueContext::Value(Node::Value val)
{
    return BaseContext::Value(std::move(val));
}
DictItemContext ArrayValueContext::StartDict()
{
    return BaseContext::StartDict();
}
ArrayItemContext ArrayValueContext::StartArray()
{
    return BaseContext::StartArray();
}
BaseContext ArrayValueContext::EndArray()
{
    return BaseContext::EndArray();
}

}