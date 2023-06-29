# epie

epie (as in e-pie) is a terminal text editor that is similar to vi.

## Features

- Modal Editing
- Numberline
- toml config file (default: ~/.config/epie/settings.toml)
- Auto indent

### Planned Features

- File Buffers
- Text selection
- Copy and paste
- Soft wrap
- Windows Support

## Configuration

edit configuration file

	[settings]
		number = true
		numberlen = 4
		message-timeout = 5
		tab-stop = 4
		separator = '|'

## Requirements

meson, c compiler, (mingw, cygwin for windows)

## Compile and Run

	$ meson build
	$ cd build
	$ meson compile --buildtype=release
	$ ./epie

### Install

	# meson install

## License

The epie source code is released under the BSD 2-Clause license.
