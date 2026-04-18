#include "VirtualKeyboard.h"
#include <QApplication>

// curl -O src/main/backends/linux/wlroots/native/protocols/virtual-keyboard-unstable-v1.xml
// sudo pacman -S layer-shell-qt
// sudo pacman -S wayland-protocols
// sudo pacman -S wlr-protocols
// sudo pacman -S plasma-wayland-protocols
// sudo pacman -S --needed base-devel cmake qt6-base qt6-wayland wayland-protocols libx11 libxtst pkgconf
// sudo pacman -S qt6-multimedia # for click sound
// https://github.com/onboard-osk/onboard/tree/main/sounds

// cat /etc/udev/rules.d/60-uinput.rules 

// Add yourself to input group (or uinput?)
// sudo usermod -aG input $USER

// # Give the uinput node correct permissions
// KERNEL=="uinput", GROUP="uinput", MODE="0660", OPTIONS+="static_node=uinput"

// # This rule finds ANY device that looks like a keyboard (ID_INPUT_KEYBOARD) 
// # and isn't your own virtual keyboard, then creates the symlink.
// ACTION=="add|change", KERNEL=="event*", ENV{ID_INPUT_KEYBOARD}=="1", ATTRS{name}!="Qt Virtual Keyboard Device", SYMLINK+="input/hw_kbd", GROUP="input", MODE="0660"

// sudo udevadm control --reload-rules && sudo udevadm trigger

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	// Ensure we use the integrated Wayland support if available
	VirtualKeyboard w;
	w.show();

	return a.exec();
}
