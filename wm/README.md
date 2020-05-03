# What is the DPAW WM

## Goals / Vision

DPAW is envisioned to become a fully fledged desktop environment at some point.
The DPAW WM is the window manager portion of it. It's short term goal is to become
an easy to use X11 window manager for mobile systems. Long-term, it should become
usable on different kinds of workspaces, such as desktops for example, too. It
is intended to allow for differend kinds of window management depending on the
kind of workspace used at some point.

## Is it ready to be used yet

It can be used on devices with touch screens already, and should be fairly stable.
It isn't really ready for daily usage yet, though. Currently, it can only place
a panel/keyboard/dock at the top/bottom of the screen and switch between or close
fullscreen windows using touch gestures. Without a touchscreen, it's currently
pretty much unusable.

## Why is it called DPAW

Originally, DPAW used to be only a window manager.
It's name is a german acronym for *Daniel Patrick Abrecht's Windowmanager*, because this
is the window manager originally created by daniel abrecht. DPAW is a german acronym, it
was chowsen over DPA WM because DPAW flows easier off the tongue, which makes it easier
to talk about it, and it would be kind of funny if people started talking about `the paw`,
an approximate homophone, in normal conversations.

## Non-goals

This project tries to stay independent of other big desktop environments or gui frameworks
such as gnome and kde/qt to the extent possible. If a functionality requires usage of
such a framework or similarely common or invasive software, it'll be put into an optional
program.


# Developer notes

## Programming language

This project is fully written in C. There are currently no plans to change that in any way.
There are benefitts and some drawbacks to this. The reasons for using C are:
* Multiple independent free software compilers are available for C
* C is a fairly stable language which won't change much anymore
* The future of C isn't at the mercy of any big company or other kind of group
* It has a very concise set of program constructs. Enough to structure data and programs,
  but not so much as to have countless features which allow the same thing to be done in yet
  another equivalent but different way. The absence of inheritance and class methods aren't
  necessary a drawback, composition and normal functions often, but not always, provide an easier,
  more flexible and less ambigouse aproache for solving most problems. Although there certainly
  are some fearures which would have been nice to have.
* There is no garbage collection, no overly complex runtime, it won't do things it isn't explicitly told to do.
* The runtime it does have is stable and has a solid ABI.
* Programs written in C are less likely to recursively pull in tons of dependencies, there is no central/standard
  repository for it on which everyone relies on.
* Building c programs doesn't require complicated tools, on most systems, compilation can be done one source
  file at a time, independent of the other ones, and linking is an independent step. (Although, linking
  using just the linker can be difficult in some cases, which is why a compiler/linker wrapper
  (the gcc command, for example) is often used, and since that wrapper isn't language agnostic
  and other languages such as C++ have their own, this makes linking multiple languages and/or
  associated standard libraries unreasonably difficult. But that's a more general problem
  for another day.)
* It doesn't depend on a speciffic architecture.

If another programming language is to be added to the project in the future, it should
satisfy the above points as well, and have a signifficant benefit justifying it's usage.

## Build system

This project uses plain old make. While it's not without its' flaws, it is a simple & common
but very powerful tool and more than sufficient for this project.

## Organisation of files

Header files are located in the `include/` directory. Source files are located in `src/`.
Generated include files are to be placed in `build/include/`. Files corresponding / strongly
relating to each other should have similar names and relative location to those directories.

Files in `include/-dpaw` are only to be used within the window manager. Always include them
using <> instead of "". This way, `<-dpaw/*>` will be part of the include, making it explicit
that it is a internal dpaw header file.

There are some special files in `include/-dpaw/` which the extension `.c` instead of `.h`.
These files generate code if compiled using `-DGENERATE_DEFINITIONS`, but are regular header
files otherwise. The makefile will compile them this way once. The code generation of
those files is usually handled by a separate `*.template` file, and have the purpose
of abstracting away common boilerplate code for things like used X11 extensions and X11 atoms.

There are variouse subprojects in this project. These are in their own folder with their
own makefile, although they share variouse options and common makefile parts by using the
`common.mk` makefile. The DPAW WM is usually independent of these subprojects, but they may
enhance some of it's functionality. If a subproject becomes too large, it should become it's
own independent project. A special subproject is the `api/` subproject, it builds the `libdpaw-wm.so`
convenience library, which makes it easier for programs to interact with the DPAW WM.
The `api/` subproject is also currently the only one the DPAW WM directly depends on, and
should be considered part of the DPAW WM itself.
