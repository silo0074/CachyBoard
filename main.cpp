#include "VirtualKeyboard.h"
#include <QApplication>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>

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
	
	// Construct the Application object
    QApplication a(argc, argv);
    
    // Set the desktop file name (for taskbar/Wayland)
	// This must match the 'StartupWMClass' in the .desktop file
    QGuiApplication::setDesktopFileName(APP_NAME);
	
	// Create a temporary path for the lock file.
	// A lock file is a temporary file created when the app starts.
	// If a second instance tries to start, it sees the file exists and is
	// "locked" by another Process ID (PID), so it exits.
	// If app crashes, QLockFile is smart enough to check if the PID stored in the lock file
	// is still active. If the PID is dead, it will automatically break the old lock
	// and let the new instance start.
	QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
	QLockFile lockFile(tmpDir + "/" + APP_NAME + "_unique_lock.lock");
	
	// Try to lock the file. If it fails, another instance is running.
	if (!lockFile.tryLock(100)) { // Wait 100ms to be sure
		qDebug() << APP_NAME << "is already running. Exiting.";
		return 0;
	}


	// Ensure we use the integrated Wayland support if available
	VirtualKeyboard w;
	w.show();

	return a.exec();
}
