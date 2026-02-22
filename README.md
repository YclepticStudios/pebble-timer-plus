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

To configure Visual Studio Code with formatting and intellisense perform the following:

1. Ensure `bear` is installed: `sudo apt install bear`.
2. Run the `Show Recommended Extensions` command and install them.
3. Run the default build command `ctrl + shift + b` to build the project.
   - This uses `bear` to generate a `compile_commands.json` file for clang while building the
     project.
