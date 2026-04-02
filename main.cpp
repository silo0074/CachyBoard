#include "VirtualKeyboard.h"
#include <QApplication>

// curl -O src/main/backends/linux/wlroots/native/protocols/virtual-keyboard-unstable-v1.xml
// sudo pacman -S layer-shell-qt
// sudo pacman -S wayland-protocols
// sudo pacman -S wlr-protocols
// sudo pacman -S plasma-wayland-protocols
// sudo pacman -S --needed base-devel cmake qt6-base qt6-wayland wayland-protocols libx11 libxtst pkgconf

// Note: The zwp_input_method_v1_keysym expects a keysym rather than a raw Linux keycode. 
// If your compositor requires true keysyms, you may need to use a mapping table or libxkbcommon 
// to convert your KEY_A style constants into the appropriate keysym values.


int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  // Ensure we use the integrated Wayland support if available
  VirtualKeyboard w;
  w.show();

  return a.exec();
}
