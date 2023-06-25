# Kilo

kilo is a terminal text editor in <1000 LoC that is similar to vi.

## Features

- numberline
- toml config file (default: $HOME/.config/kilo/settings.toml)
 
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

## Compile

	meson build
	cd build
	meson compile

## License

The kilo source code is released under the BSD 2-Clause license.

