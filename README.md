# GameHook

The gamehook is a tool that wraps DirextX 11 to to intercept (and change) the rendering code of a video game. The main purpose for this is to enable the extraction of ground truth labels and the internal game state of popular video games.

The project (and code) is organized into a core code with a number of plugins interfacing the core. The core code compiles into `dxgi.dll` with hooks into the game through dll injection and function hooking. It wraps all of DirectX 11 and DXGI. Unless you found a bug or want to add a new feature you might not have to touch the core code (part of it is quite ugly, especially the HLSL injection).

The plugin part of the project is easier to understand and modify, most of the plugin interface is described in `SDK/sdk.h` and examples in the `examples` directory. Plugins allow you to do a wide range of things:
 * Inject code into vertex or pixel shaders
 * Copy and save internal buffers
 * Read the game state from internal buffers
 * Intercept and send keyboard and mouse commands
 * ...

Different plugins can also indirectly communicate with one another. For example a game plugin can provide a game state and several rendering outputs, which are then used or saved by another plugin (e.g. `server`, `capture`).

Gamehook also acts as an `asi` loader, and is compatible with scripthook.

If you find this code useful in a research please consider citing
```
@InProceedings{Kraehenbuehl_2018_CVPR,
author = {Philipp Kr\"henb\"uhl},
title = {Free Supervision From Video Games},
booktitle = {CVPR},
year = {2018}
}
```

## Getting started

You need Windows (10) and VC++ 2017 to compile the source. All dependencies (minhook, pybind11, simple-web-server, asio) are included in this repo.

Once compiled copy the `gamehook.dll` into the directory of the games main executable and rename it to `dxgi.dll`, also copy any of the plugins you with to use (files with ending `.hk`).
Then start the game. If the game was successfully hooked you'll see a command prompt with glorious debug information and a file called 'intercept.log'.

Make sure to remove any other asi loader you might be uses (e.g. `dinput8.dll`). Also make sure to disable the steam overlay, as it might interfere with gamehook.

There are currently two ways to view the captured output:
 1. `plugins/server` implements a simple HTTP server than allows a browser to interface with the gamehook
 2. `plugins/capture` dumps the captured files to disk

## Where to go from here?
If you're interested in hooking into GTA V, check out the [GTA V plugin](https://github.com/philkr/gamehook_gtav).
If you prefer to use python to prototype your plugin try `plugins/python`.

### Writing your own plugin
I highly recommend launching a new game without gamehook first and using renderdoc to debug it.
Renderdoc can show you how the rendering pipeline works, and how to hook into it.
It might also be a lot easier to modify an existing plugin than writing your own.

## Known bugs
If you're resizing the game window (change resolution) while gamehook is running be ready for the game to crash. Changing graphics settings should generally be save though.


## Extra credit

I'm currently uses QEMU 2.8.0 from my ubuntu 17.04 machine to run Windows. After a bit of fiddeling around I found a fairly good setup. 
In order to use GPU passthru you need to unbind the GPU you want to use **before** you login to X (from the commandline or ssh).
```bash
#!/usr/bin/sudo /bin/bash
echo 0000:04:00.0 > /sys/bus/pci/devices/0000\:04\:00.0/driver/unbind
echo 0000:04:00.1 > /sys/bus/pci/devices/0000\:04\:00.1/driver/unbind
```
If you call these lines after the X login your machine will likely crash within 10-30 sec.

Once you're logged in fire up the VM using the following command.

```bash
#!/usr/bin/sudo /bin/bash
vfiobind() {
    dev="$1"
        vendor=$(cat /sys/bus/pci/devices/$dev/vendor)
        device=$(cat /sys/bus/pci/devices/$dev/device)
        if [ -e /sys/bus/pci/devices/$dev/driver ]; then
                echo $dev > /sys/bus/pci/devices/$dev/driver/unbind
        fi
        echo $vendor $device > /sys/bus/pci/drivers/vfio-pci/new_id

}

modprobe vfio-pci

vfiobind 0000:04:00.0
vfiobind 0000:04:00.1

qemu-system-x86_64 -enable-kvm -m 64096 -cpu host,kvm=off -smp 8,sockets=1,cores=4,threads=2 \
-drive if=pflash,format=raw,readonly,file=/usr/share/OVMF/OVMF_CODE.fd \
-drive if=pflash,format=raw,file=./OVMF_VARS.fd \
-net nic \
-net user,smb=/media/philkr/,hostfwd=tcp::8765-:8765,hostfwd=tcp::8766-:8766,hostfwd=tcp::8767-:8767,hostfwd=tcp::3389-:3389 \
-device vfio-pci,host=04:00.0,multifunction=on \
-device vfio-pci,host=04:00.1 \
-drive format=raw,file=/dev/sdd1,if=virtio $@
```
where `OVMF_VARS.fd` is a local copy of `/usr/share/OVMF/OVMF_VARS.fd`.

## Why is the code so messy and poorly documented?

Because I have a life other than writing code.

