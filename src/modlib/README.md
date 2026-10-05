# `modlib` -- плагины через .so-файлы
_Писал Дидык Иван_

Данная либа реализует плагины с помощью динамической загрузки `.so` файлов.
Подход основывается на COM.

## Как писать плагины

Простейший плагин выглядит как:

```cpp
#include "modlib_mod.hpp"

/** Плагин, который просто существует */
class MyMod : public Mod {
public:
    /** УНИКАЛЬНЫЙ идентефикатор мода. НЕ что-нибудь в стиле "My Plugin", для этого есть brief() */
    virtual std::string_view id() const { return "author.thing.subthing.mymod"; };

    /** Тут уже можно название */
    virtual std::string_view brief() const { return "" } 

    /** Версия модуля */
    virtual ModVersion version() const  { return ModVersion(0, 0, 1); }
};
```

Также есть `onResolveDeps()` и другие виртуальные методы, гляньте исходник.
Если при инициализации нужных зависимостей нет, то кидайте `ModManager::Error`.

Если плагин реализует какую-то полезную функциональность, то он наследуется от
соответствующего интерфейса и реализует виртуельные методы в нём.

Или другой вариант: плагин находит плагин, который менеджит эту функциональность
и регистрируется у него коллбэками. Но этот вариант хуже.

Интерфейсы модулей можете глянуть в [`../modlib-ifc/`](../modlib-ifc/)

Чтобы скомпилить свой модуль с помощью CMake:

```cmake
add_library(mymod MODULE mymod.cpp)
target_link_libraries(mymod PRIVATE modlib)
# Называйте модули в соответствии с id(), чтобы их можно было легко искать!
set_target_properties(mymod PROPERTIES OUTPUT_NAME "author.thing.subthing.mymod")
set_target_properties(mymod PROPERTIES PREFIX "")
set_target_properties(mymod PROPERTIES SUFFIX ".mod")
```

## Как загружать плагины

Для этого есть `ModManager`.

```cpp
#include "modlib_manager.hpp"

int main(...) {
    ModManager mgr;

    // Грузит все файлы с расширением .mod
    // Лучше парсите аргументы коммандной строки,
    // или читайте env или рассчитывайте относительно
    // исполняемого файла, чтобы получать эту папку
    mgr.loadAllFromDir("./mods/"); 

    // Все модули загружены (возможно из нескольких разных
    // локаций), пусть инициализуруются (в том числе
    // разберутся с зависимостями).
    try { 
        mgr.initLoaded();
    } catch (const ModManager::Error &err) {
        // Кто-то недоволен, выводим err.what(), возможно крашимся
    }

    // Ищем плагины, что-нибудь делаем.
    // Полный список функций смотрите в заголовочнике
    GameMap *map = mgr.anyOfType<GameMap>();
    map.doSomething();
} 
```

## История

История данной либы такова: сначала она была написана для нашего
прошлого группового проекта [`mipt-ded-zemax` (по сути редактор сцены с рейтрейсером)](https://github.com/random-username-here/mipt-ded-zemax),
потом я её чуть доработал для своих целей, теперь она снова доработана
в групповом проекте.

