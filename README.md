# ReWrite2

A rule-based language for expressing recursive computation through pattern matching. Initial version an interpreter in Modern C++, but the plan is to bootstrap it - write a ReWrite2 compiler in itself that generates instructions suitable for a Virtual Machine, and maybe eventually x86-64 and ARM machine code.

Note: ReWrite2 is a working name while I look for a better one.

## Overview

A ReWrite2 program is a list of functions, where each function is a set of rules, and the execution path is that a function call will try each of the rules, then fire the first one that matches (without Prolog style backtracking). For a simple example:

```
fact(0) -> 1;
fact(n) -> n*fact(n-1);
```

Each call will check first if the argument is 0, in which case it will terminate returning 1, or it will calculate `fact(n)` recursively in terms of `fact(n-1)`.

ReWrite2:
* is currently interpreted, with the goal of later being compiled.
* will be strongly typed - it is currently dynamically typed, but this will change in the compiled version.
* is mostly functional (does allow side effect for things like logging).
* uses patterns instead of if/then/else and case.
* uses tail recursion instead of loops.
* uses no garbage collector - everything is copied, modified in place or disposed of, so that there is exactly one reference to anything. This will be replaced by reference counting in the compiled version. Both of these techniques avoid garbage collection pauses and make memory management predictable.
* is intended to have the Rust/Haskell property that where possible things fail at parse or compile time rather than run time.

## License

ReWrite2 is licensed under the [Apache License 2.0](LICENSE).

