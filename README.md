# Hacking my ECM Casa IV

This page describes trying to install a PID controller on my ECM Casa IV espresso machine.

## Shot journal server

The local FastAPI/SQLite shot journal (web UI and API) lives in [`src/server/`](src/server/README.md).
See its README for Docker setup, API documentation, and development commands.

## Building the docs

Docs made with [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/).
To preview, run `docker run --rm -it -p 8000:8000 -v ${PWD}:/docs squidfunk/mkdocs-material` [^running].

[^running]: See [here](https://squidfunk.github.io/mkdocs-material/creating-your-site/) for more ways on running the page.

