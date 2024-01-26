Currently only supports xorg desktop enviornments.

# USAGE
Record: `./input-playback`

Replay: `./input-playback /path/to/replay/file [-loop]`

> -loop is an optional flag to have the input playback repeated indefinitely.
# Compiling
`g++ main.cpp -o <output_name> -iX11 -iXtst`

## Dependencies
- libXtst / X11

## Planned Content
- a system that highlights your inputs as they are input
- non auto-playback 