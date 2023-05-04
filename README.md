A simple c++ based messaging library
===============

In a nutshell:

- It's based on the `nng` package which is a lightweight c-based messaging library. And we implemented a thin c++ interface to help you add pub/sub features in an easy manner.

- Fully asynchronous subscription support

## Prerequisites

- Install `nng` according to [guide](https://github.com/nanomsg/nng#quick-start)

- Install `protobuf`


## Quick Start

We provided a simple demo, and you can have a try.

## TODO

- Shared memory support

- Asynchronous publish support

- More robust MD5 check method.

## Special Thanks

- [hash-library](https://github.com/stbrumme/hash-library) provides a portable `MD5` calculation module.
