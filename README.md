# Pebble Timer+

Timer+ is a beautiful, simple timer for the Pebble Smartwatch. It will run in the background, using
the WakeUp api, so there is no need to keep the app open. Once the timer has gone off, or even while
it is running, long pressing the select button will reset the timer. Additionally, starting a timer
from 0:00 will cause Timer+ to go into stopwatch mode.

![Timer+ Pebble](assets/screenshots/aplite-diorite-flint-animated.gif)
![Timer+ Pebble](assets/screenshots/basalt-animated.gif)
![Timer+ Pebble](assets/screenshots/chalk-animated.gif)

## Building

### Command Line

To simply obtain a pbw file, build the project using the `pebble` CLI tool.

```sh
pebble build
```

### VS Code

To configure Visual Studio Code with formatting and intellisense run the following commands. This
will need to be repeated any time the build configuration changes (aka file or package additions).

1. Ensure `bear` is installed: `sudo apt install bear`.
2. Ensure a clean build so all commands will be captured: `pebble clean`.
3. Create the build directory: `mkdir -p build`.
4. Generate the `compile_commands.json` file:
   `bear --output build/compile_commands.json -- pebble build`.
5. Run the `Show Recommended Extensions` command and install the suggested extensions.
6. Run `clangd: Restart language server` for changes to take effect.

After configuring, the project can be built either via the command line (`pebble build`) or by the
default build command (`ctrl + shift + b`).
