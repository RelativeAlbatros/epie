# epie

epie (as in e-pie) is a vim-like modal text editor for POSIX terminal.

## Features

- Modal Editing
- Numberline
- toml config file (default: *~/.config/epie/settings.toml* )
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

- POSIX compliant OS (currently no support for Windows and MacOS)
- Meson
- C compiler

## Compile and Run

	$ meson build
	$ cd build
	$ meson compile --buildtype=release
	$ ./epie

### Install

	# meson install

## License

The epie source code is released under the BSD 2-Clause license.
