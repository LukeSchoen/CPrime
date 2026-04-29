# CPrime Compiler - [Download Windows Zip](https://github.com/LukeSchoen/CPrime/archive/refs/heads/main.zip)

## Whats's CPrime?

cPrime is a new programming language.
It's a serious attempt to extract the *useful core* of C++ & relayer it onto C;
without reinheriting c++'s historical-baggage, complexity, and slow compilation.

---

## Core Properties

- Compiles **any C program** (including itself)
- Can compile **many C++ files**
- Can be compiled with **any C++ compiler**
- Runtime behavior matches C++ expectations

---

## The Problem

C is:
- simple
- fast
- predictable

But:
- lacks structure
- scales poorly

C++ solved this with:
- classes
- abstraction
- encapsulation

But introduced:
- slow compile times
- massive toolchains
- legacy complexity
- no real feature deprecation

Modern C++ compilers (LLVM / MSVC) can be **20–200× slower** than C compilers like TCC/LCC.

---

## The Goal

CPrime sits in the middle:

- Keep **C’s simplicity and speed**
- Adds **just enough c++ structure**

---

## The Repo

Currently Includes:
- Self-hoisting compiler
- Windows `build` demo
- release.exe + Source

## The History

- Built in ~2 days using > 300 requests to gpt5.5  
- Already used to port multiple 3D projects  
- Compile speeds are dramatically faster

## The Plan

- Focus: core language + compile speed
- Essentially a **clean-room partial reimplementation of C++ on top of C**
- Early, but already usable

---

## Contributing

reports, experiments, Ideas are all welcome  
Use the issues tab

---

# Language Features

---

## Classes

```c
class Point
{
  int x;
  int y;

  void translate(int dx int dy)
  {
    this->x += dx;
    this->y += dy;
  }
};
```

- class is a struct alias  
- supports access modifiers  
- typedef boilerplate is optional  

---

## Methods

Supported syntax

- obj.fn(arg1)  
- ptr->fn(arg1, arg2)  
- const correct calls  
- name mangling for overloads  

---

## Operators

```c
class Num
{
  int value;
  int operator+(int rhs);
};

int Num::operator+(int rhs)
{
  return this->value + rhs;
}
```

Supported

- arithmetic + - * / %  
- comparisons  
- shifts  
- indexing []  
- assignment variants +=  

---

## Constructors / Destructors

```c
class Widget
{
  int value;
  Widget(int seed);
};

Widget::Widget(int seed)
{
  this->value = seed;
}

class Guard {
  int value;
  ~Guard();
};

Guard::~Guard()
{
  cleanup_total += this->value;
}
```

- automatic lifetime handling  
- standard C++ syntax  

---

## Templates

```c
template<typename T>
T id(T value)
{
  return value;
}

template<typename T>
class List {
  T items[8];
  int count;

  void push(T value)
  {
    this->items[this->count++] = value;
  }
};
```

- single type parameter (intentional)  
- supports nesting: Map<List<String>>  

---

## Separation of Declaration / Definition

```c
class Camera
{
  int value;
  void init(int v);
};

void Camera::init(int v)
{
  this->value = v;
}
```

- clean interface / implementation split  
- no loss of C compatibility  

---

## Overloading

```c
struct Accumulator
{
  int value;

  void set() { this->value = 5; }
  void set(int v) { this->value = v; }
  void set(float v) { this->value = (int)(v * 10.0f); }
};
```

- overload resolution via mangling  
- templates optional  

---

# Motivation

Using C++ instead of C genuinely costs the project important resources yet it only helps the dev while they are deving.

- complex setup  
- larger binaries  
- heavier toolchains  
- slower compile times  

The historical tradeoff to use c++ anyway has become undefensible under the rising weight of humandev-less AI-projects.
Abstractions that only benefits developers at compile time must become free if we are to be able to justify using them.
Thankfully object orientation helps AI's as much as it helps humans so the gains appear for humans and AI agents alike. 
---

# Final Note

Huge respect to the man, Bjarne Stroustrup  
C++ has been the in my hand for most days of my life

CPrime is about returning to

- fast & lightweight
- Prime Productivity

while keeping what actually matters ~(templatable O.O.P. for scaling and customizable operators for expressing)

---

# License

Demo / test purposes for now  
Must remain GPLv3-compatible  

You may fork under GPL  

---

## Credits

Compiler core derived from  
https://bellard.org/tcc/

Transitioning toward a clean-room rewrite  
next-big goal: a public-domain release
