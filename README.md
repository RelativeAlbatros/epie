# Kilo

kilo is a terminal text editor in <1000 LoC that is similar to vi.

## Features

- numberline
- toml config file (default: ~/.config/kilo/settings.toml)
 
### Planned Features

- Auto indent
- Text selection
- Copy and paste
- Soft wrap

## Configuration

edit configuration file

	[settings]
		number = 1
		numberlen = 4
		message-timeout = 5
		tab-stop = 4
		separator = "|"

## Compile and Run

	$ meson build
	$ cd build
	$ meson compile
	$ ./kilo
 
### Install

	# meson install

## License

The kilo source code is released under the BSD 2-Clause license.

