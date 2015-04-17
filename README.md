# playground

This repository is a skeleton version of the [loftili core](https://github.com/loftili/core); including a very basic [gnu build system](https://www.gnu.org/software/autoconf/manual/autoconf-2.63/html_node/The-GNU-Build-System.html) setup. The purpose of code written in this repository is to explore c++ data structures, algorithms, and idoms - with the added bonus of having the foundational build sytem already in place.

## building 

```
$ ./configure
$ make
```

## brances

The master branch in this repository will be kept in the same state, aside from the changes to this README and any other "cosmetic" changes. The content of the `makefile` and `configure.ac` should remain relatively constant. Each exploratory topic will have it's own branch; this is prefered over using directories because each application will be given root of the codebase.
