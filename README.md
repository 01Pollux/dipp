# dipp

dipp is a lightweight and flexible dependency injection library for C++ 20 and later. It provides a simple and efficient way to manage dependencies in your applications, promoting better code organization and testability.

dipp is a header-only library inspired by the .NET [`Microsoft.Extensions.Dependency.Injection`](https://learn.microsoft.com/en-us/dotnet/core/extensions/dependency-injection) framework.

## Overview

Here's a quick demo to show usage of this library. This is some basic usage of the library with two user classes.

```cpp
#include <dipp/dipp.hpp>
#include <cassert>

// We define a normal class with our logic
struct Window
{
};

// we define the service with the class, lifetime and optionally the scope and key identifier for unique services
// the service will be injected as a singleton, meaning that it will be created once and shared across all consumers
using WindowService = dipp::injected<Window, dipp::service_lifetime::singleton>;

struct Engine
{
    Window& window; // singleton window
    
    Engine(std::reference_wrapper<Window> window) : window1(window)
    {
    }
};

// Similarly, the engine will be injected as a scoped service, meaning that it will be created once per scope
using EngineService = dipp::injected<Engine, dipp::service_lifetime::scoped, dipp::dependency<WindowService>>;

int main()
{
    // create a collection to hold our services
    dipp::default_service_collection collection;

    // add the services to the collection
    collection.add<WindowService>();
    collection.add<EngineService>();

    // create a service provider with the collection
    dipp::default_service_provider services(std::move(collection));

    // get the engine service
    // the engine service will create a window service and inject it into the engine
    // if the scope is at the root level, the engine will be treated as a singleton
    auto engine = services.get<EngineService>();

    // create a scope
    auto scope = services.create_scope();

    // get the engine service from the scope
    // the engine will be destroyed when the scope is destroyed
    auto engine2 = scope.get<EngineService>();

    // get the window service from the scope
    auto window = scope.get<WindowService>();

    // since the window is a singleton, the window from the scope should be the same as the window from the engine
    assert(&engine->window == &engine2->window);

    // and the engine from the scope should be different from the engine from the root scope
    assert(engine.ptr() != engine2.ptr());

    return 0;
}
```

## Keyed Services

Keyed services are services that are registered with a unique key identifier. This allows you to have multiple services of the same type but with different implementations. Here's an example of how to use keyed services.

```cpp
#include <dipp/dipp.hpp>
#include <cassert>

// We define a normal class with our logic
struct Window
{
};

// we define the service with the class, lifetime and optionally the scope and key identifier for unique services
// the service will be injected as a singleton, meaning that it will be created once and shared across all consumers
using WindowService1 = dipp::injected<Window, dipp::service_lifetime::singleton>;
using WindowService2 = dipp::injected<Window, dipp::service_lifetime::singleton, dipp::dependency<>, dipp::key("UNIQUE")>;
static_assert(dipp::base_injected_type<WindowService1>);
static_assert(dipp::base_injected_type<WindowService2>);

struct Engine
{
    Window& window1; // singleton window
    Window& window2; // singleton window

    Engine(std::reference_wrapper<Window> window1,
           std::reference_wrapper<Window> window2) :
           window1(window1),
           window2(window2)
    {
    }
};

using EngineService =
    dipp::injected<Engine, dipp::service_lifetime::scoped, dipp::dependency<WindowService1, WindowService2>>;

int mai2()
{
    // create a collection to hold our services
    dipp::default_service_collection collection;

    // add the services to the collection
    collection.add<WindowService1>();
    collection.add<WindowService2>();
    collection.add<EngineService>();

    // create a service provider with the collection
    dipp::default_service_provider services(std::move(collection));

    // get the engine service
    // the engine service will create a window service and inject it into the engine
    // if the scope is at the root level, the engine will be treated as a singleton
    auto engine = services.get<EngineService>();

    // get the window service from the engine
    auto& window1 = engine->window1;
    auto& window2 = engine->window2;

    // both window services shouldn't be the same
    assert(window1.ptr() != window2.ptr());

    return 0;
}
```

## Features

* Explicit, you control the lifetime, key and storage of your services.
* No auto-registration, you must register your services explicitly.
* Clean and simple API for simple cases, flexible enough for complex cases
* Header only library
* Clean diagnostics at compile-time.
* Extensible and flexible to define your own service storage.
* Non intrusive, you can use it with your existing classes.

## Requirements

* XMake 2.9.6 or later
* C++ 20 compiler
* Boost.Test (optional, for tests), will be installed automatically by xmake

## Build the project

```bash
$ xmake build
$ xmake install -o install
```


## Run the tests

```bash
$ xmake f --no-test=n # to enable/disable tests
$ xmake run <test_name> # tests found in project/test.lua
```


## Benchmark

Benchmarks were done using the [Google Benchmark library](https://github.com/google/benchmark). The benchmarks were run on a Windows 11 machine with the following specs:

Run on (8 X 2208 MHz CPU s)

- CPU Caches:
    - L1 Data 32 KiB (x4)
    - L1 Instruction 32 KiB (x4)
    - L2 Unified 256 KiB (x4)
    - L3 Unified 6144 KiB (x1)

| Benchmark                    | Time             | CPU           | Iterations | Library                                         |
|------------------------------|------------------|---------------|------------| ----------------------------------------------- |
| BM_FruitContainerCreation    | 18159 ns         | 10010 ns      | 64000      | [Google Fruit](https://github.com/google/fruit) | 
| BM_FruitResolution           | 5.2015e+12 ns    | 781250000 ns  | 1          | [Google Fruit](https://github.com/google/fruit) |
| BM_KangaruContainerCreation  | 15125 ns         | 10619 ns      | 64743      | [Kangaru](https://github.com/gracicot/kangaru)  |
| BM_KangaruResolution         | 5.2036e+12 ns    | 2125000000 ns | 1          | [Kangaru](https://github.com/gracicot/kangaru)  |
| BM_DippContainerCreation     | 1559 ns          | 1360 ns       | 448000     | [dipp](#)                                       |
| BM_DippResolution            | 5.2045e+12 ns    | 2890625000 ns | 1          | [dipp](#)                                       |


## Acknowledgements

Inspired by:
* the .NET [`Microsoft.Extensions.DependencyInjection`](https://learn.microsoft.com/en-us/dotnet/core/extensions/dependency-injection) framework
* the [`kangaru`](https://github.com/gracicot/kangaru.git) library
* the [`Google Fruit`](https://github.com/google/fruit) library
* the [`Google Benchmark`](https://github.com/google/benchmark) framework