Example programs (in the `tests/` directory) are released under [CC0 1.0 Universal (Public Domain)](https://creativecommons.org/publicdomain/zero/1.0/) — you are free to use them without restriction.

## History and Motivation

In the 1990s, I got really interested in using pattern matching for programming, partly inspired by Prolog (I know that is unification, not pattern matching), and partly inspired by Mathematica. This seemed particularly suited to logic where there are many cases with different specificity to deal with. I wrote a language called ReWrite that worked on Classic MacOS, initially writing a minimal interpreter in Pascal, then bootstrapped it to write a ReWrite -> 68000 compiler in ReWrite itself, then discarded the interpreter. This worked well - the source for the ReWrite -> 68000 compiler was about 100K of text (excluding some of the library stuff). I wanted to develop this into a generally useful language, but writing full library support was an enormous amount of work, so the project died on the vine. I briefly explored porting it to PowerPC and having a way to integrate it with C++, but this would have been totally dependent on CodeWarrior (the MacOS PowerPC development environment at the time). I also looked at compiling to Java Virtual Machine, but that lacks some of the features like tail recursion needed for a language like this. [Mercury](https://www.mercurylang.org/) (a strongly typed logic language) has been the language closest to filling this niche, but its built-in backtracking makes it less suited to rule-based dispatch where exactly one rule fires per call.

This idea lay dormant until recently, when as part of my work on the [Galaxy Generator Project](https://orange-kiwi.com/galaxy-generator/) (now being developed in Rust), I realised that some of the logic in there is going to get very complex, and there would be value in having a way of having some of the logic in a ReWrite style language in a module embedded into Rust. This avoids the whole library issue of trying to write a general purpose language. A lot has changed in language design in the last 30 years, so this project is not the original ReWrite - I'm using a more modern syntax, have somewhat more powerful semantics.

Most importantly, the original ReWrite was dynamically typed (types checked at runtime), and this new project will be strongly statically typed (types checked at compile time), as that avoids runtime errors, and allows more optimal code generation.

As a secondary motivation, I was interested to see how well Modern C++ could be used to rapidly develop the initial interpreter. The interpreter was written in under four days of actual development time, suggesting the approach was successful.

## Use cases

ReWrite2 can be used for general programming, but is particularly good at dealing with complex-case logic. For instance, this makes it well suited to lexers, parsers and compilers. I also think it is going to have value in complex game logic (I haven't tested this yet), and converting a data structure into readable text (for example, planet descriptions).

The example programs demonstrate recursive algorithms including prime generation and the n-queens problem, giving a sense of the language's expressive range beyond compiler construction.

It is designed to be embedded in other languages (currently C++, later will also support C and Rust), so the plan is to use it for the logic that goes alongside heavy numerical computation in the embedding language.


## Installation

ReWrite2 requires a C++23 compatible compiler (GCC 14+ or Clang 18+) and CMake 3.20+. It uses no platform-specific code and should work on any platform supporting these tools, but has only been tested on Linux.

```bash
git clone https://github.com/royward/rewrite2.git
cd rewrite2
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The binary will be at `build/release/rewrite_cpp`.

For a debug build:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

I welcome bug reports if there is some issue where it won't build on another platform with a recent enough C++ compiler.

Note that once ReWrite2 builds itself, compiling the VM will not require C++-23 - probably C++17 at most.

## Usage

ReWrite2 currently has a command line utility:

```
<path>/rewrite_cpp <program_file> <expression>
```

where `<expression>` is any valid ReWrite2 expression, which may call functions defined in `<program_file>`.

So for instance (from the `build/release` directory):

```
./rewrite_cpp ../../tests/factorial.rw "fact(10)"
Results:
3628800
```

It is also possible to just use the classes directly (just look at `main.cpp` for the code for this). A note of caution with doing this - the current version is the throwaway prototype, and the API to phase 2 will be quite different.

## Language description

This section has examples which can all be found in the `tests` directory.

#### General Structure

The structure of a ReWrite2 program is simple - it is a list of functions, where each function is a list of rules of the form:

```
<function_name>(<match parameters>)[::<optional guard expression>] -> <expressions>;
```

and the language just fires the first rule that successfully matches (it is a runtime error if none of them fire - later versions might make this a compile time error).

So for instance, in the example from earlier:

```
fact(0)->1;
fact(n)->n*fact(n-1);
```

If `fact(3)` is evaluated, it fails to match with the first rule, matches with the second which fires, so `n*fact(n-1)` is evaluated, which calls `fact(2)`.

When `fact(0)` is called, it will fire the first rule, but having succeeded, it will _not_ fire the second one - unlike Prolog or Mercury, there is no backtracking.

#### Guards

Guards are useful when the condition can't be expressed purely as a pattern match. They are expected to be an expression returning a single boolean.

An alternative version of `fact` using a guard is:

```
fact(n)::n>0 -> n*fact(n-1);
fact(_) -> 1;
```
The first rule will fire if `n` is matched (which will match to any single argument), but the rule will fire only if the guard condition is met: `n>0`, giving the same result as the first version (except it won't crash the stack like the first version would).

The `_` is a special symbol that means "match with anything". Unlike a named parameter, it doesn't bind to anything, so you can use multiple `_` in the same rule without conflict.

#### Pattern matching

So far, all the examples have treated function parameters like they would appear in most other languages. ReWrite2 does full matching, which is richer than this - the same identifer can appear as part of a match multiple times, but then it must be bound to the same value. For instance:

```
equal(x,x) -> true;
equal(_,_) -> false;
```

Will test for equality for the two parameters - if they are equal, the first rule will fire, otherwise the second one will.

#### Multiple return values

One unusual feature of ReWrite2 is that functions can return multiple values, and these will be taken as separate values when inserted into a list or another function call.

For instance, in the following, `min_max` returns two values, the min and the max:

```
min_max(a,b) :: a<b -> a,b;
min_max(a,b) -> b,a;
```

and this will be taken as two parameters by another function, for instance:

```
sub(a,b) -> b-a;
```
so `sub(min_max(10,3))` and `sub(min_max(3,10))` will both return `7`.

It's worth unpacking what is going on here: `min_max(10,3)` returns the results `3` (the min) and `10` (the max). The `3` and `10` are passed as two arguments to `sub(a,b)`, so `sub(3,10)` is evaluated.

One motivation for this (using features not yet in the language) was `polar_to_rectangular`:

```
// hypothetical - floating point not yet supported
polar_to_rectangular(r, theta) -> r*cos(theta), r*sin(theta);
```

It naturally returns multiple values, and while many other languages allow the result to be returned as a pair (or more generally, a tuple), it then needs to be unpacked, usually by assigning it to something and getting out indexed values. ReWrite2 makes this much more natural - if the parameter order matches up (if it doesn't, ReWrite2 would easily support a one line shim to fix this).

#### Lists and splat

ReWrite2 has support for constructing lists, using `{ }`, so `{1,2,3}` is the list containing the three elements, 1,2,3. Lists can also be used as part of pattern matching:

```
// not in test directory as more powerful example below
reverse3({x,y,z}) -> {z,y,x};
```
This will reverse a list of three elements. However this is not very general - we really want to reverse any sized list. To allow more powerful list processing, there is the splat `..` operator, which is effectively the inverse of the list operator. When used as part of a match, it means "take any number of elements and bind it to a list", when used as part of the expression on the right, it means "expand a list into its elements in the surrounding context", so `..{x}` = `x`, and {..list} = `list` (if `list` is a list).

Here is a more generic reverse list function:

```
reverse({}) -> {};
reverse({a,..rest}) -> {..reverse(rest),a};
```

Working through this, if `reverse({1,2,3})` is called, then `a` will bind to `1`, `rest` will bind to `{2,3}`. `reverse(rest)=reverse({2,3})` will return `{3,2}`, and the `..` will strip the brackets, so `{..{3,2},1}` = `{3,2,1}`. The first rule gives a terminating condition to the recursion.

A splat doesn't need to be at the end of a list. An example that takes all the elements of a list except the first and last:

```
middle({_,..x,_}) -> x;
```

So `middle({1,2,3,4})` returns `{2,3}`.

You might be tempted to use multiple splats:

```
// This won't work (yet)
member(x,{.._,x,.._}) -> true;
member(x,_) -> false;
```

This is conceptually nice, but the language doesn't (yet) support it - this is a minimal interpreter, and that would take a fair bit more machinery to get working.

The following will work:

```
member(_,{}) -> false;
member(x,{x,.._}) -> true;
member(x,{_,..rest}) -> member(x,rest);
```

As part of an expression, splat can deal with more than one argument (if they are all lists):

```
flatten(x) -> {.. ..x};
```

Calling `flatten({{1,2},{3,4}})` returns `{1,2,3,4}`. `..{{1,2},{3,4}}` gives the two results `{1,2}`,`{3,4}` and the other `..` expands each of them to `1`,`2` and `3`,`4`, which is enclosed in a list giving `{1,2,3,4}`

For another example using list and splat:

```
listn(0) -> {};
listn(n) -> {..listn(n-1),n-1};
```

`listn(3)` returns `{0,1,2}`. I leave working through this as an exercise for the reader.

#### Tail recursion

ReWrite2 uses a lot of recursion. One thing to be aware of is that each recursive call typically uses another frame on the stack, so deep recursions can cause a stack overflow. This is mitigated by tail recursion - a jump (tail call) is used on the last operation performed by a function, so in:

```
f(x) -> g(x),t(h(x))
```

the `g(x)` and `h(x)` will be called, creating another level on the stack, but when it gets to `t(x)`, that is done with a jump rather than a call, as there would be nothing left to do on return.

Code can always be made tail recursive with appropriate restructuring. For instance, the `listn` above is not tail recursive, but the following is:

```
listn2(n) -> listn_aux(0,n,{});

listn_aux(n,n,sofar) -> sofar;
listn_aux(n,m,sofar) -> listn_aux(n+1,m,{..sofar,n});
```

This is a little more complicated than `listn`, but avoids the stack growing with the size of `n`. It basically sets up an accumulator `sofar`, so that `listn_aux` effectively becomes a loop.

## More complex examples

Here are some examples that put these concepts together. I'm not going to work through them, but give some commentary.

#### First n prime numbers

The "find the nth prime number" was one of my milestone tests in ReWrite version 1, so it seemed fitting to include it here.

```
nprime(n) -> nprime_aux(n,2,{});

nprime_aux(0,_,sofar) -> sofar;
nprime_aux(n,p,sofar)::isprime(p,sofar) -> nprime_aux(n-1,p+1,{..sofar,p});
nprime_aux(n,p,sofar) -> nprime_aux(n,p+1,sofar);

isprime(_,{}) -> true;
isprime(n,{i,.._})::i*i>n -> true;
isprime(n,{i,.._})::n%i==0 -> false;
isprime(n,{_,..rest}) -> isprime(n,rest);
```

`nprime_aux` is effectively a loop, trying increasing numbers one at a time against the list of primes found so far.

`isprime` is a good exemplar showing how rule matching can make code clear. There is a terminating condition ("we tried them all") first rule, another rule for exiting early, another one for "this is not prime", and lastly, "we tested this number, go try the next one". I sometimes think of it as "one thought = one line".

#### N-Queens

This would not be complete without an example that is computationally more heavy. I've played a lot with Prolog and Prolog like languages, so the N-Queens problem is my selection here - "how can I place N queens on an NxN chessboard so that none of them take each other". This code returns all the solutions, as a list of which column each queen should go on for the corresponding row.

```
nqueens(n) -> nqueens_aux({},listn1(n),{});

listn1(0) -> {};
listn1(n) -> {..listn1(n-1),n};

// nqueens_aux(column_choices_sofar, column_choices_todo, rows_done)
nqueens_aux({},{},solution) -> {solution};
nqueens_aux(_,{},solution) -> {};
nqueens_aux(col_so_far,{c,..col_todo},rows_done) ->
    {..nqueens_try(col_so_far,col_todo,c,rows_done),
     ..nqueens_aux({..col_so_far,c},col_todo,rows_done)};

nqueens_try(col_so_far,col_todo,c,rows_done)::attack(c,1,rows_done) -> {}; // no solutions here
nqueens_try(col_so_far,col_todo,c,rows_done) -> nqueens_aux({},{..col_so_far,..col_todo},{c,..rows_done}); // got another row

attack(c,offset,{}) -> false;
attack(c,offset,{x,..rest}) :: c+offset==x || c-offset==x -> true;
attack(c,offset,{_,..rest}) -> attack(c,offset+1,rest);
```

Things of note:
* `nqueens` returns a list of solutions, where each solution is a list of numbers.
* This explores all branches of the tree, using the `{..nqueens_try( ),..nqueens_aux( )}` code - `nqueens_try` tries the selected column `c` and then `nqueens_aux( )` recursively tries the remaining columns. Also the `{..x,..y}` is the pattern for joining two lists together.
* `col_so_far` and `col_todo` between them are the columns not filled yet.

There is also a variant of this `nqueens_bitmask.rw` in the `tests` directory, that does the same thing, but using bitmasks for the `col_so_far` and `col_todo` instead of lists. In that example, `count_trailing_zeros` is an operator that will count the number of trailing zeros in the binary form of a number.

## Road map

ReWrite2 development will occur in phases:

#### Phase 1: minimal interpreter

This is mostly done - the only things left to complete this are:

* char/string support
* constants
* additional library functions required by the phase 2 compiler

#### Phase 2: compiling to VM

This stage does not involve adding language features, but writing a ReWrite2 program that compiles itself to a VM targetted binary. Steps are:

* write ReWrite2 -> VM compiler, written in ReWrite2 itself (self-hosting)
* write the VM (probably in C++ or C - this will be fairly low level)

#### Phase 3+: strong typing and other features

Once ReWrite2 can build and run itself, the interpreter is no longer necessary, so other features can be added:

* Interface from C, C++ and Rust
* Strong static typing (types checked at compile time)
* Structs
* Better code optimization
* More language features and libraries as useful.
  
