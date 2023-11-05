# Victoria LLC challenge 1

Repository for the first challenge of the Victoria LLC selection process.

## Setting up

Clone the repository and install cmake if not installed.

```bash
cmake -B build -S .
cmake --build build
```

## Usage

```bash
>>> ./build/client -h
All options are required.
Usage: ./build/client [options]
Options:
  -e, --email     Email address
  -p, --password  Password (WARNING: insecure)
  -n, --name      Full name (enclose in quotes if it includes spaces)
  -g, --ghrepo    GitHub repository url
Example:
  ./build/client -e user@example.com -p password123 -n "John Doe" -g rep-url
```