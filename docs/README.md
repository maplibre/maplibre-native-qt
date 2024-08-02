# Doxygen documentation

This directory contains the files needed to generate the documentation
of MapLibre Native Qt Bindings using [Doxygen](https://www.doxygen.nl).

[Doxygen Awesome](https://jothepro.github.io/doxygen-awesome-css/index.html)
is used as a theme and is included as a submodule.
To update, simply check out a new version of the submodule.

The `[Documentation.md](./Documentation.md)` file contains the contents of
the first page you see when you open the documentation website,
while `[config/header.html](./config/header.html)` and
`[config/footer.html](./config/footer.html)`contain the contents of
the header and the footer, respectively.

To generate the documentation, run

```shell
doxygen
```

in this directory.

While working on the documentation you might find it useful to run a file server.
For example, you can use:

```shell
cd output/html
python -m http.server
```
