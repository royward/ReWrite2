# ReWrite2

A rule-based language for expressing recursive computation through pattern matching. Initial version an interpreter in Modern C++, but the plan is to bootstrap it - write a ReWrite2 compiler in itself that generates instructions suitable for a Virtual Machine, and maybe eventually x86-64 and ARM machine code.

Note: ReWrite2 is a working name while I look for a better one.

## Overview

## License

ReWrite2 is licensed under the [Apache License 2.0](LICENSE).

Example programs (in the `tests/` directory) are released under [CC0 1.0 Universal (Public Domain)](https://creativecommons.org/publicdomain/zero/1.0/) — you are free to use them without restriction.

## History and Motivation

In the 1990s, I got really interested in using pattern matching for programming, partly inspired by Prolog (I know that is unification, not pattern matching), and partly inspired by Mathematica. This seemed particularly suited to logic where there are many cases with different specificity to deal with. I wrote a language called ReWrite that worked on Classic MacOS, initially writing a minimal interpreter in Pascal, then bootstrapped it to write a ReWrite -> 68000 compiler in ReWrite itself, then discarded the interpreter. This worked well - the source for the ReWrite -> 68000 compiler was about 100K of text (excluding some of the library stuff). I wanted to develop this into a generally useful language, but writing full library support was an enormous amount of work, so the project died on the vine. I briefly explored porting it to PowerPC and having a way to integrate it with C++, but this would have been totally dependent on CodeWarrior (the MacOS PowerPC development environment at the time). I also looked at compiling to Java Virtual Machine, but that lacks some of the features like tail recursion needed for a language like this.

This idea lay dormant until recently, when as part of my work on the [Galaxy Generator Project](https://orange-kiwi.com/galaxy-generator/) (now being developed in Rust), I realised that some of the logic in there is going to get very complex, and there would be value in having a way of having some of the logic in a ReWrite style language in a module embedded into Rust. This avoids the whole library issue of trying to write a general purpose language. A lot has changed in language design in the last 30 years, so this project is not the original ReWrite - I'm using a more modern syntax, have somewhat more powerful semantics.

Most importantly, the original ReWrite was dynamically typed (types checked at runtime), and this new project will be strongly statically typed (types checked at compile time), as that avoids runtime errors, and allows more optimal code generation.

As a secondary motivation, I was interested to see how well Modern C++ could be used to rapidly develop the initial interpreter. The interpreter was written in under four days of actual development time, suggesting the approach was successful.

## Use cases

## Usage

## Language description

(examples included in here)

## Road map

