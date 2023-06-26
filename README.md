# epie

epie is a terminal text editor that is similar to vi.

## Features

- numberline
- toml config file (default: ~/.config/epie/settings.toml)
- Auto indent

### Planned Features

- Text selection
- Copy and paste
- Soft wrap

## Configuration

edit configuration file

	[settings]
		number = true
		numberlen = 4
		message-timeout = 5
		tab-stop = 4
		separator = '|'

## Compile and Run

	$ meson build
	$ cd build
	$ meson compile
	$ ./epie

### Install

	# meson install

## License

The epie source code is released under the BSD 2-Clause license.
