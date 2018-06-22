# Python plugin

This plugin loads all python files in the game directory.
Any module loaded this way contains an `api` module to interface with gamehook.
If the python plugin contains classes that inherit from `api.BaseController` they are loaded.
The python controllers currently support only a subset of commands.
Finally, python plugins can be reloaded seemlessly (press F11).

See `example.py` for a simple python example.